# M5.4 — Language, Literature and Thought Runtime

## Purpose

M5.4 turns language, literature, ethics and philosophy into gameplay systems that can produce structured evidence for the M5.2 composable mission runtime. The goal is not to bolt quizzes onto adventures. Players must communicate in context, infer from narrative information, compare consequences and perspectives, and revise arguments when challenged.

The runtime follows the M5 product rule: **do not lower the conceptual ceiling; lower the interaction friction**.

## Privacy and expression boundary

The first runtime uses `stable-ids-no-child-free-text`.

Gameplay submissions are structured selections such as language intent, register, inference route, reason, perspective, assumption, counterexample and revision count. M5.4 does **not** persist child-authored free text, voice transcripts, political/religious position labels, personality scores or ideological profiles.

This is a runtime/privacy boundary, not a claim that children cannot write or speak. Rich open production can be added later behind explicit moderation, retention and family/privacy decisions. The current architecture gives the game a rigorous reasoning model before adding high-risk free-form data capture.

## English and Spanish communication

A communication challenge defines:

- target language (`en` or `es` in the initial catalog);
- world context;
- intended meaning;
- acceptable register(s);
- authored response options with language, semantic intent and syntax-pattern metadata;
- a stable evidence event.

Success requires the selected option to satisfy **language + meaning + register** simultaneously. A grammatically plausible sentence in the wrong language, or a sentence with the wrong communicative intent, cannot satisfy the challenge.

This supports contextual gameplay such as asking a guardian for directions, requesting a clue, negotiating access or interpreting an instruction. Later content may add listening, morphology, figurative language and productive composition without replacing this semantic contract.

M5.2 primitive: `communicate-in-language`.

## Literature, myths and legends

Narratives are represented as directed graphs rather than fixed quiz passages. Each story carries:

- stable story ID;
- cultural/traditional context ID;
- source class: `retelling`, `adaptation` or `historical-source`;
- provenance key;
- cultural-review state;
- nodes with localization passage keys and point-of-view IDs;
- choices/edges with inference tags and evidence events.

The first prototype graph is a retelling context for the Minotaur labyrinth tradition. Its purpose is to prove inference, symbolism/context clues and point-of-view transitions, not to claim one canonical version of a culturally transmitted story.

A narrative edge is valid only from its authored source node. This lets future missions make interpretation alter where the player can travel, which character perspective becomes available or what information the world reveals.

M5.2 primitive: `interpret-text-world`.

## Ethics: quality of reasoning, not moral answer keys

Ethical dilemmas contain multiple legitimate options. Each option may expose:

- supporting reasons;
- affected perspectives;
- explicit tradeoff tags;
- a normalized consequence profile across dimensions such as fairness, welfare, consent, stewardship and common good.

The consequence profile is a **design model**, not an objective moral truth score. It is used to make consequences discussable and comparable.

`EvaluateEthicalReasoning` does not decide that one option is morally correct. It accepts an option when the player:

1. supplies at least one authored reason that actually supports that option;
2. considers the configured minimum number of affected perspectives;
3. explicitly acknowledges a tradeoff.

Automation tests require opposing options in the same dilemma to be able to pass when reasoned well. This protects the system from silently becoming an ideology-answer key.

M5.2 primitive: `reason-through-dilemma`.

## Philosophy for children

Philosophy problems model argument structure. A problem defines:

- alternative claim IDs;
- reason links and their relations (`supports`, `challenges`, `clarifies`);
- assumptions;
- counterexamples;
- minimum revision count;
- stable evidence event.

The first prototype is the Ship of Theseus. Both "same ship" and "different ship" positions can satisfy the runtime. What matters is whether the submitted reason actually targets the selected claim, whether assumptions are surfaced, whether a counterexample is considered, and whether the argument is revised.

This establishes the foundation for later problems about identity, knowledge, truth, causality, freedom, language and mind without requiring ideological conformity.

M5.2 primitive: `argue-and-revise`.

## M5.2 integration

`UWMLanguageThoughtSubsystem` loads the staged catalog and provides Blueprint-callable operations:

- `EvaluateCommunicationAndRecord`;
- `TraverseNarrativeAndRecord`;
- `EvaluateEthicalReasoningAndRecord`;
- `EvaluatePhilosophicalArgumentAndRecord`.

Each successful operation emits a stable `FWMThoughtEvidenceResult` and submits its primitive/event pair to the active composable mission through `RecordComposableEvidence`.

The thought runtime therefore owns semantic/argument validity while Mission Runtime owns objective attribution, counts, completion and rewards.

## Data pipeline

Canonical source:

`content/thought/language-literature-thought-v1.json`

Packaged runtime copy:

`game/Content/WorldMakers/Thought/language-literature-thought-v1.json`

Repository Quality requires semantic parity between both copies and validates bilingual coverage, narrative provenance, ethical neutrality constraints and philosophy structure.

## Source-complete scope

M5.4 is source-complete when:

- English and Spanish contextual communication are represented;
- narrative graphs support provenance, source class, point of view and inference edges;
- ethical reasoning requires reasons, multiple perspectives and tradeoff recognition without a hidden moral-answer field;
- philosophy reasoning requires claim-linked reasons, assumptions, counterexamples and revision;
- thought evidence can flow into M5.2 composable missions;
- stable-ID/no-child-free-text privacy is enforced;
- canonical/runtime content parity is checked;
- Unreal Automation and Repository Quality gates cover the behavior.

Native Unreal Engine 5.8.2 execution remains a separate certification layer.

## Next phase

M5.5 builds the first Fantastic Adventure Pack on top of M5.2, M5.3 and M5.4. That phase turns the reusable systems into playable mission families such as The Dragon Who Lost His Words, La Biblioteca de las Palabras Perdidas, The Labyrinth of Stories, The City Inside a Cell, The Alchemist's Archipelago, The Moonforge, The Bridge Between Two Villages and The Ship That Was Never the Same.
