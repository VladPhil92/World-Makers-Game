# World Makers Runtime API

Production-facing gateway for game clients. It exposes only the bounded player-state surface required by World Makers and forwards authenticated requests to the CTG One identity/runtime authority.

## Endpoints

- `GET /api/health` — process liveness; never includes secrets.
- `GET /api/ready` — configuration readiness.
- `GET /api/worldmakers/player-state` — authenticated cloud-state read.
- `PUT /api/worldmakers/player-state` — authenticated, revision-controlled, idempotent cloud-state write.

## Security boundary

The client sends a CTG One Bearer token. This gateway forwards that token only to the configured HTTPS CTG One API origin. Cookies and arbitrary client headers are not forwarded. Writes require an `Idempotency-Key`, validate the four allowed state arrays, enforce a 64 KiB body ceiling and preserve the upstream optimistic-revision contract.

The service contains no Supabase service-role key, database password, CTG bridge HMAC secret, payment credential, child free-text channel, or direct privileged database access.

## Railway

Use `apps/worldmakers-api` as the service root, `npm start` as the start command and `/api/health` as the Railway healthcheck. Configure:

```text
NODE_ENV=production
CTG_ONE_API_BASE_URL=https://ctgone.com
```

The service may be alive while `/api/ready` returns 503 if the upstream URL is missing or invalid. Production promotion requires both endpoints healthy.
