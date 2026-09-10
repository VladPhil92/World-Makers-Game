# Initial roadmap

## M0 — Repository & production foundation

- Repository architecture, LFS, conventions, security/privacy boundaries.
- Unreal project opens and compiles on a supported workstation.
- Device/performance budget defined for representative tablets.

## M1 — Playable building prototype

- Third-person child avatar placeholder.
- Place, rotate, move, recolor, and remove safe building pieces.
- Save/load one local world.
- No combat or loss loops.

## M2 — Mathematics missions v1

- Mission schema integrated into Unreal runtime/data pipeline.
- Measurement, proportion, spatial reasoning, and resource-budget mission examples.
- Pedagogue review and observable evidence events.

## M3 — Caribbean Rainforest biome vertical slice

Current source progression:

- M3.1 — semantic biome runtime, zones, POIs and exploration core: source-complete.
- M3.2 — deliberate focus/observe interaction system: source-complete.
- M3.3 — multi-evaluator mission runtime + first rainforest science mission: source-complete pending native UE certification.
- M3.4 — reactive ecosystem state and visible consequences: source-complete pending native UE certification.
- M3.5 — spatial ecological building interventions + persistent creative unlocks: source-complete pending native UE certification.
- M3.6 — child journey UX: next.
- M3.7 — tablet performance profiles and capture.
- M3.8 — vertical-slice certification.
- M3.6 — child journey UX: source-complete pending native UE certification.
- M3.7 — tablet performance profiles and capture: source-complete pending representative-device evidence.
- M3.8 — vertical-slice certification infrastructure: source-complete; runtime/device certification blocked by Issue #9 and external evidence.

Target outcomes:

- Stylized premium biome with mobile-scalable lighting/materials.
- Science/ecosystem learning hooks and missions embedded in play.
- Building used as a causal tool for ecological problem solving.
- Creative rewards expand possibilities without manipulative progression.
- Child-facing journey translates trusted technical state into calm, understandable adventures.
- Cultural/environmental provenance review.
- Tablet performance capture.
- Fail-closed certification matrix spanning source, native UE, manual smoke, integrated route, device performance, evidence integrity and privacy.

## M4 — Parent portal

Current source progression:

- M4.1 — parent portal MVP: source-complete with demo-only auth and provider-neutral contracts.
- M4.2 — production identity, durable family linking and backend adapters: next.

M4.1 outcomes:

- Guardian session boundary with production fail-closed behavior.
- Authorized child-profile filtering and cross-account denial.
- Read-only seven-day play-time, recent creations, learning and adventure summaries.
- Link requests that remain pending until backend relationship verification.
- Explicit export, unlink and delete requests with guardian acknowledgement.
- Accessible responsive Family Space UI.
- No direct browser/database coupling, raw child telemetry or browser-exposed secrets.

M4.2 target outcomes:

- Select production identity provider and authenticated server adapter.
- Durable family/guardian/child relationship storage and verification.
- Real data aggregation from authorized game services.
- Durable export/delete/unlink workflows with audit and retention semantics.
- Production localization and deployment topology.

## M5 — Closed multiplayer prototype

- Parent-approved invite/friend relationship.
- Private session join flow.
- Server authorization and abuse-case tests.
- No public stranger discovery or open stranger chat.

## Suggested GitHub issue titles

1. `M1: Implement basic building placement and grid snapping`
2. `M2: Integrate curriculum mission schema and math mission v1`
3. `M3: Build Caribbean Rainforest vertical slice and tablet performance budget`
4. `M4: Implement parent portal read model MVP`
5. `M5: Implement parent-approved private multiplayer invitation flow`
