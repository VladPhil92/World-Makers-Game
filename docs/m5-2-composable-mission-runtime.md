# M5.2 — Composable Mission Runtime v2

## Purpose

M5.2 upgrades World Makers from a small set of subject-shaped evaluators into a composable evidence runtime. The goal is not to encode school subjects in C++. The runtime should understand reusable ways of thinking and acting, while mission content maps those actions to mathematics, geometry, languages, literature, biology, chemistry, physics, ecology, ethics and philosophy objectives.

This preserves the product rule established in M5.1: difficult ideas are allowed; interaction and representation are scaffolded instead of lowering the conceptual ceiling.

## Runtime evaluator

M5.2 adds the evaluator:

`composable`

A composable mission declares one or more `evidencePrimitives`. Each requirement contains:

- `primitiveId` — the reusable cognitive/gameplay primitive;
- `evidenceEventId` — stable minimized event ID;
- `objectiveId` — declared learning objective receiving the evidence;
- `requiredCount` — bounded integer from 1 to 20, defaulting to 1.

Every declared mission evidence event must map to exactly one composable requirement. Requirement event IDs are unique, while one primitive can be reused for several objectives or events.

## Evidence primitive allowlist

The initial M5.2 primitive vocabulary is deliberately small:

- `construct-to-constraint`
- `observe-and-classify`
- `predict-test-revise`
- `sequence-and-infer`
- `communicate-in-language`
- `model-system`
- `solve-spatial-system`
- `interpret-text-world`
- `reason-through-dilemma`
- `argue-and-revise`

These are not subjects. Chemistry and physics can both use `predict-test-revise`; geometry and physics can both use `solve-spatial-system`; literature and philosophy can both use `interpret-text-world`; ethics and philosophy can share reasoning primitives while producing different objective evidence.

New primitives require a deliberate schema, runtime, test and pedagogical-contract change. Arbitrary content strings are rejected by the source and C++ allowlists.

## Progress semantics

A composable mission progresses by required evidence units:

`accepted evidence units / total required evidence units`

For example, a requirement with `requiredCount: 2` contributes two units. Evidence beyond the required count is rejected and cannot inflate progress or learning telemetry.

Completion occurs only after every requirement reaches its bounded count. Rewards continue through the existing deterministic Trust Economy journey completion path.

## Runtime API

`UWMMissionRuntimeSubsystem::RecordComposableEvidence` accepts:

- stable `PrimitiveId`;
- stable `EvidenceEventId`;
- optional finite numeric value.

The active mission must explicitly require the exact primitive/event pair. A world system cannot satisfy an event by submitting a different primitive with the same event ID.

The subsystem also exposes read-only checks for required primitive/event pairs, unique required primitive IDs and per-event accepted counts. These are intended for Blueprint/world-system adapters in M5.3 and M5.4.

## Backward compatibility

M5.2 preserves both existing evaluators:

- `measure-and-build`;
- `observe-ecosystem`.

Their evaluator-specific fields remain mutually exclusive with `evidencePrimitives`. Existing mathematics and rainforest missions therefore continue using their established runtime path while future missions can adopt `composable` incrementally.

This phase does not silently reinterpret existing evidence. A later migration can wrap legacy mechanics in primitive adapters only when native behavior and evidence equivalence are proven.

## Multidisciplinary source proof

`content/missions/examples/composable-eclipse-engine-proof.json` demonstrates a single mission definition combining:

- physics through `model-system`;
- geometry through `solve-spatial-system`;
- philosophy through `argue-and-revise`;
- mathematics as a secondary curriculum stream available to later authored objectives.

The fixture is intentionally not copied into `game/Content/WorldMakers/Missions`. It proves the source contract without inserting an unfinished adventure into the playable catalog or startup progression.

## Privacy boundary

Composable learning evidence remains minimized. `FWMLearningEvidenceRecord` stores stable mission, event, objective and primitive IDs, a finite numeric value and a local sequence number. It does not store child-authored prose, chat, voice, names, email addresses or narrative behavioral profiles.

Language, literature, ethics and philosophy systems may need richer transient interaction state in later phases, but durable learning evidence must remain objective-oriented and privacy-minimized. Raw child free text must not be added to this generic evidence record.

## Safety and epistemic quality

The composable runtime measures evidence completion, not ideological conformity or rote recall. `reason-through-dilemma` and `argue-and-revise` should reward coherent reasoning, comparison of reasons, counterexamples and revision rather than a predetermined moral or philosophical answer.

Likewise, `predict-test-revise` exists so science missions can distinguish a genuine inquiry loop from passive observation or an attached quiz.

## Source certification

M5.2 source gates verify:

- the schema keeps legacy evaluators and adds `composable`;
- all ten primitives are synchronized between schema and C++;
- evaluator-specific fields are semantically validated;
- required counts are bounded;
- the multidisciplinary fixture composes at least three primitives;
- the runtime records primitive attribution and completes only after all required units;
- unsupported primitives are rejected;
- existing M2 automation test names remain present;
- generic evidence remains free of child-authored free text.

GitHub source checks cannot prove native Unreal behavior. M5.2 is only runtime-certified after the locked native Unreal Engine 5.8.2 automation path executes the new `WorldMakers.Missions.Composable.*` tests successfully.

## Exit condition

M5.2 is source-complete when a `composable` mission can be parsed, validated, activated and progressed by multiple reusable evidence primitives with deterministic completion, bounded counts, minimized evidence, Blueprint-accessible submission APIs and preserved M2/M3 behavior.

M5.3 then supplies the first major producers of these primitives: matter/chemistry, force/energy/physics, biological systems, plant growth and persistent ecology simulation.
