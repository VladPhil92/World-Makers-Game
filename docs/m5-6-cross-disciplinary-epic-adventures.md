# M5.6 — Cross-Disciplinary Epic Adventures

## Purpose

M5.6 is the next gameplay-content phase after M5.5. M5.5 proved that each first-class learning stream can drive a fantastic adventure through ordered, authenticated evidence. M5.6 now proves that several disciplines can cooperate inside one coherent world problem without collapsing into a sequence of disconnected mini-lessons.

The product rule is unchanged: **learning must be structurally necessary to play**. Cross-disciplinary does not mean attaching extra questions to an adventure. It means that the world state cannot be repaired, understood, navigated or transformed unless the player combines concepts from several streams.

## Why this phase is next

The repository already contains:

- M5.2 composable evidence primitives and objective attribution;
- M5.3 deterministic science simulation primitives;
- M5.4 language, literature, ethics and philosophy reasoning systems;
- M5.5 ordered adventure progression with stable producer identity;
- first-person interaction, building and world-interaction foundations;
- persistent player identity and a live web playtest path.

The missing layer is an **epic orchestration model** that coordinates multiple missions, producers and world-state consequences in one larger adventure while retaining exact evidence attribution per objective.

## Initial epic adventures

### 1. The Eclipse Engine

A celestial machine that regulates light, navigation and energy has lost synchronization. The player must reconstruct the machine rather than simply answer a sequence of questions.

Required learning streams:

- mathematics — ratio, sequence, proportional reasoning and quantitative constraints;
- geometry — angles, spatial alignment, symmetry and coordinate relationships;
- physics — force, motion, energy and electrical relationships;
- English or Spanish language — contextual communication needed to recover operating knowledge;
- literature — inference from fragmented records and symbolic clues;
- philosophy for children — revise an explanation when new evidence challenges the first model.

Representative gameplay chain:

1. observe the failed celestial mechanism and infer a repeating timing pattern;
2. align mirror or lens geometry to restore a valid optical path;
3. balance energy requirements through a deterministic physics/circuit model;
4. recover missing operational meaning through contextual language interaction;
5. interpret a fragment whose symbolic meaning changes which subsystem should be activated;
6. construct the final alignment under spatial constraints;
7. defend and revise the player's causal explanation after the engine behaves differently from the initial prediction.

### 2. The Garden at the End of Winter

A living valley is trapped in a delayed seasonal transition. The player must restore the system by understanding the coupled relationships between organisms, matter, climate and community decisions.

Required learning streams:

- biology — organism systems, transport and life-cycle constraints;
- chemistry — matter, solution and reaction/conservation relationships;
- ecology — limiting factors, pollination, interdependence and ecosystem state;
- mathematics — resource and proportional constraints;
- ethics — reason across affected perspectives and tradeoffs;
- mythology/literature — interpret the cultural story embedded in the landscape without treating it as scientific evidence.

Representative gameplay chain:

1. diagnose plant and ecosystem limiting factors;
2. test a bounded virtual chemistry relationship affecting soil/water state;
3. restore habitat or infrastructure through building constraints;
4. model resource allocation quantitatively;
5. evaluate a community dilemma in which every option has tradeoffs;
6. interpret a mythic narrative as cultural context, keeping provenance distinct from scientific evidence;
7. observe ecosystem recovery and explain which causal links actually changed the world.

## Epic runtime contract

M5.6 should introduce an orchestration layer above the M5.5 ordered-adventure runtime rather than replace it.

An epic contract should define:

- `epicId` — stable epic identifier;
- `chapterId` — ordered chapter identifier;
- `missionId` — one or more M5.2/M5.5-compatible mission contracts associated with the chapter;
- `requiredObjectives` — explicit objective IDs and required evidence counts;
- `producerKind` and `producerRefId` — trusted producer identity;
- `worldStateRequirement` — stable world-state predicate that must be satisfied where appropriate;
- `unlocks` — deterministic next chapter/world-state transitions;
- `disciplineAttribution` — exact mapping from accepted evidence to learning stream/objective;
- `localizationKeys` — child-facing prompt, feedback and formalization keys;
- `privacyModel` — `stable-ids-no-child-free-text`;
- `prototypeOnly` — true until native/runtime/device certification exists.

## Cross-disciplinary design rules

1. **No subject checklist.** A chapter must not exist only to "cover" a discipline. Every learning stream must contribute to the world problem.
2. **No evidence laundering.** One action may support several objectives only when each objective has an explicit evidence mapping.
3. **No hidden answer key for ethics/philosophy.** Reasoning quality, perspective use and revision remain the assessed behaviors.
4. **Science remains deterministic and bounded.** Epic scripts consume M5.3 simulation results rather than inventing new scientific truth.
5. **Narrative provenance remains explicit.** Myth, legend and cultural story are never silently promoted into historical or scientific fact.
6. **Building must be causal.** Construction is used to alter or solve world state, not as decorative filler.
7. **Failure is informative, not punitive.** A failed model changes feedback/world response and invites revision; it does not introduce loss loops or manipulative pressure.
8. **Calm progression.** No loot boxes, streak pressure, daily obligation, chance rewards or artificial scarcity.

## World-state orchestration

M5.6 should formalize a small, deterministic world-state graph for each epic. Chapters transition only when both pedagogical evidence and the corresponding trusted world-state condition are satisfied.

Example:

`observe -> model -> test -> construct -> consequence -> revise -> restore`

The runtime must reject:

- evidence for a future locked chapter;
- evidence from an untrusted producer;
- a valid evidence event attached to the wrong objective;
- world-state transitions that lack the required learning evidence;
- duplicate credit beyond bounded `requiredCount`;
- client-supplied completion flags that bypass authoritative runtime checks.

## Learning evidence and telemetry

M5.6 preserves minimized telemetry:

- stable epic/chapter/mission/objective IDs;
- bounded completion/progress counters;
- stable evidence event and producer IDs;
- optional deterministic attempt/result state where needed for pedagogy;
- no child-authored free text;
- no raw voice transcripts;
- no ideology/personality labels;
- no unnecessary behavioral-history payload.

The future parent-facing telemetry sync should consume summarized objective progress rather than raw child interaction streams.

## Implementation slices

### M5.6A — Epic schema and orchestration foundation

- versioned epic content schema;
- canonical + packaged content parity;
- chapter/world-state state machine;
- explicit multi-objective discipline attribution;
- trusted producer/world-state gates;
- repository validator and Unreal automation tests.

### M5.6B — The Eclipse Engine vertical epic

- production contract for all chapters;
- reusable interactions mapped to mathematics, geometry, physics, language/literature and philosophy;
- deterministic world-state consequences;
- end-to-end source/runtime test path.

### M5.6C — The Garden at the End of Winter vertical epic

- biology/chemistry/ecology simulation coupling;
- resource/building constraints;
- ethics reasoning and provenance-aware cultural narrative;
- end-to-end source/runtime test path.

### M5.6D — Epic UX and persistence

- chapter resume/continue state;
- player-dashboard reflection of unlocked/completed epic chapters;
- calm child-facing progress language;
- parent-facing summarized learning evidence contract.

### M5.6E — Certification

- native Unreal execution;
- authored map/asset integration;
- pedagogical review;
- cultural/provenance review where applicable;
- representative tablet performance evidence;
- fail-closed certification status distinct from source completeness.

## Source-complete acceptance criteria

M5.6 is source-complete when:

- at least two epic contracts exist and each naturally combines four or more learning streams;
- every chapter has explicit objective/evidence attribution;
- the epic runtime enforces chapter ordering, producer identity and world-state requirements;
- canonical and packaged epic data are semantically identical;
- both epics can be traversed end-to-end in automated source/runtime tests using deterministic producers;
- no epic can complete from client-provided completion flags alone;
- privacy and calm-progression boundaries are validated automatically;
- Repository Quality executes the M5.6 source gate;
- roadmap status distinguishes source completeness from native/runtime/device certification.

## Immediate next implementation target

The next code slice is **M5.6A — Epic schema and orchestration foundation**. Do not begin by producing more standalone mission JSON files. First establish the cross-disciplinary epic contract and authoritative orchestration state machine so that The Eclipse Engine and The Garden at the End of Winter become coherent playable systems rather than loose collections of subject activities.
