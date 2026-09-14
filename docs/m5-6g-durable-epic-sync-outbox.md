# M5.6G — Durable Epic Sync Outbox & Conflict Reconciliation

## Objective

M5.6F made native epic progress local-first: a chapter checkpoint had to reach Unreal `SaveGame` before the client attempted the remote write. M5.6G closes the remaining crash/offline window. A checkpoint that has been saved locally but not yet acknowledged by the Player Dashboard is retained as a minimal durable outbox item and retried on a later authorized native session.

This phase does **not** make the network authoritative over the active local runtime. It makes synchronization recoverable, monotonic and idempotent without persisting native credentials or expanding child telemetry.

## Durable local contract

`UWMEpicJourneySaveGame` moves from format version 1 to format version 2 and remains backward compatible with version 1.

Version 2 persists two privacy-minimized arrays:

- `Checkpoints`: the player's latest local chapter checkpoint per epic.
- `PendingSyncCheckpoints`: the latest chapter checkpoint per epic that still requires remote acknowledgement.

The outbox uses the same `FWMEpicCheckpoint` shape: epic ID, chapter ID, chapter index, chapter count and completion state only. It does not persist launch tickets, progress-sync tokens, player identifiers, answers, free text, evidence events, hint history, personality labels or ideology fields.

A version-1 save loads with its existing checkpoints and an empty outbox. New writes use version 2.

## Local-first atomicity

When epic progress reaches a chapter checkpoint during a native launch:

1. the runtime builds the new `FWMEpicCheckpoint`;
2. the local checkpoint is updated;
3. the outbox is coalesced to the newest checkpoint for that epic;
4. checkpoint and outbox are written together through `SaveGameToSlot`;
5. only after that write succeeds may HTTP synchronization be dispatched.

If the `SaveGame` write fails, both in-memory arrays are rolled back. Network state can therefore never be newer than a checkpoint that the local persistence layer failed to commit.

If the process exits, crashes, loses connectivity or receives a non-2xx response after the local write, the outbox remains in `SaveGame`.

## Retry and acknowledgement

A redeemed `worldmakers-launch-v2` session still receives only an ephemeral `epic-checkpoint:write` token in process memory. Credentials are never written to `SaveGame`.

When the launch context resumes an epic, M5.6G retries the pending checkpoint for that same epic with the fresh token. The token remains epic-bound: a launch authorized for one epic cannot be used to drain another epic's outbox entry. Pending entries for other epics remain durable until an appropriate later launch.

A pending outbox item is removed only after a 2xx response confirms the write path. Dispatch failures and non-2xx responses leave it untouched.

## Server conflict reconciliation

`reconcileProfileEpicProgress` makes native retries monotonic and idempotent. The server returns one of four dispositions:

- `created`: no server record existed and the checkpoint was created;
- `advanced`: the incoming checkpoint is genuinely newer and the profile advances;
- `idempotent`: the same chapter/completion state is already stored, so the request succeeds without another profile write;
- `server-ahead`: the server already contains a later chapter or completed epic, so the stale retry succeeds without regressing server state or creating revision churn.

For every successful response the server also projects an `authoritativeCheckpoint` containing only `epicId`, `chapterId`, `chapterIndex` and `state`. Objective summaries and other profile data remain outside that native acknowledgement shape.

## Multiple-device behavior

The monotonic rule is intentionally asymmetric:

- a stale native retry never pulls the server backward;
- a server bootstrap never pulls an equal/newer local checkpoint backward;
- server-equal or server-ahead state settles an old outbox item rather than replaying mutations indefinitely;
- a later launch can import newer server resume state through the existing M5.6F bootstrap path.

This is chapter-level conflict resolution, not real-time multiplayer state replication.

## Privacy boundary

The durable outbox is deliberately not an analytics queue. It may contain only chapter checkpoint fields already permitted by M5.6D. It must never grow to include raw pedagogical evidence, answers, free text, behavior telemetry, moral choices, HMAC secrets, native launch tickets, progress-sync tokens or identity data.

Outbox entries are coalesced by epic, so repeated offline advancement does not create an unbounded local event log.

## Validation

M5.6G source validation requires:

- SaveGame v2 plus version-1 compatibility;
- durable `PendingSyncCheckpoints` round-trip coverage;
- transactional checkpoint/outbox rollback on local save failure;
- latest-per-epic outbox coalescing;
- 2xx-only acknowledgement and retry-on-next-authorized-launch behavior;
- epic-bound token use;
- `created`, `advanced`, `idempotent` and `server-ahead` reconciliation semantics;
- preservation of another active `currentEpicId` when a different epic completes;
- Repository Quality and Unreal project-validation wiring.

The M5.6E native automation filter continues to include `WorldMakers.Epic.Persistence.NativeSaveGameRoundTrip`, now covering the version-2 outbox field as well.

## Certification boundary

Issue #9 still blocks native self-hosted Unreal execution in ordinary CI. M5.6G can therefore establish source contract readiness and Node/server-domain behavior, but it does **not** claim packaged crash/restart, real network interruption, representative-device or native end-to-end certification until the Unreal runner is enabled and the corresponding evidence is captured.
