# D1 — World Makers Player Dashboard Foundation

## Purpose

World Makers should not drop a signed-in player directly into Unreal. D1 introduces a web-facing **Player Hub** that sits between identity and gameplay:

**Authentication → Player Dashboard → Configuration → Secure Launch Context → Unreal**

The dashboard is a player-facing product surface, not an admin panel. It follows the eco-futurist visual language already established for World Makers.

## D1 user flow

1. Sign in through the player-facing web experience.
2. Land on **Inicio** with avatar preview, current adventure, selectable game modes and a prominent **JUGAR** CTA.
3. Open **Avatar Studio** to modify stable cosmetic slots.
4. Select a playable mode:
   - **Exploración Libre** — sandbox/creator experience with no mandatory mission.
   - **Aventuras & Misiones** — structured narrative learning adventures.
   - **Laboratorio** — controlled scientific experimentation and building.
   - **Cooperativo** is visible but locked for later work because child multiplayer requires a dedicated social-safety track.
5. Choose a compatible world and, for mission mode, a mission in that world.
6. Browse the **Store**. D1 is cosmetic-only and has no direct child purchase endpoint; store actions create `parent-approval-required` requests.
7. Press **JUGAR**. The dashboard asks the server for a short-lived HMAC-signed `worldmakers-launch-v1` context.
8. A future launcher/native bridge consumes that context and starts Unreal with the correct avatar, mode, world and mission.

## Avatar contract

The dashboard uses the existing `avatar.child-explorer.v1` modular shape. D1 exposes stable cosmetic IDs for:

- hair;
- top;
- bottom;
- footwear;
- head accessory;
- back accessory;
- hand prop.

The browser never generates biometric likenesses. It stores only stable cosmetic IDs and selected catalog values. D1 includes a stylized CSS preview; native 3D avatar preview is a later phase.

## Game mode contract

### `free-explore`

Free sandbox exploration and creation. Building, observation and experimentation can happen without a mandatory narrative objective.

### `missions`

Mission/adventure selection. D1 enforces that a mission belongs to the selected world before it can launch.

### `laboratory`

A distinct experimentation mode for physics, chemistry, biology, ecology, measurement and building. It is intentionally different from both sandbox exploration and linear adventures.

### `cooperative`

Visible future mode, but `playable=false` in D1. Multiplayer for children must not be enabled before moderation, communication, privacy, griefing and parental-control boundaries are designed.

## World selection

D1 ships with four launchable world descriptors:

- `world.rainforest`
- `world.research-island`
- `world.impossible-city`
- `world.moonforge`

Each world declares compatible mode IDs. The server validates compatibility; the browser cannot force an invalid combination.

## Store boundary

D1 Store is a **discovery/request surface**, not a payments backend.

Rules:

- cosmetics only;
- no gameplay power;
- no loot boxes;
- no random rewards;
- no direct purchase endpoint from the child/player app;
- request status is `parent-approval-required`;
- actual commerce and entitlement fulfillment belong to a later parent/commerce integration phase.

## Launch contract

`POST /api/launch-context` returns a signed, short-lived context only when `WORLD_MAKERS_LAUNCH_SIGNING_SECRET` is configured with at least 32 characters.

The payload contains:

- `playerId`
- `avatarId`
- `avatarLoadout`
- `selectedMode`
- `selectedWorld`
- `missionId`
- `inventoryEntitlements`
- `issuedAt`
- `expiresAt`
- HMAC-SHA256 `signature`

The server reports protocol `worldmakers-launch-v1`.

The dashboard never stores the signing secret in browser code and never emits a production context when the signing boundary is unavailable. PLAY therefore fails closed until native launch infrastructure is configured.

## Identity boundary

D1 mirrors the Parent Portal strategy:

- production identity provider is not hardcoded;
- demo authentication only exists when `WORLD_MAKERS_ALLOW_DEMO_AUTH=true`;
- cookies are `HttpOnly` and `SameSite=Strict`;
- no localStorage/sessionStorage authentication state;
- same-origin checks protect state-changing APIs.

## API surface

- `GET /api/health`
- `POST /api/demo/session`
- `GET /api/session`
- `DELETE /api/session`
- `PATCH /api/avatar/loadout`
- `PATCH /api/selection`
- `GET /api/store`
- `POST /api/store/requests`
- `POST /api/launch-context`

There is deliberately **no** `/api/store/purchase` endpoint.

## D1 completion boundary

D1 is complete when source CI proves:

- dashboard package remains dependency-minimized;
- three modes are playable and cooperative remains locked;
- avatar loadout validation is stable;
- invalid mode/world/mission combinations fail closed;
- store entries require parental approval;
- launch contexts are signed, expire and reject tampering;
- browser code avoids unsafe HTML and persistent token storage;
- Repository Quality executes both the source validator and the app's Node lint/tests.

## Next phases

D2 should connect real CTG One / World Makers identity, persistent player profile storage and entitlement synchronization.

D3 should replace the CSS avatar proxy with a native/WebGL avatar preview driven by the same stable loadout IDs and add actual wardrobe inventory state.

D4 should implement launcher/native deep-link handoff so clicking **JUGAR** starts or resumes Unreal using the signed launch context.
