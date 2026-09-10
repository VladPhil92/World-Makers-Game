# M4.1 — Parent Portal MVP

## Purpose

M4.1 turns the parent-portal scaffold into a runnable privacy-first family dashboard while preserving a strict boundary between browser UI, guardian authorization and game data.

This phase is **source-complete** when Repository Quality and the parent-portal checks pass. It is not production-authenticated until M4.2 selects and integrates a real identity/backend provider.

## Runtime boundary

```text
Browser
  |
  | same-origin requests
  v
Parent Portal server
  |
  +--> guardian session authority
  +--> child relationship authorization
  +--> minimized dashboard projection
  +--> privacy/link request validation
  |
  v
Provider adapter (M4.2)
```

The browser never receives backend credentials and never talks directly to gameplay databases.

## Guardian session

Production APIs fail closed until a provider exists. The production identity boundary therefore **fails closed** rather than creating a permissive fallback. Local/demo authentication is enabled only with:

`WORLD_MAKERS_ALLOW_DEMO_AUTH=true`

The demo uses an opaque in-memory session identifier stored in a cookie with `HttpOnly` and `SameSite=Strict`; production mode adds `Secure`.

Demo authentication is development evidence only. It is not a substitute for a production identity provider.

## Authorized child access

Every child-scoped read or request calls the same relationship guard. Missing and unauthorized child IDs intentionally return the same external response so the portal does not expose whether another family's child identifier exists.

The family read model contains pseudonymous stable child IDs and short display aliases. It does not require child legal names.

## Dashboard minimization

The parent dashboard exposes only:

- play time aggregated over seven days;
- session count aggregated over seven days;
- broad objective progress;
- recent creation summaries with bounded piece counts;
- adventure state;
- wellbeing invariants such as Right to Stop.

It excludes raw telemetry, chat, exact learning answers, precise location, advertising identifiers, purchase history and payment data.

## Child linking

A family link code contains exactly eight alphanumeric characters. Submitting one creates only `pending-backend-verification`; it never links a profile locally. M4.2 must verify the relationship and authorize the link server-side.

## Privacy controls

Guardian-only request types are:

- `export-child-data`;
- `unlink-child-profile`;
- `delete-child-data`.

All require explicit acknowledgement and repeat child relationship authorization. M4.1 returns `pending-backend-processing`; M4.2 must make these durable, auditable and legally compliant.

## Browser security baseline

The Node server applies:

- Content Security Policy;
- `X-Content-Type-Options: nosniff`;
- `X-Frame-Options: DENY`;
- `Referrer-Policy: no-referrer`;
- restrictive Permissions Policy including `payment=()`;
- `Cross-Origin-Opener-Policy: same-origin`;
- `Cache-Control: no-store` for APIs.

State-changing endpoints reject mismatched browser origins. Request bodies are bounded to 16 KiB.

The browser code does not use `innerHTML`, `eval`, `localStorage` or `sessionStorage` for authenticated state.

## Accessibility

The MVP includes:

- skip navigation;
- semantic headings/sections/forms;
- 44px minimum controls;
- visible keyboard focus;
- `aria-live` status regions;
- progressbar semantics;
- responsive one-column mobile layout;
- `prefers-reduced-motion` support.

English is the source locale for M4.1. M4.2 must introduce a localization framework before production rollout.

## API contracts

- `services/backend/contracts/parent-portal.openapi.yaml`
- `services/backend/contracts/parent-family-dashboard.schema.json`

The OpenAPI contract is provider-neutral and requires server-side guardian session authorization.

## Tests

`npm test` verifies:

- linked-profile filtering;
- cross-account access denial;
- minimized dashboard shape;
- privacy acknowledgement;
- link-code validation;
- HTTP session enforcement and child authorization.

`npm run lint` syntax-checks all runtime modules and enforces browser/server security markers.

Repository source gate:

`scripts/validate-m4-parent-portal.py`

## Production boundary

M4.1 deliberately does **not** claim:

- production identity;
- durable family-link persistence;
- real export/delete execution;
- cloud data aggregation;
- production localization;
- deployment certification.

Those are the next development stream: **M4.2 — Production Identity, Durable Family Linking & Backend Adapters**.
