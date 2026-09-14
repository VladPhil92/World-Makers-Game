# M5.6F — Native Bootstrap and Durable Epic Progress Sync

Status: **source implementation complete; native/package execution evidence remains gated by Issue #9**.

## Goal

M5.6F closes the continuity loop created by M5.6D and hardened by M5.6E:

1. the Player Dashboard creates a server-authoritative signed launch context;
2. the external `worldmakers://` URI carries only an opaque, one-time ticket;
3. Unreal redeems that ticket against the trusted dashboard API;
4. an `epicResume` checkpoint is merged into local SaveGame state without regression;
5. future locally committed chapter checkpoints are synced back to the durable player profile using a narrowly scoped token.

The design deliberately avoids embedding `WORLD_MAKERS_LAUNCH_SIGNING_SECRET` in Unreal. The HMAC remains a server-side integrity mechanism for the internal v1 context; the native client receives that context only after the server has redeemed the one-time ticket and re-verified the signature.

## Protocol split

### Internal context contract — `worldmakers-launch-v1`

The existing HMAC-signed context remains the canonical server-owned launch representation. It contains player identity, avatar/loadout, selected mode/world, entitlements, expiry and optional `epicResume`.

### External native handoff — `worldmakers-launch-v2`

The browser now opens:

`worldmakers://launch?protocol=worldmakers-launch-v2&ticket=<opaque-token>`

The v2 URI must not contain `payload`, player identity, entitlements, chapter data or the server HMAC secret.

Launch tickets are:

- generated from 32 cryptographically random bytes;
- stored server-side by SHA-256 digest rather than plaintext key;
- single use;
- valid for at most 120 seconds by default;
- bound to the signed context and profile revision that existed when issued.

## Native redemption

`UWMLaunchBootstrapSubsystem` detects a v2 launch URI in the process command line and calls:

`POST /api/native/launch/redeem`

with only the opaque ticket.

The server removes the launch ticket before returning any context, re-verifies the HMAC-signed v1 context, and returns:

- the verified launch context;
- a separate progress-sync token;
- scope exactly `epic-checkpoint:write`;
- an independent sync-token expiry.

The native client never needs or receives `WORLD_MAKERS_LAUNCH_SIGNING_SECRET`.

## Trusted API endpoint

The native client reads the dashboard API base URL from `WORLD_MAKERS_API_BASE_URL` or `/Script/WorldMakers.WMLaunchBootstrapSubsystem` in `DefaultGame.ini`.

Production/Shipping accepts HTTPS only. Non-Shipping builds may additionally use loopback `http://localhost` or `http://127.0.0.1` for local development. The source configuration intentionally leaves `ApiBaseUrl` blank; a v2 native launch therefore fails closed until deployment supplies a trusted endpoint.

## Auto-start race prevention

A v2 launch suppresses configuration-driven Eclipse/Garden auto-start while ticket redemption is pending or failed. This prevents the prototype's `bStartEclipseEngineVerticalSlice=True` setting from starting chapter zero and overwriting a server-provided resume state.

Once redemption succeeds, `TryApplyEpicResume()` imports the server checkpoint and starts the matching experience. The operation is idempotent.

## Merge rule: local progress never regresses

`ApplyExternalResumeCheckpoint` validates the incoming chapter against the packaged epic catalog.

If the local device already has:

- a completed checkpoint, or
- the same chapter, or
- a later chapter,

local progress wins and no rollback occurs.

Only a strictly newer, valid server checkpoint replaces an older local checkpoint. Imported resume state contains no partial evidence or world-state counters.

## Local-first persistence

Chapter transitions follow this order:

1. build the privacy-minimized `FWMEpicCheckpoint`;
2. update the in-memory checkpoint list;
3. write local Unreal SaveGame;
4. if local persistence fails, roll the in-memory list back;
5. only after the local write succeeds, attempt remote sync.

Remote sync is best-effort. Loss of connectivity must not invalidate local gameplay progress.

## Progress-sync token

The token issued during redemption:

- has scope only `epic-checkpoint:write`;
- is held only in process memory;
- is not written into `UWMEpicJourneySaveGame`;
- expires after a bounded session (8 hours by default);
- is bound to the epic from `epicResume`, when one exists;
- if launch had no epic, becomes bound to the first epic checkpoint written;
- cannot subsequently be used to write a different epic.

## Native checkpoint API minimization

`POST /api/native/epic-checkpoint` accepts only these checkpoint fields:

- `epicId`;
- `chapterId`;
- `chapterIndex`;
- `state` (`in-progress` or `complete`).

Any additional checkpoint field is rejected. In particular, the native client cannot upload raw evidence, free text, answers, hint history, producer identifiers, moral-choice content, personality labels or ideology labels through this route.

The server controls `updatedAt` and preserves the existing aggregate `objectiveSummary`; native checkpoint sync cannot manufacture or expand the parent-facing learning summary.

Unknown progress states fail closed rather than being coerced to `in-progress`.

## Concurrency

The durable profile store retains optimistic revision control. Native checkpoint writes load the latest profile and persist against its current revision. The domain-level monotonic chapter rule prevents stale native sessions from regressing a newer server checkpoint.

## Deployment caveat: ephemeral ticket store

The initial M5.6F `NativeLaunchSessionStore` is process-local memory. That is appropriate for the current single-process source phase, but horizontally scaled production deployments require a shared ephemeral store (for example Redis or another TTL-capable server-side store) or sticky routing for launch redemption and progress-sync authorization.

M5.6F must not be represented as multi-instance production-hardening until that store is externalized.

## Privacy boundary

M5.6F does not widen the child data model. Neither local SaveGame nor native sync persists:

- partial evidence;
- answers or free text;
- hint history;
- moral choices;
- personality or ideology labels;
- raw interaction telemetry;
- Garden session-only mathematics/philosophy mastery gates.

## Certification boundary

Source tests and validators can prove the protocol and minimization contracts are wired. They cannot prove the packaged custom-URI handler, Windows networking, UE HTTP behavior or native SaveGame/runtime integration on a real build while Issue #9 remains unresolved.

Therefore the current honest status is:

**SOURCE CONTRACT READY / NATIVE END-TO-END EVIDENCE BLOCKED**

A native M5.6F certification later requires the locked UE 5.8.2 runner, authored certification map, installed/registered packaged client, real ticket redemption, real epic resume and observed server profile advancement on the same evidence chain.
