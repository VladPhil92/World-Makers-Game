# M5.5 — First Fantastic Adventure Pack

## Purpose

M5.5 converts the reusable learning engines from M5.2, M5.3 and M5.4 into a first coherent set of fantastic adventure contracts. The pack contains **11 adventures** — one primary adventure for each first-class learning stream — and **22 ordered beats**.

The product rule remains: learning is structurally necessary to play. A beat is not complete because a player opened a lesson screen; it completes only when a trusted world, building, science or thought producer emits the evidence required by the active act.

## Ordered Adventure Runtime

M5.2 intentionally allowed composable evidence to arrive independently. M5.5 adds narrative sequencing above it.

Each beat declares:

- a stable `beatId`;
- an M5.2 `primitiveId`;
- an `evidenceEventId`;
- a trusted `producerKind` (`building`, `science`, `thought` or `world`);
- a stable `producerRefId`;
- a localized prompt key;
- a formalization key;
- a bounded `requiredCount`.

`FWMAdventureProgressModel` accepts evidence only for the current beat and only when producer, producer reference, primitive and event all match. Future-beat evidence and spoofed producers fail closed. Once accepted, the evidence is recorded through `UWMMissionRuntimeSubsystem::RecordComposableEvidence`, so M5.2 remains the authoritative learning-progress and reward ledger.

The Adventure layer therefore owns story order; the Mission layer owns pedagogical evidence/completion.

## First pack

| Adventure | Primary stream | Core gameplay idea |
| --- | --- | --- |
| The Vault of the Infinite Staircase | Mathematics | Infer a changing numerical rule, model ratios, then construct an optimized route. |
| Architects of the Impossible City | Geometry | Restore symmetry, solve angle/scale constraints and justify spatial stability. |
| The Dragon Who Lost His Words | English | Use English to restore a magical machine, then infer meaning from contextual fragments. |
| La Biblioteca de las Palabras Perdidas | Spanish | Request information precisely and make figurative meaning alter the living library. |
| The Labyrinth of Stories | Literature | Navigate a mythic retelling through inference, point of view and symbolism. |
| The City Inside a Cell | Biology | Restore a cellular system and reason about transport/homeostasis. |
| The Alchemist's Archipelago | Chemistry | Predict/test solution saturation, then model a mass-conserving transformation. |
| The Moonforge | Physics | Predict motion under force, then power the forge through circuit relationships. |
| The Forest of a Thousand Voices | Ecology | Diagnose plant limiting factors and restore pollination/interdependence. |
| The Bridge Between Two Villages | Ethics | Make a resource decision and justify it through multiple perspectives and tradeoffs. |
| The Ship That Was Never the Same | Philosophy for children | Defend a position on identity, confront a counterexample and revise the argument. |

History and culture remain a cross-curricular context layer. For example, The Labyrinth of Stories retains provenance and cultural-review metadata without converting history-culture into a twelfth primary learning stream.

## M5.3 science boundary

Science beats consume or are designed to consume deterministic results from M5.3 rather than inventing scientific truth inside mission scripts. Chemistry keeps the `virtual-only-no-real-world-procedure` safety boundary. Biology uses normalized gameplay proxies only where the simulation explicitly labels them as such.

M5.5 does not claim that every final 3D science apparatus, environment animation or interaction actor is complete. It establishes the ordered, evidence-authenticated route those actors must drive.

## M5.4 language, literature and thought boundary

M5.4 outcomes now route through the active Adventure Runtime when an adventure is active. Communication challenge IDs, story IDs, ethical dilemma IDs and philosophy problem IDs become producer identities.

The pack adds a dedicated English challenge for The Dragon Who Lost His Words. The existing Spanish library challenge, Minotaur narrative graph, Bridge Between Two Villages dilemma and Ship of Theseus philosophy problem are reused directly.

Ethics and philosophy remain reasoning systems, not ideological answer keys. Narrative material retains provenance/source-class/cultural-review metadata.

## Privacy and progression

The pack keeps `stable-ids-no-child-free-text`. Adventure state is stable IDs plus bounded counters; no child-authored free text, voice transcript, ideology label or personality score is required by M5.5.

Rewards are completion/unlock IDs only. The pack does not introduce loot boxes, streak pressure, daily obligations, chance rewards or limited-time scarcity.

## Source-complete definition

M5.5 is **source-complete** when:

- all 11 streams have a first fantastic adventure contract;
- canonical and packaged pack data match;
- every adventure maps 1:1 and in order to a valid composable mission;
- the ordered producer/evidence gate is implemented and automation-tested;
- M5.4 thought producers route through that gate without breaking non-adventure M5.2 behavior;
- packaged builds stage the Adventure catalog;
- Repository Quality validates breadth, parity, ordering, privacy and reward boundaries;
- Unreal CI remains green on the implementation head.

Source-complete explicitly means **not final 3D production**. Final environments, animations, cinematics, localized prose/audio, UX polish and representative-device play evidence remain production/certification work.

## Next phase

M5.6 builds cross-disciplinary epic adventures in which multiple streams must cooperate naturally inside the same world problem. Initial targets remain The Eclipse Engine and The Garden at the End of Winter.
