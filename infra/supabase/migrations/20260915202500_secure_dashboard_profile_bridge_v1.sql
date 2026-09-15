-- World Makers dashboard profile bridge v1.
--
-- The public Supabase anon key is not sufficient to call the legacy profile
-- RPCs. Railway services authenticate this wrapper with a high-entropy
-- server-only secret whose SHA-256 digest is intentionally safe to version.
-- Rotate WORLD_MAKERS_PROFILE_BRIDGE_SECRET by deploying a new digest migration
-- and updating both Railway services atomically.

create or replace function public.wm_secure_player_profile(
  p_operation text,
  p_player_profile_id text,
  p_expected_revision integer,
  p_payload jsonb,
  p_server_secret text
)
returns jsonb
language plpgsql
security definer
set search_path = 'public'
as $function$
declare
  v_secret_hash constant text := 'ec77329d084a6b40a6e54993cbca1ba2c789c837e02b61eb3a53a72cb6598991';
  v_actual_hash text;
  v_deleted boolean;
begin
  if p_operation not in ('get', 'put', 'delete') then
    raise exception 'profile_bridge_invalid_operation';
  end if;

  if p_player_profile_id is null
     or p_player_profile_id !~ '^player\.[a-zA-Z0-9._:-]{3,120}$' then
    raise exception 'invalid_player_profile_id';
  end if;

  if p_server_secret is null or length(p_server_secret) < 48 then
    raise exception 'profile_bridge_auth_failed';
  end if;

  v_actual_hash := encode(
    extensions.digest(convert_to(p_server_secret, 'UTF8'), 'sha256'),
    'hex'
  );

  if v_actual_hash <> v_secret_hash then
    raise exception 'profile_bridge_auth_failed';
  end if;

  if p_operation = 'get' then
    return public.wm_get_player_profile(p_player_profile_id);
  end if;

  -- Serialize mutations per profile, including first-write creation where a
  -- SELECT ... FOR UPDATE would otherwise lock no row.
  perform pg_advisory_xact_lock(hashtextextended(p_player_profile_id, 1));

  if p_operation = 'put' then
    if p_payload is null or jsonb_typeof(p_payload) <> 'object' then
      raise exception 'profile_bridge_invalid_payload';
    end if;
    if p_payload->>'playerProfileId' is distinct from p_player_profile_id then
      raise exception 'profile_bridge_profile_mismatch';
    end if;
    if p_expected_revision is null or p_expected_revision < 0 then
      raise exception 'profile_bridge_invalid_revision';
    end if;
    return public.wm_put_player_profile(p_payload, p_expected_revision);
  end if;

  v_deleted := public.wm_delete_player_profile(p_player_profile_id);
  return jsonb_build_object('deleted', v_deleted);
end;
$function$;

revoke all on function public.wm_secure_player_profile(text, text, integer, jsonb, text)
  from public, authenticated;
grant execute on function public.wm_secure_player_profile(text, text, integer, jsonb, text)
  to anon, service_role;
