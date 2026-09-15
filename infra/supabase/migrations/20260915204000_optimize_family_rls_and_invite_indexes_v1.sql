-- World Makers cloud/runtime database production hardening.
--
-- 1. Cover the two family_invites foreign keys flagged by Supabase advisors.
-- 2. Evaluate auth.uid() once per statement in RLS policies rather than once
--    per candidate row. Policy semantics remain unchanged.

create index if not exists family_invites_created_by_idx
  on public.family_invites(created_by);

create index if not exists family_invites_used_by_idx
  on public.family_invites(used_by)
  where used_by is not null;

alter policy parents_self_select on public.parents
  using (id = (select auth.uid()));

alter policy parents_self_update on public.parents
  using (id = (select auth.uid()))
  with check (id = (select auth.uid()));

alter policy families_authenticated_insert on public.families
  with check ((select auth.uid()) is not null);

alter policy families_member_select on public.families
  using (
    exists (
      select 1
      from public.family_parents fp
      where fp.family_id = families.family_id
        and fp.parent_id = (select auth.uid())
    )
  );

alter policy family_parents_self_select on public.family_parents
  using (parent_id = (select auth.uid()));

alter policy family_parents_self_insert on public.family_parents
  with check (parent_id = (select auth.uid()));

alter policy children_member_select on public.children
  using (
    exists (
      select 1
      from public.family_parents fp
      where fp.family_id = children.family_id
        and fp.parent_id = (select auth.uid())
    )
  );

alter policy children_member_insert on public.children
  with check (
    exists (
      select 1
      from public.family_parents fp
      where fp.family_id = children.family_id
        and fp.parent_id = (select auth.uid())
    )
  );

alter policy children_member_update on public.children
  using (
    exists (
      select 1
      from public.family_parents fp
      where fp.family_id = children.family_id
        and fp.parent_id = (select auth.uid())
    )
  )
  with check (
    exists (
      select 1
      from public.family_parents fp
      where fp.family_id = children.family_id
        and fp.parent_id = (select auth.uid())
    )
  );

alter policy children_member_delete on public.children
  using (
    exists (
      select 1
      from public.family_parents fp
      where fp.family_id = children.family_id
        and fp.parent_id = (select auth.uid())
    )
  );

alter policy child_dashboard_stats_member_select on public.child_dashboard_stats
  using (
    exists (
      select 1
      from public.children c
      join public.family_parents fp on fp.family_id = c.family_id
      where c.child_profile_id = child_dashboard_stats.child_profile_id
        and fp.parent_id = (select auth.uid())
    )
  );
