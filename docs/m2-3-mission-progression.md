# M2.3 — Mission Progression, Persistence & Learning Journey

## Goal

Turn the M2.2 multi-mission catalog into a deterministic learning journey that persists completed milestones without turning progression into a compulsion mechanic.

## Journey states

Every mission resolves to one of four child-facing states:

- `Locked` — prerequisite missions have not been completed;
- `Available` — prerequisites are satisfied and the mission may be started;
- `Active` — the mission is the current runtime activity;
- `Completed` — the mission has been demonstrated at least once.

Completion never decays. There are no daily streaks, daily reset mechanics or penalties for returning later.

## Data-driven prerequisites

Mission definitions may declare `runtime.prerequisiteMissionIds`. The runtime validates that every prerequisite exists and rejects self-dependencies and dependency cycles. `mission.mathematics.measure-and-build-02` now depends on `mission.mathematics.measure-and-build-01`, proving the unlock path with real packaged content.

## Idempotent rewards

`FWMMissionJourneyModel` maintains two independent stable-ID sets:

- completed missions;
- granted gameplay rewards.

Replaying a completed mission is allowed for practice. `ApplyCompletion` returns only reward IDs not previously granted, so replay cannot duplicate badges/tools or become a farming loop.

## Local persistence

`UWMMissionJourneySaveGame` stores only:

- format version;
- completed MissionIds;
- granted RewardIds;
- last active MissionId.

No child name, email, free text, analytics identifier, commercial transaction, ad identifier or learning-response text is stored. In-progress evidence remains session-local; only completed milestones persist.

The prototype uses the local slot `WM_MissionJourney_Prototype`. A future multi-profile implementation must derive profile-specific storage from a pseudonymous internal profile key rather than PII.

## Startup and mission cycling

At startup the runtime:

1. discovers and validates the mission catalog;
2. loads the journey save when format-compatible;
3. restores completed missions and granted rewards;
4. resumes a previously active unfinished mission when still valid, otherwise selects the first unlocked incomplete mission;
5. falls back to an activatable completed mission only when the current journey has no incomplete unlocked mission.

`CycleMission` now iterates only missions that are unlocked (or already completed and replayable). If the current mission is the only activatable mission, the action is a no-op instead of restarting progress. Locked missions remain visible through the journey read model but cannot be activated.

Saved completion data is sanitized against the current prerequisite graph. A dependent mission is not restored as completed when its required predecessor is absent from the persisted completion set.

## Read model

`FWMJourneyMissionReadModel` exposes stable mission ID, state, progress fraction, learning-objective IDs and prototype status. This supports both a future child mission selector and a backend adapter for the Parent Portal without exposing raw evidence or identity data.

The provider-neutral Parent Progress OpenAPI contract now includes an optional `missionJourney` projection using the same four states.

## Boundaries

- Mission 01 and Mission 02 remain `prototypeOnly=true` while pedagogy/safety review are draft.
- M2.3 does not persist raw learning evidence or partial measurement sessions.
- M2.3 does not introduce XP, streaks, daily rewards, premium currency or monetized progression.
- Issue #9 remains the independent blocker for native UE 5.8.2 compile, Automation execution, authored certification map and manual runtime evidence.
