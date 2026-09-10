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

## Runtime foundations

M5.2 introduces a composable evidence runtime. Missions can require reusable primitives such as `construct-to-constraint`, `predict-test-revise`, `model-system`, `communicate-in-language`, `interpret-text-world`, `reason-through-dilemma` and `argue-and-revise` instead of creating a hard-coded evaluator for every school subject.

M5.3 introduces deterministic science truth: matter, reactions, mechanics, circuits, cellular state, plant lifecycle and ecological coupling.

M5.4 introduces deterministic structured language and thought: bilingual contextual communication, provenance-aware narrative graphs, ethical reasoning and philosophy argument revision. The initial thought baseline uses stable semantic/reasoning IDs and does not retain child-authored free text or voice transcripts.

These layers are intentionally separate. Domain runtimes decide whether a scientific, linguistic, narrative or reasoning action is semantically valid; Mission Runtime decides which learning objective receives evidence, how much evidence is required, when a mission completes and which deterministic rewards are granted.

## Fantastic mission families

### Mathematics — The Vault of the Infinite Staircase

A tower rearranges itself every night. The player must discover numerical patterns, ratios, fractions and equations to reconstruct a path to the observatory. Advanced variants introduce unknowns, coordinate systems and optimization.

Learning evidence: successful pattern construction, equivalence, estimation, constraint satisfaction and explanation of a numerical rule.

### Geometry — Architects of the Impossible City

A floating city is collapsing because its bridges, domes and plazas no longer fit together. The player uses symmetry, transformations, tessellation, angle relationships, area, volume, scale and eventually Pythagorean reasoning to rebuild it.

Learning evidence: constructions satisfy geometric constraints instead of answering detached questions.

### English — The Dragon Who Lost His Words

A multilingual dragon can no longer activate ancient machines because inscriptions have fragmented. Players use contextual vocabulary, syntax, listening, inference and dialogue to restore meaning. English can be progressively used as an interface and problem-solving language rather than merely a subject screen.

M5.4 foundation: a communication action succeeds only when target language, intended meaning and acceptable register align. Wrong-language or wrong-intent responses cannot satisfy the same challenge merely because they contain familiar tokens.

Learning evidence: meaningful comprehension and production in context.

### Spanish — La Biblioteca de las Palabras Perdidas

A living library changes when words are used precisely. Players reconstruct sentences, distinguish meanings, identify relationships between words, interpret figurative language and assemble short texts that alter the environment.

M5.4 foundation: Spanish is a first-class target language in the same semantic challenge model as English rather than a translation afterthought.

Learning evidence: semantic precision, syntax, inference and contextually appropriate expression.

### Literature — The Labyrinth of Stories

A shifting labyrinth is built from myths and legends from many traditions. Players compare versions, infer motives, distinguish narrators, interpret symbols and notice recurring structures without flattening culturally distinct stories into one universal template.

M5.4 foundation: narrative content is a graph with point-of-view nodes, inference edges, source classification (`retelling`, `adaptation`, `historical-source`), provenance keys and cultural-review state. The first prototype uses a Minotaur-labyrinth retelling context only as a systems proof.

Learning evidence: interpretation changes what the world reveals or which path becomes available.

### Biology — The City Inside a Cell

The player is miniaturized into a living cell where organelles and cellular processes become functional districts. Energy, transport, membrane integrity, information, nutrients, oxygen and waste must remain in balance.

Learning evidence: system repair and causal modeling rather than organelle-name recall alone.

### Chemistry — The Alchemist's Archipelago

Islands are governed by material properties and transformations. Players investigate states of matter, mixtures, solutions, saturation, separation, reaction evidence, stoichiometric relationships and conservation of mass.

Learning evidence: predictions and successful manipulation of a deterministic virtual chemistry model. Canonical chemistry content remains virtual-only and contains no child-facing hazardous real-world procedures.

### Physics — The Moonforge

A forge on a broken moon can only be restored by making machines actually work. Players reason with forces, motion, momentum, energy, circuits, mirrors, waves and eventually orbital relationships.

Learning evidence: physical systems behave according to the model rather than accepting detached answers.

### Ecology — The Forest of a Thousand Voices

A living forest reacts to water, plant growth, habitat quality, biodiversity pressures and player construction. The player must see the ecosystem as a coupled system rather than a collection of decorative animals and plants.

Learning evidence: observation, prediction, intervention and revision in a persistent environment.

### Ethics — The Bridge Between Two Villages

Two communities need scarce rebuilding resources after a storm. Options may prioritize urgency, equal division, participation, promises or other defensible considerations.

M5.4 foundation: there is no hidden moral-answer key. The runtime evaluates whether the player gives a reason relevant to the chosen option, considers multiple affected perspectives and acknowledges a tradeoff. Different options can satisfy the same reasoning standard.

Learning evidence: reasoned choice, perspective-taking and explicit tradeoff recognition.

### Philosophy — The Ship That Was Never the Same

A magical ship is repaired piece by piece until none of its original material remains. Players must decide what makes something the same thing through change and confront the rebuilt-original-parts counterexample.

M5.4 foundation: both "same ship" and "different ship" positions may succeed. The runtime evaluates whether reasons target the selected claim, assumptions are surfaced, a counterexample is considered and the argument is revised.

Additional future problems include the Cave of Shadows, the Liar's Door, the Machine That Predicts Tomorrow, the Island of Perfect Copies and the City Where Nobody Can Lie.

Learning evidence: argument quality, contradiction detection, assumptions, counterexamples and reason revision rather than ideological conformity.

## Mission design contract

Every production adventure should define:

- primary and secondary disciplines;
- concepts and formal vocabulary;
- age/scaffold profile;
- Experience → Concept → Formalization depth layers;
- fantastic premise and stakes;
- required world systems;
- player actions that count as evidence;
- stable evidence events and learning objectives;
- misconceptions and alternate outcomes;
- non-punitive retry path;
- cross-disciplinary links;
- localization/read-aloud requirements;
- pedagogy, safety and cultural review state where relevant.

A mission fails the World Makers quality bar if the academic idea can be removed without materially changing how the mission is played.

## Quality bar

World Makers should not be "educational content attached to a game." It should be **a game world whose most interesting problems are solved by thinking**.

M5.5 begins converting these foundations into the first playable Fantastic Adventure Pack. M5.6 then composes them into larger cross-disciplinary adventures such as The Eclipse Engine and The Garden at the End of Winter.
