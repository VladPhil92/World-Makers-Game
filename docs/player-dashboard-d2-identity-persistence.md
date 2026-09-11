# D2 — Player Identity & Persistent Profile

## Purpose

D2 upgrades the D1 Player Hub from an in-memory demo session into a server-backed player profile boundary. The dashboard now treats identity, avatar/loadout, game selection, entitlements, progress summary, preferences and store approval requests as persistent profile data rather than transient browser/session state.

The flow becomes:

**CTG One identity assertion → World Makers session → persistent player profile → dashboard → signed launch context → Unreal**

## Identity boundary

D2 does not invent an OAuth implementation inside World Makers. Instead it defines a narrow assertion exchange contract for the existing CTG One identity layer.

A valid `ctg-one-identity-v1` assertion contains only:

- issuer `ctg-one`;
- audience `world-makers`;
- stable upstream subject;
- stable World Makers `playerProfileId`;
- display name;
- issued/expiry timestamps.

The assertion is HMAC-SHA256 authenticated by the integration boundary and expires within five minutes. Production verification is fail-closed unless `WORLD_MAKERS_IDENTITY_ASSERTION_SECRET` is configured with at least 32 characters.

World Makers does **not** persist the raw upstream subject. `deriveIdentityLinkId()` produces a one-way HMAC identity-link identifier. A profile ID that is later presented by a different identity link is rejected.

`POST /api/identity/session` validates the assertion, loads or creates the matching profile and then issues the normal HttpOnly `wm_player_session` cookie.

## Persistent profile

Canonical profile fields include:

- `playerProfileId`;
- opaque `identityLinkId`;
- display name;
- `avatar.child-explorer.v1`;
- seven-slot cosmetic loadout;
- selected game mode, world and optional mission;
- inventory entitlements;
- progress summary;
- reduced-motion and preferred-mode/world preferences;
- parent-approval store requests;
- optimistic `revision`;
- created/updated timestamps.

The reference adapter is `JsonFileProfileStore`. It writes atomically through a temporary file + `rename()` and creates the file with owner-only permissions (`0600`). It is intentionally simple and dependency-free, suitable for development/single-instance deployments and as the executable contract for a future database adapter.

Production must explicitly configure `WORLD_MAKERS_PROFILE_STORE_PATH`; the app does not silently use a development path when `NODE_ENV=production`.

## Optimistic concurrency

Every mutable dashboard request carries `profileRevision`.

The server compares the submitted revision with the latest persisted profile. A stale request returns:

`409 { "code": "profile_conflict" }`

The browser then reloads `/api/profile` and re-renders the latest server state. This prevents two tabs/devices from silently overwriting one another.

## Endpoints added/changed

- `POST /api/identity/session` — verified CTG One assertion → session.
- `GET /api/profile` — current persistent dashboard/profile read model.
- `PATCH /api/avatar/loadout` — revision-controlled persistent loadout.
- `PATCH /api/selection` — revision-controlled persistent mode/world/mission.
- `PATCH /api/preferences` — revision-controlled presentation preferences.
- `POST /api/store/requests` — parent-approval requests persist with the profile.
- `POST /api/launch-context` — reloads the current profile before signing; launch includes `profileRevision`.

## Launch integrity

D2 never signs an arbitrary browser state. The server reloads the authenticated player profile from the profile store immediately before creating `worldmakers-launch-v1`. Therefore Unreal receives the server-confirmed avatar, entitlements, mode/world/mission and the exact profile revision represented by that launch.

## Demo behavior

`WORLD_MAKERS_ALLOW_DEMO_AUTH=true` remains available for local development. The demo player is now also stored through the same persistent profile abstraction, so restarting the dashboard process does not inherently reset avatar/selection changes.

## Security properties

- HttpOnly + SameSite=Strict browser session cookie.
- No access token/password storage in browser JavaScript.
- No `localStorage`, `sessionStorage` or `document.cookie` usage.
- Signed, short-lived identity assertions.
- Opaque identity linkage rather than raw external subject persistence.
- Atomic profile writes.
- Optimistic revision conflict detection.
- No direct child purchase endpoint.
- No database/service-role/commerce secrets in browser code.

## Backend contracts

- `services/backend/contracts/player-profile.schema.json`
- `services/backend/contracts/player-identity-profile.openapi.yaml`

These are provider-neutral at the persistence layer while keeping the identity assertion boundary explicit.

## Production boundary

D2 is source-complete for identity assertion verification and durable single-instance profile persistence. It does **not** claim that CTG One's actual OIDC/login redirect has been wired into production, nor that the JSON profile adapter is the final horizontally scalable database.

A later production integration should replace the file adapter with a durable transactional datastore while preserving the same profile/revision contract. The actual browser-to-CTG-One login/return flow should issue the `ctg-one-identity-v1` assertion through a trusted server-side identity bridge, not expose upstream credentials to the Player Hub.

## Next phase

**D3 — Native Launcher & Unreal Session Bootstrap** should consume `worldmakers-launch-v1`, validate its signature/expiry/profile revision, open the requested mode/world/mission and initialize the avatar loadout before control is handed to the player.
