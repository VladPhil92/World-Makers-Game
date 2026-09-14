# M5.6B — The Eclipse Engine: game-first vertical epic

## Product intent

The Eclipse Engine is the first World Makers epic designed explicitly to feel like an adventure game rather than an educational platform. The player is not asked to study mathematics, geometry, physics, language, literature or philosophy as named subjects. The player is asked to repair an impossible celestial machine whose behavior can only be understood through observation, experimentation, construction, interpretation and revision.

The governing rule is **world first, learning underneath**. Learning evidence remains exact and auditable inside the runtime, but the player-facing layer exposes mysteries, artefacts, mechanisms, consequences and choices instead of curriculum labels.

## Core game loop

The reusable loop is:

**Discover → Manipulate → Predict → Test → Observe visible consequence → Adapt → Stabilize**

The loop is intentionally different from quiz logic. A player should normally encounter the phenomenon before formal language is offered. The system teaches through affordance and consequence: the world behaves differently when the player changes a meaningful variable.

The player may fail, but failure is **reversible**. A failed mirror angle sends light somewhere else; an unstable model produces an unstable mechanism; a poor route wastes motion. Nothing is framed as a wrong school answer, and no attempt removes progress, currency or status.

## Intrinsic gamification

M5.6B rejects points-first educational gamification. The primary rewards are intrinsic:

- curiosity: a machine or place contains a mystery worth understanding;
- agency: the player can directly manipulate meaningful systems;
- mastery: a mechanism that initially appears opaque becomes predictable;
- transformation: successful reasoning visibly changes the world;
- capability: restored systems open new paths, tools, information or interactions;
- authorship: construction and experimentation allow more than one viable intermediate approach.

There are no required streaks, stars, grades, score multipliers, loot boxes, daily obligations or chance rewards. The game may later celebrate milestones, but celebration must represent what changed in the world rather than substitute points for understanding.

## No school UI

The main adventure HUD must not expose subject names, learning objectives, grades, quizzes or correctness labels. The immediate language should be diegetic: "Wake the orbit ring", "Bend the starlight", "Reignite the Moonforge", "Teach the archive to listen", "Find the route hidden in the story", and "Understand why the engine still resists".

Formalization may exist later in an optional codex, reflection view or parent-facing summary. It should never interrupt the core play loop with a worksheet-like modal.

## Six-act player journey

### Act I — Wake the orbit ring

The celestial ring drifts out of rhythm. The player watches its pulse, discovers a repeated pattern, tunes coupled cycles and constructs an efficient route that carries the motion. Ratios and pattern inference are used because the machine cannot be stabilized without them.

World reward: the first orbit locks into rhythm and previously dark machinery begins moving.

### Act II — Bend the starlight

The recovered orbit reveals a broken mirror lattice. The player rotates mirrored elements, aligns a light bridge and tests whether the path remains stable under perturbation. Symmetry, angles and spatial proof are mechanics rather than labels.

World reward: a coherent beam crosses the structure and illuminates the next region.

### Act III — Reignite the Moonforge

The engine has a power source but cannot sustain motion. The player predicts how a counterweight will change a moving assembly, tests the result, and routes a bounded circuit until the core becomes stable.

World reward: the Moonforge wakes with visible, audible and spatial feedback and powers the archive.

### Act IV — Teach the archive to listen

The archive is active but no longer understands the operating language. The player uses context to recover a command and infer missing meaning from surrounding clues.

World reward: the archive responds and physically unfolds new information surfaces.

### Act V — Find the route hidden in the story

The archive does not contain an explicit map. It contains a story whose point of view, repeated images and symbols encode spatial information. The player interprets the narrative because that interpretation reveals a traversable route.

World reward: apparently decorative symbols reorganize into a usable map.

### Act VI — Understand why the engine still resists

Even after the subsystems are restored, the player’s first explanation of the machine is incomplete. A counterexample forces the player to identify an assumption and revise the causal model.

World reward: the entire Eclipse Engine synchronizes and the environment changes state. The final success is not a score screen; it is the world itself behaving differently.

## Progressive hints without punishment

Hints are player-requested. The first hint should redirect attention to a useful environmental feature, the second should expose a relationship, and the third may make the underlying structure more explicit. Requesting a hint never decreases a score or marks the player as weak.

Attempt counts are session-only scaffolding signals. They are not persisted as personality, ability or behavior labels. The system should help the player recover flow, not construct a deficit profile.

## First-person game feel

Eclipse interactions reuse the existing first-person presentation layer. Tools, hands, wrist display, scan/measure modes and contextual feedback are presentation systems only; they do not own learning or completion authority. This preserves a clean separation between game feel and trusted outcomes.

Every important interaction should have an immediate sensory response even before success: motion, light direction, mechanical sound, change in vibration/animation, or spatial reconfiguration. The player should be able to form a hypothesis from the world response without needing explanatory text.

## Trusted outcome boundary

Clicking an artefact is never sufficient to earn evidence. `BeginAction` starts interaction and presentation. A mechanism must validate the resulting state and then call `ResolveTrustedAction`.

For the mirror lattice, M5.6B includes deterministic checks for:

- bilateral mirror symmetry with meaningful angular deflection;
- equality of incidence and reflection within a bounded tolerance plus receiver alignment;
- path stability across several positive and negative perturbations rather than one lucky configuration.

Once all evidence for an act is satisfied, the final causal control becomes available. The act only advances when the hidden mission evidence and its trusted world-state predicate are both complete.

## Failure and experimentation

A bad experiment must teach through consequence rather than punishment. Incorrect alignment scatters the beam. An unstable path drifts when perturbed. A model that fails a counterexample must be revised. Players should be encouraged to manipulate variables, test predictions and compare outcomes.

This is the central learning strategy: the game makes concepts useful before it names them.

## Source-complete boundary

M5.6B is source-complete when:

- the Eclipse epic has six causally necessary learning streams and six coherent acts;
- each player-facing action maps exactly to hidden M5.6 evidence or one trusted world-state outcome;
- the ordinary world-focus system can discover Eclipse artefacts;
- first-person presentation is available without owning completion authority;
- geometry/optics has executable deterministic validation rather than metadata only;
- progressive hints are bounded, requested by the player and non-punitive;
- player-facing contracts contain no school-style subject/score language;
- canonical and packaged catalogs remain identical;
- automated source and Unreal-side tests pass.

Native Unreal execution, authored final environments, final audio/VFX and representative-device certification remain separate evidence boundaries.

## Next phase

After this vertical epic, M5.6C should apply the same game-first architecture to **The Garden at the End of Winter**. Its core fantasy should be ecological restoration rather than a subject sequence: diagnose a living system, experiment with matter and resources, rebuild habitat, observe emergent recovery, negotiate competing needs and change the season through causal play.
