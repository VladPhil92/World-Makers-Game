# Final Pre-Unreal Readiness Audit & Handoff Closure

This is the last gate before World Makers may be declared ready to enter native Unreal materialization. It does **not** certify native Unreal compilation, `.uasset` quality, `.umap` quality, device performance, or store packaging.

## What this gate closes

The gate combines the existing source-side readiness work with live external cloud evidence. `PRE_UNREAL_READY` is permitted only when the same repository commit has:

1. passed the Pre-Unreal Content & Platform source gate;
2. passed Cloud Runtime source validation;
3. passed Unreal source preflight and the UE 5.8.2 production baseline;
4. verified all three Railway production services as `SUCCESS`;
5. verified the three product custom domains over DNS + HTTPS;
6. executed a real CTG One Bearer player-state journey through the public World Makers runtime API;
7. proven GET, PUT, idempotent retry, stale-revision conflict and persistence;
8. removed temporary E2E marker data;
9. recorded no Bearer, secret value or child PII in evidence;
10. run on a clean checkout of current `main` with evidence for that exact commit.

## Production cloud topology under test

| Service | Product domain | Health path |
| --- | --- | --- |
| `worldmakers-api` | `api.worldmakers.ctgone.com` | `/api/ready` |
| `player-dashboard` | `play.worldmakers.ctgone.com` | `/api/health` |
| `parent-portal` | `parents.worldmakers.ctgone.com` | `/api/health` |

The domains being present in Railway is not enough. The final evidence requires them to resolve publicly and return successful HTTPS health responses.

## CTG One E2E safety rule

Use a **dedicated adult-owned test account**, never a child account. The probe writes a temporary non-PII marker to the test account, proves persistence/idempotency/conflict behavior, and removes the marker before exit. The account's numeric revision will advance; therefore do not use a real player's production identity.

The Bearer token is read only from the process environment:

```powershell
$env:WORLD_MAKERS_E2E_BEARER = '<temporary CTG One test bearer>'
```

Do not put the token in command history, JSON, GitHub, screenshots, CI variables for source-only workflows, or committed files. `scripts/probe-pre-unreal-cloud-e2e.py` clears the environment value in its own process before exit and never writes it to evidence.

## Generate live evidence

Start from clean current `main` and obtain its commit:

```powershell
git checkout main
git pull --ff-only origin main
$commit = (git rev-parse HEAD).Trim()
```

Confirm Railway independently, then execute the probe with the observed deployment statuses:

```powershell
python scripts/probe-pre-unreal-cloud-e2e.py `
  --repository-commit $commit `
  --observer release-ops `
  --railway-status parent-portal=SUCCESS `
  --railway-status player-dashboard=SUCCESS `
  --railway-status worldmakers-api=SUCCESS `
  --allow-mutating-dedicated-test-account
```

The output is written to:

```text
artifacts/pre-unreal-handoff/cloud-e2e.json
```

The probe fails closed if a domain does not resolve, HTTPS health is not 2xx, CTG One authentication fails, the write does not advance revision, the same idempotency key advances revision twice, stale `expectedRevision` does not return `revision_conflict`, persistence is not observable, or temporary probe data cannot be cleaned.

If the probe reports `EMERGENCY CLEANUP REQUIRED`, stop and restore the dedicated test account before any handoff decision.

## Final certification command

Once the sanitized external evidence exists:

```bat
WorldMakers-FinalPreUnreal-Certify.cmd
```

The runner re-executes all source validators, fetches `origin/main`, checks the current commit and worktree, and runs the final assessor.

Possible results:

- `PRE_UNREAL_READY`: all source and external pre-native evidence passed on clean current `main` for the same commit.
- `NON_CERTIFYING_PASS`: the evidence itself passed but the checkout is not an official certifying context.
- `BLOCKED`: one or more required conditions are absent or failed.

## CI truth boundary

GitHub Actions runs source validation and synthetic fail-closed tests only. Synthetic self-test data is never evidence of a live Railway, DNS, CTG One or Supabase journey. CI therefore cannot issue `PRE_UNREAL_READY` by itself.

The assessor specifically blocks, among other things:

- unhealthy custom domain;
- failed idempotent retry;
- missing revision-conflict behavior;
- evidence that records secret values;
- evidence produced for a different repository commit.

## Known external closure boundary

The 2026-09-15 operational audit found all three Railway services deployed with `SUCCESS`, and the dedicated World Makers Supabase project was `ACTIVE_HEALTHY`. Railway has the three custom domains registered. Those observations are not substitutes for final same-commit evidence: public DNS/HTTPS and the authenticated CTG One E2E must still pass the probe on the handoff commit.

## Handoff truth

`PRE_UNREAL_READY` means the known source-side and external prerequisites for native materialization have been closed. It does not mean native Unreal certification has happened. Native compilation, authored binary assets, maps, visual review, device profiling and packaging remain later native evidence domains.
