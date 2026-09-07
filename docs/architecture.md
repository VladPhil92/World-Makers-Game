# Architecture

## Decision: modular monorepo

World Makers starts as a monorepo because the expected 3–8 person team benefits from atomic changes across gameplay, curriculum, parent-facing reporting, and API contracts. A multi-repo split would add version coordination, permissions, release choreography, and discovery overhead before team boundaries justify it.

Deployable units remain isolated (`game`, `apps/parent-portal`, `services/backend`) so they can be extracted later with Git history if ownership, security, or release cadence demands it.

## Backend decision

No provider is selected yet. Keep domain contracts provider-neutral and implement adapters later.

Candidates:

- PlayFab: game-centric identity, multiplayer/lobbies, telemetry; faster game-service adoption but vendor coupling.
- Epic Online Services / Unreal Online Subsystem: useful cross-platform session/social abstractions; does not replace every application/backend need.
- Firebase: rapid auth/data/product analytics; must be designed carefully for child-data minimization and server-authoritative multiplayer.
- AWS/GameLift: strong scalable session hosting; operationally heavier for a small early-stage team.
- Custom backend: maximum control and portability; highest engineering/operations burden.

For the first closed multiplayer prototype, prefer Unreal Online Subsystem abstractions plus a thin provider adapter. Select the concrete provider only after defining session topology, authoritative state, parent-managed friend approval, regions, concurrency, budget, and console requirements.

## Data boundaries

1. **Identity/consent private zone** — parent PII, child-parent relationship, consent evidence, account recovery.
2. **Gameplay zone** — pseudonymous profile ID, inventory/build state, mission state, approved-session permissions.
3. **Learning/progress zone** — objective IDs, evidence events, mastery/progress summaries keyed by pseudonymous profile ID.
4. **Analytics zone** — minimized events; no raw child PII.
5. **Parent read model** — server-produced summaries exposed only to the authorized parent/guardian account.

No client may infer authorization solely from a child/profile identifier. Relationship and invitation permissions are server-side decisions.

## Unreal module direction

Start with one native module and split only when boundaries become real. Likely future modules: `WorldMakersCore`, `Building`, `Missions`, `Exploration`, `Customization`, `Online`, `Learning`, and `UI`.

## Rendering/scalability

Treat Nanite and Lumen as quality-tier capabilities, not unconditional requirements. Maintain scalable materials, lighting, geometry LOD/HLOD, texture pools, effects, shadows, and resolution policies for mid-range tablets. Device profiling determines defaults.
