# M5.6 — Cross-Disciplinary Epic Adventures

## Purpose

M5.6 transforms the reusable learning engines from M5.2–M5.5 into coherent, multidisciplinary world problems. M5.5 proved that each first-class learning stream can drive a fantastic adventure through ordered, authenticated evidence. M5.6 proves that several disciplines can cooperate inside one larger adventure without collapsing into a sequence of disconnected mini-lessons.

The product rule remains: **learning is structurally necessary to play**. Cross-disciplinary does not mean attaching extra questions to an adventure. It means that the world cannot be repaired, understood, navigated or transformed unless the player combines concepts from several streams.

## M5.6A implementation status

**M5.6A — Epic schema and orchestration foundation is source-complete in PR #91.** Native Unreal execution and representative-device certification remain separate evidence boundaries.

Implemented source/runtime capabilities:

- versioned canonical epic catalog plus byte-identical packaged runtime catalog;
- `FWMEpicCatalog`, `FWMEpicDefinition`, ordered chapter definitions and explicit evidence/world-state requirements;
- `FWMEpicProgressModel` with fail-closed chapter ordering;
- exact objective + discipline + primitive + event + producer attribution;
- separate trusted world-state predicates that must be satisfied in addition to pedagogical evidence;
- `UWMEpicRuntimeSubsystem` integration with the existing M5.2/M5.5 Mission Runtime rather than a duplicate learning ledger;
- safe reuse of already-completed M5.5 missions through the existing journey model;
- canonical/package parity validation, privacy/authority validation and Repository Quality gate;
- Unreal automation contracts for breadth, world-state gating, attribution failure cases and deterministic end-to-end traversal of both real epic catalogs;
- packaging of both `WorldMakers/Adventures` and `WorldMakers/Epics` as NonUFS runtime content.

Source validation is green. Native `unreal-build-and-test` remains unavailable until the Windows/Unreal runner described by Issue #9 is provisioned, so M5.6A is **source-complete, not native-certified**.

## Initial epic adventures

### 1. The Eclipse Engine

A celestial machine that regulates light, navigation and energy has lost synchronization. The player must reconstruct the machine rather than simply answer a sequence of questions.

The M5.6A contract currently composes mathematics, physics, English/literature-context and philosophy across four ordered chapters, while the declared epic scope also reserves geometry and ethics as first-class integration targets for the authored vertical slice.

Current chapter contract:

1. `chapter.eclipse.decode-orbit` — mathematical patterns, ratios and constrained construction through The Vault of the Infinite Staircase mission contract;
2. `chapter.eclipse.power-core` — force/motion and electrical relationships through The Moonforge;
3. `chapter.eclipse.recover-language` — contextual English communication and inference through The Dragon Who Lost His Words;
4. `chapter.eclipse.revise-explanation` — argument revision through The Ship That Was Never the Same.

Each chapter also requires a trusted causal world-state predicate before progression can advance.

### 2. The Garden at the End of Winter

A living valley is trapped in a delayed seasonal transition. The player must restore the system by understanding coupled relationships between organisms, matter, ecosystem dynamics and community decisions.

Current chapter contract:

1. `chapter.garden.restore-cellular-balance` — biology/homeostasis through The City Inside a Cell;
2. `chapter.garden.repair-soil-water` — chemistry/solution and conservation reasoning through The Alchemist's Archipelago;
3. `chapter.garden.restore-pollination` — ecology/limiting factors and interdependence through The Forest of a Thousand Voices;
4. `chapter.garden.choose-shared-restoration` — ethical multi-perspective reasoning through The Bridge Between Two Villages.

The declared epic scope also reserves mathematics and philosophy for deeper authored-world integration in M5.6C.

## Epic runtime contract

M5.6 adds an orchestration layer above the M5.5 ordered-adventure runtime rather than replacing it.

An epic contract defines:

- `epicId` — stable epic identifier;
- `chapterId` — ordered chapter identifier;
- `missionId` — an M5.2/M5.5-compatible mission contract associated with the chapter;
- `evidenceRequirements` — explicit objective, discipline, primitive, evidence event, producer identity and bounded count;
- `worldStateRequirements` — stable world-state predicates with trusted producer identity;
- `localizationKeys` — stable child-facing title/prompt keys;
- `privacyModel` — `stable-ids-no-child-free-text`;
- `prototypeOnly` — true until native/runtime/device certification exists.

## Cross-disciplinary design rules

1. **No subject checklist.** A chapter must not exist only to "cover" a discipline. Every integrated stream must contribute to the world problem.
2. **No evidence laundering.** A valid event receives credit only when objective, discipline, primitive, producer kind and producer reference all match the active chapter contract.
3. **No hidden answer key for ethics/philosophy.** Reasoning quality, perspective use and revision remain the assessed behaviors.
4. **Science remains deterministic and bounded.** Epic scripts consume M5.3 simulation results rather than inventing scientific truth.
5. **Narrative provenance remains explicit.** Myth, legend and cultural story are never silently promoted into historical or scientific fact.
6. **Building must be causal.** Construction alters or solves world state, rather than acting as decorative filler.
7. **Failure is informative, not punitive.** A failed model changes feedback/world response and invites revision; it does not introduce loss loops or manipulative pressure.
8. **Calm progression.** No loot boxes, streak pressure, daily obligation, chance rewards or artificial scarcity.

## World-state orchestration

The runtime uses a deterministic ordered state model:

`observe -> model -> test -> construct -> consequence -> revise -> restore`

A chapter advances only when all required pedagogical evidence **and** all required trusted world-state predicates have been committed.

The runtime rejects:

- evidence for a future locked chapter;
- evidence from an untrusted producer;
- a valid event attached to the wrong objective or discipline;
- duplicate credit beyond bounded `requiredCount`;
- spoofed world-state producers;
- direct chapter advancement before requirements are complete;
- client-supplied completion flags as an authority source.

The subsystem activates the next existing mission before committing a chapter transition. If that mission cannot be activated, the chapter remains ready and `AdvanceEpicIfReady()` provides a safe retry point instead of corrupting progress.

## Learning evidence and telemetry

M5.6 preserves minimized telemetry:

- stable epic/chapter/mission/objective IDs;
- bounded completion/progress counters;
- stable evidence event and producer IDs;
- trusted stable world-state IDs;
- no child-authored free text;
- no raw voice transcripts;
- no ideology/personality labels;
- no unnecessary behavioral-history payload.

The future parent-facing telemetry sync should consume summarized objective progress rather than raw child interaction streams.

## Implementation slices

### M5.6A — Epic schema and orchestration foundation — source-complete

- versioned epic content schema;
- canonical + packaged content parity;
- chapter/world-state state machine;
- explicit objective/discipline attribution;
- trusted producer/world-state gates;
- Repository Quality validator;
- Unreal automation contracts, including deterministic traversal of both real epics.

### M5.6B — The Eclipse Engine vertical epic — next

Convert the source-complete orchestration contract into the first coherent authored/playable epic slice:

- authored Eclipse Engine world-space interaction actors and causal world-state producers;
- geometry-specific mirror/alignment gameplay instead of relying only on declared cross-disciplinary scope;
- integration of existing physics, building, language/literature and philosophy producers into one world route;
- chapter-to-chapter child UX, prompts, consequences and feedback;
- deterministic source/runtime integration tests covering the complete route;
- asset slots and fallback/proxy behavior compatible with the existing first-person interaction framework;
- no claim of native visual certification until authored assets and UE/device evidence exist.

### M5.6C — The Garden at the End of Winter vertical epic

- biology/chemistry/ecology simulation coupling;
- mathematical resource/building constraints;
- ethics/philosophy reasoning and provenance-aware cultural narrative;
- deterministic causal ecosystem recovery route;
- end-to-end source/runtime integration tests.

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

## M5.6A source-complete acceptance criteria

M5.6A is source-complete because:

- two epic contracts exist and each declares four or more first-class learning streams;
- every chapter has explicit objective/evidence attribution and trusted producer identity;
- the epic runtime enforces chapter ordering and trusted world-state requirements;
- canonical and packaged epic data are byte-identical;
- both real epics are traversed deterministically end-to-end by Unreal automation source contracts;
- no epic can advance from a client-provided completion flag alone;
- privacy and authority boundaries are validated automatically;
- Repository Quality executes the M5.6A source gate;
- roadmap status distinguishes source completeness from native/runtime/device certification.

## Immediate next implementation target

The next code phase is **M5.6B — The Eclipse Engine vertical epic**. The priority is no longer another schema layer. The next work should create the authored/proxy world interactions and causal producers that make the four Eclipse chapters feel like one continuous game problem, while adding the geometry interaction still missing from the executable evidence route.
