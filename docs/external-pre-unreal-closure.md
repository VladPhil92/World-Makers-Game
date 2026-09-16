# External Pre-Unreal Closure Execution

This phase closes the external network boundary before native Unreal materialization. It is intentionally separate from the authenticated player-state probe so DNS/HTTPS failures can be diagnosed without requiring or persisting a CTG One Bearer token.

## Public surface probe

Run:

```powershell
python scripts/probe-pre-unreal-public-health.py --repository-commit (git rev-parse HEAD) --observer local-release-ops
```

The probe checks, for each production service:

- public DNS resolution of the custom `*.worldmakers.ctgone.com` host;
- TLS/HTTPS reachability;
- the required 2xx health/readiness endpoint;
- the corresponding Railway-provided fallback domain.

The fallback comparison matters. A healthy Railway host plus an unhealthy custom host isolates the problem to custom DNS, ownership verification, certificate issuance or Railway custom-domain routing rather than the application process itself.

## Railway domain requirements

Railway custom domains require both the routing record and the ownership-verification record supplied by Railway. For subdomains this means a CNAME plus the Railway TXT verification record. The exact values must come from Railway's domain status/details; do not invent targets or verification tokens.

Current production inventory is versioned in `content/production/pre-unreal-public-endpoints-v1.json`.

## GitHub Actions

`External Pre-Unreal Closure` runs the public probe from a GitHub-hosted runner. The live probe uses `--soft-fail` so an expected external DNS problem does not block source integration. Its JSON artifact is retained for 14 days.

A green workflow means only that the diagnostic job executed correctly. Inspect `artifacts/pre-unreal-handoff/public-health.json` or the uploaded artifact for `status` and per-service `classification`.

## Authenticated closure

After all custom domains are healthy, run the existing authorized E2E probe with a temporary Bearer from a dedicated adult-owned CTG One test account:

```powershell
$env:WORLD_MAKERS_E2E_BEARER='<temporary-token>'
python scripts/probe-pre-unreal-cloud-e2e.py `
  --repository-commit (git rev-parse HEAD) `
  --observer release-ops `
  --railway-status parent-portal=SUCCESS `
  --railway-status player-dashboard=SUCCESS `
  --railway-status worldmakers-api=SUCCESS `
  --allow-mutating-dedicated-test-account
Remove-Item Env:WORLD_MAKERS_E2E_BEARER
```

The authenticated probe proves GET, PUT, idempotent retry, stale-revision conflict, persistence and semantic cleanup. The Bearer must never be pasted into GitHub, committed, or recorded in evidence.

## Final handoff

Only after both public and authenticated evidence are valid, run from a clean current `main`:

```powershell
WorldMakers-FinalPreUnreal-Certify.cmd
```

`PRE_UNREAL_READY` still does not mean native Unreal compilation, authored `.uasset`/`.umap` quality, representative-device performance or store packaging has passed. It only authorizes entry into native Unreal materialization.
