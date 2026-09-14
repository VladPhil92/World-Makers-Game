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
- M4.2 — production identity, durable family linking and backend adapters: source-complete for the first web playtest slice; deployed for live testing.

M4.2 implementation status:

- Guardians register and sign in with real accounts (Supabase Auth, email + password) instead of the demo-only session.
- Family/guardian/child relationships are durable, in a dedicated Supabase Postgres project (`World Makers Game`) kept separate from the CTG One production/financial database by design — that project's own `worldmakers_interest_profiles` table already documents that child identity and gameplay data must stay out of it.
- Row Level Security enforces that a guardian only ever sees their own family and linked children; the game's player-profile store is reached only through two narrowly-scoped `SECURITY DEFINER` RPCs, so no service-role key is needed anywhere in either app.
- Guardians create a child's player profile from the portal and hand off into `player-dashboard` through the existing `ctg-one-identity-v1` assertion protocol (parent-portal now issues assertions; player-dashboard already verified them) via a same-origin `/handoff` page — no new cross-origin trust surface.
- `player-dashboard`'s persistent profile store (D2) now has a Supabase-backed implementation alongside the original local `JsonFileProfileStore`, so profiles survive redeploys once `SUPABASE_URL`/`SUPABASE_ANON_KEY` are configured.
- Both apps are deployed on Railway for the first live web playtest.
- Custom SMTP (Resend, on a dedicated `mail.ctgone.com` sending subdomain) is configured for the Supabase project, so guardian confirmation emails deliver reliably instead of hitting the default mailer's very low rate limit.
- Privacy requests are real, not a stub: `export-child-data` returns a downloadable bundle of the child's profile, dashboard stats and player-dashboard profile; `delete-child-data` actually deletes the child's rows (family membership, dashboard stats, and their player-dashboard profile via `wm_delete_player_profile`), verified end to end against the live database. `unlink-child-profile` is explicitly blocked (409, `co_parent_support_required`) rather than faked, since removing one guardian's access while preserving a child's data for another guardian is meaningless until family membership supports more than one guardian.

M4.2 remaining outcomes:

- Real gameplay-telemetry sync into `child_dashboard_stats` (currently zeroed on child creation; the game/runtime side of that pipeline is separate future work).
- Co-parent invite flow (today a guardian can only self-link the family they created at signup) — this also unblocks `unlink-child-profile`.
- Audit/retention logging for privacy requests (today they execute immediately with no durable request record).
- Production localization.

## M5 — Fantastic Learning Universe

Purpose: transform the existing mission foundation into a multidisciplinary adventure system with a high conceptual ceiling and low interaction friction.

Current source progression:

- M5.1 — curriculum architecture, 11 first-class learning streams and fantastic mission design contract: source-complete in PR #47.
- M5.2 — composable Mission Runtime v2: source-complete with schema, generic evidence API, multidisciplinary fixture, Unreal automation tests and Repository Quality gate; native Unreal execution remains required for runtime certification.
- M5.3 — Science Simulation Core: source-complete with data-driven matter/reaction catalog, solubility/filtration, stoichiometry and conservation, mechanics/energy, ideal DC circuits, cellular systems, plant lifecycle and ecology coupling; native Unreal execution remains required for runtime certification.
- M5.4 — Language, Literature and Thought Runtime: source-complete with bilingual contextual communication, provenance-aware narrative graphs, neutral ethical reasoning, philosophy argument revision, M5.2 evidence integration and privacy-minimized stable-ID inputs; native Unreal execution remains required for runtime certification.
- M5.5 — First Fantastic Adventure Pack: source-complete in PR #48 with 11 adventures, 22 ordered beats and authenticated producer/evidence routing; final 3D production and native/device certification remain pending.
- M5.6A — Epic schema and authoritative cross-disciplinary orchestration: source-complete in PR #91 with two real epic contracts, exact objective/discipline/producer attribution, trusted world-state gates, end-to-end deterministic catalog tests and Repository Quality validation; native UE certification remains blocked by Issue #9.
- M5.6B — The Eclipse Engine vertical epic: next.

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

Implementation status:

- substance identity, molar mass and simplified phase transitions;
- water solubility, saturation and conceptual filtration boundaries;
- balanced reaction definitions, limiting-reagent execution and mass-conservation checks;
- deterministic force/motion, momentum and kinetic-energy primitives;
- ideal DC circuit relationships using Ohm's law and electrical power;
- normalized cellular-system state covering membrane, energy, transport, information and waste;
- plant lifecycle from seed through fruiting with water/light/nutrient/temperature/pollinator constraints;
- bounded plant-to-ecosystem state coupling;
- canonical and staged runtime science catalog parity;
- virtual-only chemistry content boundary with no real-world procedural instructions;
- Repository Quality gate and Unreal Automation coverage.

See `docs/m5-3-science-simulation-core.md`.

### M5.4 — Language, Literature and Thought Runtime

Implementation status:

- contextual communication challenges for both English and Spanish;
- language success requires target language + semantic intent + acceptable register, not token matching alone;
- myth/legend narrative graph with explicit `retelling` / `adaptation` / `historical-source` classification;
- provenance key and cultural-review state required for narrative content;
- narrative nodes carry localization passage keys and point-of-view IDs;
- inference choices are graph edges that generate `interpret-text-world` evidence;
- ethical dilemmas expose reasons, multiple affected perspectives, tradeoffs and normalized consequence profiles;
- ethical evaluation measures reasoning quality without a hidden morally correct option;
- philosophy problems support alternative claims, reason links, assumptions, counterexamples and mandatory revision;
- argument evidence requires a selected reason to actually target the selected claim;
- `UWMLanguageThoughtSubsystem` submits valid language/literature/ethics/philosophy outcomes into M5.2 `RecordComposableEvidence`;
- runtime inputs use `stable-ids-no-child-free-text` and do not persist child-authored free text or ideological/personality labels;
- canonical/staged thought catalog parity, Repository Quality gate and Unreal Automation coverage are wired.

See `docs/m5-4-language-literature-thought-runtime.md`.

### M5.5 — First Fantastic Adventure Pack

Source-complete in PR #48. The first pack contains one primary adventure for each of the 11 first-class learning streams, with 22 ordered beats and stable producer/evidence gates layered over the composable mission runtime.

Implemented adventures:

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

Repository Quality executes `scripts/validate-m5-5-fantastic-adventure-pack.py`. Source completeness does not claim final 3D environments, authored apparatus, final animation/cinematics, localization/audio or representative-device runtime certification.

See `docs/m5-5-first-fantastic-adventure-pack.md`.

### M5.6 — Cross-disciplinary epic adventures

#### M5.6A — Epic schema and orchestration foundation — source-complete

Implemented in PR #91:

- versioned `epic-catalog.cross-disciplinary-v1` with canonical + packaged parity;
- authoritative ordered chapter state machine;
- exact objective, discipline, primitive, event and producer attribution;
- trusted causal `worldStateRequirements` in addition to learning evidence;
- `UWMEpicRuntimeSubsystem` integration with the existing Mission Runtime;
- fail-closed rejection of future evidence, spoofed producers, wrong attribution and premature advancement;
- deterministic end-to-end automation traversal of The Eclipse Engine and The Garden at the End of Winter catalogs;
- source validator wired into Repository Quality;
- Adventures and Epics staged for packaged builds.

This status is source-complete only. Native Unreal build/test and representative-device evidence still require the external runner and authored-runtime evidence tracked by Issue #9.

#### M5.6B — The Eclipse Engine vertical epic — next

The next phase converts the orchestration contract into a coherent playable world route:

- authored/proxy Eclipse world interactions and causal state producers;
- explicit geometry mirror/alignment gameplay;
- integrated mathematics, physics, building, language/literature and philosophy interactions;
- chapter transition UX and world consequences;
- end-to-end integration coverage using the real runtime producers;
- first-person interaction compatibility and asset-slot fallbacks.

#### M5.6C — The Garden at the End of Winter vertical epic — pending

- biology/chemistry/ecology simulation coupling;
- mathematical resource/building constraints;
- ethics/philosophy reasoning and provenance-aware cultural narrative;
- deterministic causal ecosystem-recovery route.

#### M5.6D — Epic UX and persistence — pending

- chapter resume/continue persistence;
- player-dashboard epic state;
- calm child-facing progress language;
- parent-facing summarized objective progress.

#### M5.6E — Epic certification — pending external/native evidence

- native Unreal execution;
- authored world/map integration;
- pedagogical and cultural/provenance review;
- representative-device performance capture;
- fail-closed certification distinct from source completeness.

See `docs/m5-6-cross-disciplinary-epic-adventures.md`.

### M5 exit condition

M5 is not complete when the repository merely contains educational text. It is complete when representative adventures can be played end-to-end and successful completion requires authentic use of the targeted ideas, with source tests, native Unreal tests, pedagogical review and minimized learning evidence.

See `docs/fantastic-learning-universe.md` and `content/learning-objectives/core-curriculum-v1.json`.

## Visual production continuation — reference-driven first-person interaction

Current progression:

- Reference-Driven Visual & Animation Polish — merged in PR #75; source policy now formalizes eco-futurist composition, contextual HUD density, first-person presentation profiles, character motion personalities and Reduced Motion safeguards.
- First-Person Interaction Kit v1 — source-proxy implementation complete on `feat/first-person-interaction-kit-v1`: camera override, local owner-only hand/tool/wrist proxies, deterministic nine-action interaction vocabulary, semantic event routing, contextual UMG card, Reduced Motion handling, authored-asset slots, automation tests and Repository Quality gate.
- First-Person Authored Asset Production & Native Integration — next: replace source proxies with authored arms/tools/animation assets while preserving stable action IDs and rollback, then validate natively in Unreal and on representative tablets.

The source-proxy phase is not production art certification. Final skeletal arms, scanner/build/measure/wrist assets, first-person animation clips, skinning, animation blueprint behavior, camera comfort and tablet performance remain explicit native/manual evidence boundaries.

See `docs/reference-driven-visual-animation-polish.md` and `docs/first-person-interaction-kit-v1.md`.

## M6 — Closed multiplayer prototype

- Parent-approved invite/friend relationship.
- Private session join flow.
- Server authorization and abuse-case tests.
- No public stranger discovery or open stranger chat.

## Suggested GitHub issue titles

1. `M5.6B: Build The Eclipse Engine vertical epic`
2. `M5.6C: Build The Garden at the End of Winter vertical epic`
3. `M5.6D: Persist epic progress and expose calm player/parent UX`
4. `M6: Implement parent-approved private multiplayer invitation flow`
