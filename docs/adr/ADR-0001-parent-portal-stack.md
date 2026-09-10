# ADR-0001 — Parent Portal MVP stack

- Status: Accepted for M4 MVP
- Date: 2026-09-10

## Context

The repository had a dependency-free parent-portal scaffold and had not selected a production identity, hosting or backend provider. M4 needs a runnable parent experience now, but authentication and child-family authorization are higher-risk boundaries than visual framework choice.

## Decision

M4.1 ships a dependency-minimized Node 22 web application using browser-native HTML/CSS/ES modules and Node's standard HTTP/crypto APIs.

The portal architecture is split into:

1. pure authorization/dashboard/privacy domain modules;
2. a server-owned session and API boundary;
3. a static browser client that receives only minimized read models;
4. provider-neutral OpenAPI/JSON Schema contracts.

Production authentication fails closed until a real identity provider is configured. Demo authentication is available only when `WORLD_MAKERS_ALLOW_DEMO_AUTH=true` and uses an in-memory opaque session cookie.

## Why not initialize Next.js in this phase?

Next.js remains the preferred migration target for the production portal, but installing a framework before identity/backend selection would add a larger package supply chain without solving the authorization problem. The MVP therefore proves the security/read-model boundary first.

## Migration trigger

Adopt the production web framework when M4.2 selects the identity/backend deployment topology. The migration must preserve:

- server-side relationship authorization;
- HttpOnly guardian sessions;
- no direct database access from browser code;
- minimized child read models;
- explicit privacy requests;
- accessibility and localization boundaries;
- existing domain tests as framework-independent contracts.

## Consequences

The M4.1 portal is runnable and testable but is not production-authenticated. Styling and routing are intentionally modest. No identity SDK, database client, analytics SDK, payment SDK or browser secret is introduced in this phase.
