-- Follow-up hardening for the CTG One player-state bridge.
--
-- Serialize writes per CTG One subject before checking idempotency receipts or
-- profile revisions. This closes both the revision-zero profile creation race
-- and the overlapping same-event retry race without weakening the legacy RPC
-- revocations introduced by the previous migration.

create or replace function public.wm_bridge_player_state(
  p_operation text,
  p_subject uuid,
  p_timestamp bigint,
  p_nonce text,
  p_event_id text,
  p_expected_revision integer,
  p_payload text,
  p_signature text
)
returns jsonb
language plpgsql
security definer
set search_path = 'public'
as $function$
declare
  v_secret text;
  v_payload_hash text;
  v_canonical text;
  v_expected_signature text;
  v_profile_id text := 'player.ctg.' || p_subject::text;
  v_identity_link_id text := 'ctg_one:' || p_subject::text;
  v_row public.player_profiles;
  v_current_revision integer;
  v_new_revision integer;
  v_payload jsonb;
  v_progress jsonb;
  v_display_name text;
  v_response jsonb;
begin
  if p_operation not in ('read', 'write') then
    raise exception 'bridge_invalid_operation';
  end if;

  if p_timestamp is null
     or abs(extract(epoch from now())::bigint - p_timestamp) > 300 then
    raise exception 'bridge_timestamp_expired';
  end if;

  if p_nonce is null or p_nonce !~ '^[A-Za-z0-9_-]{16,128}$' then
    raise exception 'bridge_invalid_nonce';
  end if;

  if p_signature is null or p_signature !~ '^[0-9A-Fa-f]{64}$' then
    raise exception 'bridge_auth_failed';
  end if;

  select decrypted_secret
    into v_secret
    from vault.decrypted_secrets
   where name = 'worldmakers_ctg_bridge_hmac_v1'
   limit 1;

  if v_secret is null or length(v_secret) < 32 then
    raise exception 'bridge_secret_unavailable';
  end if;

  if length(coalesce(p_payload, '')) > 65536 then
    raise exception 'bridge_payload_too_large';
  end if;

  v_payload_hash := encode(
    extensions.digest(convert_to(coalesce(p_payload, ''), 'UTF8'), 'sha256'),
    'hex'
  );

  v_canonical := concat_ws(
    '|',
    'v1',
    p_operation,
    p_subject::text,
    p_timestamp::text,
    p_nonce,
    coalesce(p_event_id, ''),
    coalesce(p_expected_revision::text, ''),
    v_payload_hash
  );

  v_expected_signature := encode(
    extensions.hmac(convert_to(v_canonical, 'UTF8'), convert_to(v_secret, 'UTF8'), 'sha256'),
    'hex'
  );

  if lower(p_signature) <> v_expected_signature then
    raise exception 'bridge_auth_failed';
  end if;

  delete from public.wm_bridge_nonces
   where seen_at < now() - interval '15 minutes';

  insert into public.wm_bridge_nonces(nonce, subject)
  values (p_nonce, p_subject)
  on conflict do nothing;

  if not found then
    raise exception 'bridge_replay';
  end if;

  if p_operation = 'read' then
    select * into v_row
      from public.player_profiles
     where player_profile_id = v_profile_id;

    if not found then
      return jsonb_build_object(
        'schemaVersion', 1,
        'exists', false,
        'playerProfileId', v_profile_id,
        'revision', 0,
        'displayName', null,
        'saves', '[]'::jsonb,
        'missions', '[]'::jsonb,
        'discoveries', '[]'::jsonb,
        'achievements', '[]'::jsonb,
        'updatedAt', null
      );
    end if;

    return jsonb_build_object(
      'schemaVersion', 1,
      'exists', true,
      'playerProfileId', v_row.player_profile_id,
      'revision', v_row.revision,
      'displayName', v_row.display_name,
      'saves', coalesce(v_row.progress->'saves', '[]'::jsonb),
      'missions', coalesce(v_row.progress->'missions', '[]'::jsonb),
      'discoveries', coalesce(v_row.progress->'discoveries', '[]'::jsonb),
      'achievements', coalesce(v_row.progress->'achievements', '[]'::jsonb),
      'updatedAt', to_jsonb(v_row.updated_at)
    );
  end if;

  if p_event_id is null or p_event_id !~ '^[A-Za-z0-9._:-]{8,128}$' then
    raise exception 'bridge_invalid_event_id';
  end if;

  if p_expected_revision is null or p_expected_revision < 0 then
    raise exception 'bridge_invalid_revision';
  end if;

  perform pg_advisory_xact_lock(hashtextextended(p_subject::text, 0));

  select response into v_response
    from public.wm_bridge_events
   where subject = p_subject and event_id = p_event_id;

  if found then
    return v_response;
  end if;

  begin
    v_payload := coalesce(nullif(p_payload, ''), '{}')::jsonb;
  exception when others then
    raise exception 'bridge_invalid_payload';
  end;

  if jsonb_typeof(v_payload) <> 'object' then
    raise exception 'bridge_invalid_payload';
  end if;

  if jsonb_typeof(coalesce(v_payload->'saves', '[]'::jsonb)) <> 'array'
     or jsonb_typeof(coalesce(v_payload->'missions', '[]'::jsonb)) <> 'array'
     or jsonb_typeof(coalesce(v_payload->'discoveries', '[]'::jsonb)) <> 'array'
     or jsonb_typeof(coalesce(v_payload->'achievements', '[]'::jsonb)) <> 'array' then
    raise exception 'bridge_invalid_payload';
  end if;

  select * into v_row
    from public.player_profiles
   where player_profile_id = v_profile_id
   for update;

  if found then
    v_current_revision := v_row.revision;
    if v_current_revision <> p_expected_revision then
      raise exception 'bridge_profile_conflict';
    end if;
  else
    v_current_revision := 0;
    if p_expected_revision <> 0 then
      raise exception 'bridge_profile_conflict';
    end if;
  end if;

  v_new_revision := v_current_revision + 1;
  v_display_name := left(trim(coalesce(nullif(v_payload->>'displayName', ''), v_row.display_name, 'Maker')), 80);

  v_progress := coalesce(v_row.progress, '{}'::jsonb) || jsonb_build_object(
    'saves', coalesce(v_payload->'saves', '[]'::jsonb),
    'missions', coalesce(v_payload->'missions', '[]'::jsonb),
    'discoveries', coalesce(v_payload->'discoveries', '[]'::jsonb),
    'achievements', coalesce(v_payload->'achievements', '[]'::jsonb)
  );

  insert into public.player_profiles (
    player_profile_id,
    identity_link_id,
    display_name,
    avatar_id,
    loadout,
    selection,
    entitlements,
    progress,
    preferences,
    store_requests,
    revision,
    updated_at
  ) values (
    v_profile_id,
    v_identity_link_id,
    v_display_name,
    coalesce(v_row.avatar_id, 'default'),
    coalesce(v_row.loadout, '{}'::jsonb),
    coalesce(v_row.selection, '{}'::jsonb),
    coalesce(v_row.entitlements, '[]'::jsonb),
    v_progress,
    coalesce(v_row.preferences, '{}'::jsonb),
    coalesce(v_row.store_requests, '[]'::jsonb),
    v_new_revision,
    now()
  )
  on conflict (player_profile_id) do update set
    identity_link_id = excluded.identity_link_id,
    display_name = excluded.display_name,
    progress = excluded.progress,
    revision = excluded.revision,
    updated_at = excluded.updated_at
  returning * into v_row;

  v_response := jsonb_build_object(
    'schemaVersion', 1,
    'exists', true,
    'playerProfileId', v_row.player_profile_id,
    'revision', v_row.revision,
    'displayName', v_row.display_name,
    'saves', coalesce(v_row.progress->'saves', '[]'::jsonb),
    'missions', coalesce(v_row.progress->'missions', '[]'::jsonb),
    'discoveries', coalesce(v_row.progress->'discoveries', '[]'::jsonb),
    'achievements', coalesce(v_row.progress->'achievements', '[]'::jsonb),
    'updatedAt', to_jsonb(v_row.updated_at)
  );

  insert into public.wm_bridge_events(subject, event_id, response)
  values (p_subject, p_event_id, v_response)
  on conflict (subject, event_id) do nothing;

  return v_response;
end;
$function$;

revoke all on function public.wm_bridge_player_state(text, uuid, bigint, text, text, integer, text, text)
  from public, authenticated;
grant execute on function public.wm_bridge_player_state(text, uuid, bigint, text, text, integer, text, text)
  to anon, service_role;
