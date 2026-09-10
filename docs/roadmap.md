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

M4.2 target outcomes:

- Select production identity provider and authenticated server adapter.
- Durable family/guardian/child relationship storage and verification.
- Real data aggregation from authorized game services.
- Durable export/delete/unlink workflows with audit and retention semantics.
- Production localization and deployment topology.

## M5 — Fantastic Learning Universe

Purpose: transform the existing mission foundation into a multidisciplinary adventure system with a high conceptual ceiling and low interaction friction.

Current source progression:

- M5.1 — curriculum architecture, 11 first-class learning streams and fantastic mission design contract: source-complete in PR #47.
- M5.2 — composable Mission Runtime v2: implemented in source with schema, generic evidence API, multidisciplinary fixture, Unreal automation tests and Repository Quality gate; native Unreal execution remains required for runtime certification.
- M5.3 — Science Simulation Core: next.
- M5.4 — Language, Literature and Thought Runtime: pending.
- M5.5 — First Fantastic Adventure Pack: pending.
- M5.6 — Cross-disciplinary epic adventures: pending.

Core learning streams:

1. Mathematics
2. Geometry
3. English language
4. Spanish language
5. Literature
6. Biology
7. Chemistry
8. Physics
9. Ecology
10. Ethics
11. Philosophy for children

History and culture remain cross-curricular context layers.

### M5.1 — Curriculum and mission contract

- Core curriculum concept map.
- Mission metadata for primary/secondary disciplines, concept tags and layered depth.
- Experience → Concept → Formalization progression.
- Fantastic mission design catalog.
- Pedagogy rule: learning must be structurally necessary to play, not attached as a worksheet.

### M5.2 — Composable Mission Runtime v2

Reusable evidence primitives:

- construct-to-constraint;
- observe-and-classify;
- predict-test-revise;
- sequence-and-infer;
- communicate-in-language;
- model-system;
- solve-spatial-system;
- interpret-text-world;
- reason-through-dilemma;
- argue-and-revise.

Implementation status:

- `composable` evaluator added without removing legacy M2/M3 evaluators;
- each requirement maps a primitive, stable evidence event, objective and bounded `requiredCount`;
- progress is calculated across required evidence units;
- exact primitive/event pairs are enforced;
- unsupported primitives and over-count evidence are rejected;
- minimized evidence records include primitive attribution but no child-authored free text;
- Blueprint-facing subsystem APIs expose evidence submission and requirement introspection;
- source fixture demonstrates physics + geometry + philosophy composition without entering the packaged playable catalog.

Target: one mission may compose several primitives and produce evidence for multiple learning objectives without creating a separate hard-coded runtime for every subject.

See `docs/m5-2-composable-mission-runtime.md`.

### M5.3 — Science Simulation Core

- Matter/substance identity and material properties.
- Temperature and state transitions.
- Mixtures, solutions and separation.
- Safe virtual chemical reaction model and reaction evidence.
- Force/motion/energy primitives.
- Basic electricity/circuit model.
- Biological system state and lifecycle primitives.
- Plant growth/germination/light/water/nutrient model.
- Persistent ecosystem coupling between organisms, water, soil, resources and player construction.

### M5.4 — Language, Literature and Thought Runtime

- Contextual English/Spanish communication actions.
- Reading/listening/inference evidence.
- Myth/legend narrative graph with provenance metadata.
- Point-of-view and interpretation mechanics.
- Ethics dilemma model with consequences and reason comparison.
- Philosophy dialogue model that evaluates argument structure, contradiction, assumptions, counterexamples and revision rather than ideological conformity.

### M5.5 — First Fantastic Adventure Pack

Initial production candidates:

- The Vault of the Infinite Staircase — mathematics.
- Architects of the Impossible City — geometry.
- The Dragon Who Lost His Words — English.
- La Biblioteca de las Palabras Perdidas — Spanish.
- The Labyrinth of Stories — literature / myths and legends.
- The City Inside a Cell — biology.
- The Alchemist's Archipelago — chemistry.
- The Moonforge — physics.
- The Forest of a Thousand Voices — ecology.
- The Bridge Between Two Villages — ethics.
- The Ship That Was Never the Same — philosophy for children.

### M5.6 — Cross-disciplinary epic adventures

- Missions combine multiple learning streams when the world problem naturally requires them.
- Candidate: The Eclipse Engine — geometry + physics + mathematics + language/literature + philosophy.
- Candidate: The Garden at the End of Winter — biology + chemistry + ecology + mathematics + ethics + mythology.
- Evidence attribution remains explicit per objective even when one action supports several disciplines.

### M5 exit condition

M5 is not complete when the repository merely contains educational text. It is complete when representative adventures can be played end-to-end and successful completion requires authentic use of the targeted ideas, with source tests, native Unreal tests, pedagogical review and minimized learning evidence.

See `docs/fantastic-learning-universe.md` and `content/learning-objectives/core-curriculum-v1.json`.

## M6 — Closed multiplayer prototype

- Parent-approved invite/friend relationship.
- Private session join flow.
- Server authorization and abuse-case tests.
- No public stranger discovery or open stranger chat.

## Suggested GitHub issue titles

1. `M5.3: Implement science simulation core for chemistry, physics, biology and ecology`
2. `M5.4: Implement bilingual language, literature, ethics and philosophy mission runtime`
3. `M5.5: Build first fantastic multidisciplinary adventure pack`
4. `M5.6: Build cross-disciplinary epic adventure framework`
5. `M6: Implement parent-approved private multiplayer invitation flow`
