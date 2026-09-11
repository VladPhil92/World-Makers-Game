# Player Dashboard D3 — Native game launch handoff

## Objective

Turn the Player Hub home screen into the authenticated launch surface for World Makers. The user chooses mode/world/avatar in the dashboard and **INICIAR JUEGO** opens the installed native client with a short-lived signed launch context.

## Flow

1. The player authenticates in the Player Hub.
2. The dashboard loads the persistent player profile, avatar loadout, selected game mode and world.
3. The dashboard keeps **INICIAR JUEGO** disabled unless launch signing is configured.
4. A click on **INICIAR JUEGO** calls `POST /api/launch-context`.
5. The server creates the existing `worldmakers-launch-v1` HMAC-signed context with a 120-second TTL.
6. The server serializes that context into a native URI:

   `worldmakers://launch?protocol=worldmakers-launch-v1&payload=<base64url-json>`

7. The browser asks the operating system to open the registered World Makers client.
8. The native client receives the launch URI as its first command-line argument and can decode the payload before entering the selected mode/world.

## Dashboard contract

The launch response now contains:

```json
{
  "protocol": "worldmakers-launch-v1",
  "context": { "...": "signed short-lived launch context" },
  "launchUri": "worldmakers://launch?...",
  "next": "open-native-client"
}
```

The dashboard never invents launch state. The server remains authoritative for profile revision, selected content, entitlements, expiration and signature.

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

The packaged client must accept the full `worldmakers://launch?...` URI supplied by the OS, decode `payload` from Base64URL JSON, validate that `protocol` is `worldmakers-launch-v1`, reject expired contexts and then apply the selected player/avatar/world state. Signature verification must remain aligned with `src/domain/launch.mjs` until the launch contract is migrated to an asymmetric or one-time-ticket design.

## Security properties

- Launch remains fail-closed when `WORLD_MAKERS_LAUNCH_SIGNING_SECRET` is absent or shorter than 32 characters.
- Context lifetime remains 120 seconds.
- The native URI contains only the already-issued signed context; the signing secret is never sent to the browser.
- The dashboard does not silently fall back to an unsigned launch.
- A second explicit **Abrir juego nuevamente** action is available if the browser blocks or dismisses the first external-protocol prompt.

## Remaining release dependency

The dashboard-to-OS handoff is implemented by this phase. A real end-to-end launch still requires a packaged World Makers executable installed on the device and registered for the `worldmakers://` scheme. The existing native build/release workflows can produce that client; once installed, the dashboard launch button can invoke it directly.
