-- Guarantee child privacy deletion is atomic with linked player-profile removal.
--
-- Parent Portal already requests deletion through the secure profile bridge before
-- deleting the child row. This trigger is the database-level invariant: even if
-- that best-effort pre-delete call is unavailable, deleting an authorized child
-- cannot leave the corresponding World Makers player profile orphaned.

create or replace function public.wm_cascade_child_player_profile_delete()
returns trigger
language plpgsql
security definer
set search_path = 'public'
as $function$
declare
  v_player_profile_id text;
begin
  v_player_profile_id := 'player.' || regexp_replace(old.child_profile_id, '^child\.', '');
  delete from public.player_profiles
   where player_profile_id = v_player_profile_id;
  return old;
end;
$function$;

revoke all on function public.wm_cascade_child_player_profile_delete()
  from public, anon, authenticated;

drop trigger if exists wm_children_cascade_player_profile_delete on public.children;
create trigger wm_children_cascade_player_profile_delete
before delete on public.children
for each row
execute function public.wm_cascade_child_player_profile_delete();
