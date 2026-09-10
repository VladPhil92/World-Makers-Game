# Parent Portal

Privacy-first parent/guardian application boundary for World Makers.

M4.1 upgrades the original scaffold into a runnable zero-dependency Node 22 MVP. The browser receives only minimized family read models; all child-scoped access is checked against a server-owned guardian session.

## Run locally

```bash
WORLD_MAKERS_ALLOW_DEMO_AUTH=true npm start
```

Open `http://127.0.0.1:4173` and choose **Open secure demo**.

Without `WORLD_MAKERS_ALLOW_DEMO_AUTH=true`, authenticated APIs fail closed because a production identity provider has not yet been selected.

## Local checks

```bash
npm ci
npm run lint
npm test
```

## Responsibilities

- Parent/guardian authentication boundary.
- Authorized child-profile summaries.
- Seven-day play-time summaries.
- Recent build/creation summaries.
- Learning-objective progress summaries.
- Family link requests pending backend verification.
- Privacy export/unlink/delete requests.

## Security and privacy boundaries

- No direct database access from browser code.
- No production credentials in browser bundles.
- No public child profiles.
- No raw telemetry, chat logs, exact learning answers, precise location or advertising IDs in the dashboard.
- Missing and unauthorized child IDs are externally indistinguishable.
- Guardian session cookie is HttpOnly and SameSite=Strict; production mode also requires Secure transport.

## Architecture decision

See `docs/adr/ADR-0001-parent-portal-stack.md`. M4.1 intentionally stays dependency-minimized until M4.2 selects production identity/backend topology. Next.js remains the preferred framework migration target once those boundaries are concrete.
