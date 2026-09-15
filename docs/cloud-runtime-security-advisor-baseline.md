# Cloud Runtime — Supabase Advisor Baseline

This note records the production database advisor disposition after the Cloud/Runtime Readiness deployment. It is not a waiver for future findings.

## Remediated performance findings

`20260915204000_optimize_family_rls_and_invite_indexes_v1.sql` adds covering indexes for `family_invites.created_by` and `family_invites.used_by`, and rewrites the family/child RLS policies so `(select auth.uid())` is evaluated once per statement rather than once per candidate row.

Immediately after applying the migration, the previous `unindexed_foreign_keys` and `auth_rls_initplan` findings were no longer reported. Newly-created indexes may temporarily appear as `unused_index` until production traffic exercises them; that is expected and should be reviewed after representative usage rather than removed immediately.

## Intentional deny-all RLS tables

`player_profiles`, `wm_bridge_nonces`, `wm_bridge_events`, and `family_invites` can report `rls_enabled_no_policy` because direct row access is intentionally denied to ordinary roles. Their supported operations are mediated by narrowly-scoped functions and/or authenticated family flows. Adding permissive table policies merely to silence the advisor would weaken the boundary.

## Intentional SECURITY DEFINER RPC exposure

Supabase can report `anon_security_definer_function_executable` for:

- `wm_bridge_player_state`
- `wm_secure_player_profile`

The `anon` role is only the PostgREST transport role. Neither function authorizes a mutation based on possession of the publishable anon key:

- `wm_bridge_player_state` verifies a server-generated HMAC, timestamp, one-time nonce, operation, subject, event ID, revision and payload hash before touching player state.
- `wm_secure_player_profile` hashes a high-entropy server-only bridge secret and compares it to the versioned digest before executing get/put/delete operations.

The secret/HMAC values are never committed, sent to browsers, or sent to game clients. Legacy arbitrary-profile RPCs remain revoked from public, anon and authenticated roles.

These findings are therefore **accepted-by-design**, not ignored. Any future change that removes the in-function authentication, exposes either secret to a client, or broadens the RPC payload must reopen this security review.

Supabase remediation references:
- https://supabase.com/docs/guides/database/database-linter?lint=0028_anon_security_definer_function_executable
- https://supabase.com/docs/guides/database/database-linter?lint=0008_rls_enabled_no_policy
- https://supabase.com/docs/guides/database/database-linter?lint=0003_auth_rls_initplan
