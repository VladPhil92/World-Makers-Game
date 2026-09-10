# M5 — Fantastic Learning Universe

## Product thesis

World Makers is not a simplified school game. It is a premium open-world sandbox where children can build, destroy, explore, experiment and tell stories freely, while optional and discoverable missions expose them to real ideas that may be conceptually advanced.

The design rule is:

> **Do not lower the conceptual ceiling. Lower the interaction friction.**

A child may encounter ratios, vectors, chemical reactions, cell systems, mythic archetypes, logical dilemmas or questions of personal identity long before they can express the formal vocabulary. The game should let the player first *experience* the idea, then *name* it, and finally *formalize* it when appropriate.

## Dual game loop

World Makers has two equally legitimate modes of play:

1. **Free World** — open-ended building, destruction, exploration, decoration, experimentation and storytelling with no required learning path.
2. **Adventures / Missions** — fantastic narrative challenges in which successful play necessarily demonstrates learning.

The mission layer must never turn the world into a worksheet. Evidence should come from what the player builds, tests, observes, predicts, explains, compares, chooses or revises.

## Core learning streams

World Makers treats the following as first-class learning streams:

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

History and culture remain cross-curricular context layers, especially for literature, myths, legends, architecture, technology and place-based missions.

## Layered epistemic depth

Every substantial mission should be authorable at three simultaneous depths instead of assuming that young players cannot encounter difficult ideas.

### Layer A — Experience

The child manipulates the phenomenon directly: balances a structure, mixes virtual substances, traces a shadow, follows energy through a system, resolves a dialogue, or decides how to distribute a scarce resource.

### Layer B — Concept

The game names the pattern: symmetry, momentum, ecosystem, metaphor, oxidation, fairness, identity, inference, proportion, etc.

### Layer C — Formalization

Where developmentally appropriate, the mission exposes formal notation, equations, diagrams, technical vocabulary, symbolic representation or explicit reasoning. Formalization is an extension of play, not a prerequisite to enjoyment.

Age bands therefore control scaffolding, interface density, reading load and formal notation — **not which ideas a child is allowed to encounter**.

## Fantastic mission families

### Mathematics — The Vault of the Infinite Staircase

A tower rearranges itself every night. The player must discover numerical patterns, ratios, fractions and equations to reconstruct a path to the observatory. Advanced variants introduce unknowns, coordinate systems and optimization.

Learning evidence: successful pattern construction, equivalence, estimation, constraint satisfaction and explanation of a numerical rule.

### Geometry — Architects of the Impossible City

A floating city is collapsing because its bridges, domes and plazas no longer fit together. The player uses symmetry, transformations, tessellation, angle relationships, area, volume, scale and eventually Pythagorean reasoning to rebuild it.

Learning evidence: constructions satisfy geometric constraints instead of answering detached questions.

### English — The Dragon Who Lost His Words

A multilingual dragon can no longer activate ancient machines because inscriptions have fragmented. Players use contextual vocabulary, syntax, listening, inference and dialogue to restore meaning. English can be progressively used as an interface and problem-solving language rather than merely a subject screen.

Learning evidence: meaningful comprehension and production in context.

### Spanish — La Biblioteca de las Palabras Perdidas

A living library changes when words are used precisely. Players reconstruct sentences, distinguish meanings, identify relationships between words, interpret figurative language and write or assemble short texts that alter the environment.

Learning evidence: semantic, grammatical and communicative choices with visible consequences.

### Literature — The Labyrinth of Stories

Universal myths and legends become explorable worlds rather than static readings. Missions can draw from Greek, African, Asian, Indigenous American, Middle Eastern, European and Latin American traditions with documented provenance and cultural review.

Players may enter the Labyrinth of the Minotaur, follow Anansi's stories, encounter flood myths comparatively, investigate heroic journeys, or rebuild a legend from contradictory narrators.

Learning evidence: plot reconstruction, archetype recognition, point of view, symbolism, comparison, inference and interpretation. The game must distinguish retelling, adaptation and historical source.

### Biology — The City Inside a Cell

The player is miniaturized into a living cell whose systems have stopped cooperating. Organelles become functional districts: energy production, transport, information, membranes and waste processing. Later missions can explore tissues, organs, heredity, adaptation and microbial life.

Learning evidence: restoring biological function by correctly understanding relationships among systems.

### Chemistry — The Alchemist's Archipelago

An archipelago is powered by matter transformations. Players investigate atoms and molecules through manipulable models, states of matter, mixtures, solutions, solubility, acids/bases, reaction evidence, conservation of mass, reaction rate and energy change.

The chemistry layer must teach authentic concepts while keeping child safety boundaries: virtual experiments may represent hazardous phenomena, but the game should not provide actionable real-world procedures for dangerous reactions.

Learning evidence: predictions, controlled variable changes, substance classification, conservation relationships and interpretation of reaction evidence.

### Physics — The Moonforge

An ancient forge orbiting a small moon can only be repaired by understanding forces and energy. Players build levers, pulleys, counterweights, projectiles, circuits, mirrors and resonant devices. Later missions introduce vectors, momentum, buoyancy, electricity, waves, optics and orbital reasoning.

Learning evidence: the constructed system physically works under simulated constraints.

### Ecology — The Forest of a Thousand Voices

A biome reacts persistently to player actions. The child must understand food webs, biodiversity, water and nutrient cycles, carrying capacity, habitat fragmentation, succession and trade-offs between construction and environmental health.

Learning evidence: restoring or intentionally redesigning ecosystem conditions and predicting secondary effects.

### Ethics — The Bridge Between Two Villages

Two communities need the same limited resources after a storm. There is no single hidden 'correct' moral answer. Players must consider fairness, equality, need, promises, consent, stewardship and consequences, then live with the outcomes and revisit their reasoning.

Learning evidence: consistency, perspective-taking, reasons offered and willingness to revise — never ideological conformity.

### Philosophy for Children — The Ship That Was Never the Same

Players rebuild a legendary vessel piece by piece until none of its original material remains. Characters disagree over whether it is still the same ship. The mission introduces identity, persistence, language, evidence and argument through exploration and dialogue.

Other philosophy adventures can draw on the Cave of Shadows, the Liar's Door, the Machine That Predicts Tomorrow, the Island of Perfect Copies and the City Where Nobody Can Lie.

Learning evidence: identifying assumptions, distinguishing reasons from claims, comparing positions, finding contradictions, generating counterexamples and revising an argument.

## Cross-disciplinary adventures

The strongest adventures should combine disciplines rather than isolate them.

Example: **The Eclipse Engine** may require geometry to align mirrors, physics to understand light, mathematics to calculate timing, literature to decode a mythic astronomical text, English or Spanish to communicate with characters, and philosophy to question whether prediction is the same as knowledge.

Example: **The Garden at the End of Winter** may combine botany/biology, chemistry of soil and nutrients, ecology, ratios for irrigation, ethics of resource allocation and a myth about seasonal cycles.

## Mission design contract

A production mission should eventually declare:

- primary and secondary disciplines;
- concepts and formal vocabulary;
- age/scaffold profile;
- experiential, conceptual and formal depth layers;
- fantasy premise and narrative stakes;
- required world systems;
- player actions that demonstrate understanding;
- observable evidence events;
- misconceptions that can produce meaningful alternate outcomes;
- non-punitive retry/iteration behavior;
- cross-disciplinary links;
- localization/read-aloud requirements;
- pedagogy, safety and cultural review status.

## Runtime implication

The current Mission Runtime supports `measure-and-build` and `observe-ecosystem`. M5 requires a composable evaluator architecture rather than one evaluator per school subject.

Recommended evaluator primitives:

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

Subjects compose these primitives. Chemistry and physics, for example, can both use `predict-test-revise`; literature and philosophy can both use `interpret-text-world`, while generating different evidence.

## Quality bar

A World Makers learning mission fails design review if the learning could be removed without materially changing how the mission is played.

The target is not 'educational content inside a game'. The target is **a game world whose most interesting problems are solved by thinking**.
