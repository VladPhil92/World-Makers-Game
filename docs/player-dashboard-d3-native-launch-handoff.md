# Player Dashboard D3 — Native game launch handoff

## Objective

Turn the Player Hub home screen into the authenticated launch surface for World Makers. The user chooses mode/world/avatar in the dashboard and **INICIAR JUEGO** opens the installed native client with short-lived server-authoritative launch state.

> **M5.6F update:** D3 originally transported the full `worldmakers-launch-v1` signed context inside the custom URI. That v1 encoder/decoder remains for compatibility and tests, but the active external transport is now the one-time-ticket `worldmakers-launch-v2` flow defined in `docs/m5-6f-native-bootstrap-progress-sync.md`.

## Current flow

1. The player authenticates in the Player Hub.
2. The dashboard loads the persistent player profile, avatar loadout, selected game mode/world and resumable epic state.
3. The dashboard keeps **INICIAR JUEGO** disabled unless launch signing is configured.
4. A click on **INICIAR JUEGO** calls `POST /api/launch-context`.
5. The server creates the canonical `worldmakers-launch-v1` HMAC-signed context with a 120-second TTL.
6. M5.6F binds that context to a cryptographically random, one-time launch ticket and returns a ticket-only native URI:

   `worldmakers://launch?protocol=worldmakers-launch-v2&ticket=<opaque-token>`

7. The browser asks the operating system to open the registered World Makers client.
8. The native client receives the URI, validates its v2 shape, and redeems the ticket over the configured trusted dashboard API.
9. The server consumes the ticket, re-verifies the v1 HMAC context and returns the verified context plus a narrowly scoped epic-checkpoint sync token.
10. Unreal applies any `epicResume` checkpoint without regressing newer local progress.

## Dashboard contract

The launch response contains both the server-owned context for browser UX and the ticket-only external URI:

```json
{
  "protocol": "worldmakers-launch-v2",
  "contextProtocol": "worldmakers-launch-v1",
  "context": { "...": "signed short-lived launch context" },
  "launchTicketExpiresAt": "...",
  "launchUri": "worldmakers://launch?protocol=worldmakers-launch-v2&ticket=...",
  "next": "open-native-client"
}
```

The dashboard never invents launch state. The server remains authoritative for profile revision, selected content, entitlements, expiration, signature and the ticket-to-context binding.

## Legacy v1 compatibility

`createNativeLaunchUri()` / `decodeNativeLaunchUri()` still encode and decode:

`worldmakers://launch?protocol=worldmakers-launch-v1&payload=<base64url-json>`

They remain in the repository so older callers and contract tests do not break abruptly. New launch responses must use v2 and must not place the player context in the OS-visible URI.

## Windows protocol registration

For a packaged Windows build, register the executable for the current user:

```powershell
powershell -ExecutionPolicy Bypass -File apps/player-dashboard/scripts/register-worldmakers-protocol.ps1 -ExecutablePath "C:\Path\To\WorldMakers.exe"
```

No administrator privileges are required because the handler is registered under `HKCU\Software\Classes\worldmakers`.

To remove the handler:

```powershell
powershell -ExecutionPolicy Bypass -File apps/player-dashboard/scripts/register-worldmakers-protocol.ps1 -Unregister
```

## Native client responsibility

For the active v2 contract, the packaged client must:

- accept the full `worldmakers://launch?...` URI supplied by the OS;
- require `protocol=worldmakers-launch-v2`;
- reject `payload` in a v2 URI;
- accept only a bounded base64url-style opaque ticket;
- redeem that ticket against the configured trusted API endpoint;
- reject failed/expired redemption;
- apply `epicResume` through the monotonic local checkpoint merge;
- keep the progress-sync credential in process memory only.

The native client does **not** compile in or receive `WORLD_MAKERS_LAUNCH_SIGNING_SECRET`.

## Security properties

- Launch remains fail-closed when `WORLD_MAKERS_LAUNCH_SIGNING_SECRET` is absent or shorter than 32 characters.
- Launch-ticket lifetime remains 120 seconds by default.
- The v2 native URI contains no player profile, entitlements, epic checkpoint or HMAC secret.
- A launch ticket is single use and stored server-side by digest.
- The server re-verifies the signed v1 context during redemption.
- Production native bootstrap accepts HTTPS API endpoints only; loopback HTTP is non-Shipping development-only.
- The dashboard does not silently fall back to an unsigned launch.
- A second explicit **Abrir juego nuevamente** action can reuse the same URI only until the ticket has been consumed; after redemption, a new launch context/ticket is required.

## Remaining release dependency

The source-level dashboard-to-native bootstrap and epic progress-sync contract is implemented by M5.6F. A real end-to-end launch still requires a packaged World Makers executable installed on the device, registered for the `worldmakers://` scheme, plus the Issue #9 native Unreal infrastructure needed to build and collect trustworthy runtime evidence.
