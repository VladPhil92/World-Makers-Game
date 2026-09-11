# C1 — XXII Century Citizenship & Interspecies Ethics Framework

## Purpose

World Makers is not only a game that contains educational material. C1 defines it as a formation environment for people who may inhabit the twenty-second century: citizens able to reason about living beings, planetary commons, technology, energy, artificial intelligence, power and coexistence as coupled systems.

The governing question is:

> What kind of human being should World Makers help form for a century in which human technical power can transform the conditions of life at planetary scale?

The framework is grounded in Juan Pablo Valderrama Pino's *En torno al animal: una pregunta por la soberania humana* and its later research extension on animality, artificial intelligence and the limits of the human political. The repository stores only a provenance/concept mapping, not a copy of those works.

## Philosophical ground

C1 translates the following authorial concepts into game-design constraints:

- **Sovereignty of Hospitality**: responsibility is not exhausted by possession, command or a legal minimum. Decisions must attend to other lives and the conditions in which they can flourish.
- **Sovereignty of Domination**: the game must make visible when technical, political or economic power treats living beings and natural systems merely as available material.
- **Radical singularity**: `animal` must never become one homogeneous gameplay category. Different species have different habitats, capacities, vulnerabilities and relations.
- **Living together**: a city, forest, river or farm is a more-than-human system, not a human stage with decorative fauna.
- **Decentered political decision**: a player exercising power must learn to identify who is affected but absent from the decision.
- **More-than-human community**: architecture, production, energy and governance can be designed so that differences persist instead of being erased by a single human utility function.

C1 does not turn Derrida or Valderrama into a moral answer key. Their work establishes the philosophical problem-space and the concepts the player must be capable of examining.

## Five domains

### Living World

Animals, plants and other living beings are represented through relations: habitat, nourishment, movement, reproduction, stress, territory, ecological function and response to human activity. Learning asks who lives here, what sustains them, and whose flourishing changes after an intervention.

### Planetary Commons

Water, air and soil are first-class systems rather than generic `pollution` bars. Actions may propagate upstream, downstream, through atmosphere, through soils and across ecological networks.

### Technology & Energy

The player must distinguish a technology's usefulness from its total consequences. Sustainable energy reasoning includes generation, demand, storage/flexibility, losses, resilience, material lifecycle, land/water requirements and effects on living systems.

### Responsible Intelligence

AI is treated as a governance problem as well as a technical one. A deployable automated system requires human accountability, bounded authority, auditability, privacy/fairness review, fail-safe behavior and review of computational/ecological cost.

The game deliberately does **not** decide whether an artificial system has moral status. The later philosophical research explicitly treats the machine as a second crisis of criteria historically used to define human exception; C1 turns that uncertainty into a reason to govern artificial decision power carefully, not into a claim that machines are or are not persons.

### Ethics, Sovereignty & Coexistence

The player examines who has authority to decide, who is missing from representation, which differences are being erased, what damage may be irreversible, and whether a decision can be revised after consequences become visible.

## Pedagogical rule: no ideology key

The citizenship runtime has no parameter for a `correct`, `preferred` or `approved` moral option.

It evaluates structured reasoning:

1. reasons;
2. at least one affected human perspective;
3. at least one affected non-human perspective;
4. at least one planetary common where relevant;
5. a trade-off;
6. uncertainty;
7. recognition of power asymmetry;
8. reversibility/irreversibility;
9. revision after evidence or consequences.

Different conclusions can therefore satisfy the learning contract if they meet the reasoning standard. Agreement with the designer cannot substitute for evidence.

## World rule

**Every action enters a web of relations.**

Examples:

- construction → territory → water → vegetation → fauna;
- energy → emissions/materials/land → air and habitat → living world;
- automation → decision authority → accountability → civic legitimacy;
- extraction → materials → habitat → biodiversity → communities.

C1 defines stable simulation hook IDs for water quality, air quality, soil health, biodiversity, habitat connectivity, animal stress, energy reliability, material demand, compute-energy demand, human wellbeing and civic legitimacy. Later simulation phases can bind these IDs to native world-state models without changing the pedagogical contract.

## First five XXII-century adventures

### The River of Two Futures

A community needs reliable energy. The river also carries migratory life and sustains a riparian ecosystem. The player must model water/energy/habitat coupling, test designs and deliberate among several defensible energy strategies.

### The City That Trusted the Algorithm

An automated water allocator produces efficient aggregate numbers but unequal outcomes. The player inspects the pattern, installs governance safeguards and revises policy while considering residents, operators, downstream wildlife and the water commons.

### The Forest That Learned to Listen

The player designs a wildfire/biodiversity sensing network. More sensors can mean better coverage but also energy use, disturbance, privacy risk and overconfidence in automation. Local knowledge and low-impact sensing become part of the design space.

### The Last Power Grid

The player must replace a polluting energy system without learning the false lesson that one renewable technology is automatically harmless. Reliability, storage, lifecycle materials, land use, wildlife, air quality, resilience and energy access all matter.

### The Community of New Standards

An integrative philosophy/civics adventure asks the player to design and revise a more-than-human community charter after its consequences become visible. Human residents, future generations, urban wildlife, domestic animals, wetlands, water, air and soil are represented as affected parties/systems.

## Runtime boundary

`FWMCitizenshipRuntime` is deterministic and stores stable IDs only. It does not store child-authored free text and has no gameplay authority. It emits evidence compatible with existing M5.2 primitives:

- `reason-through-dilemma`;
- `construct-to-constraint`;
- `model-system`.

The five adventure definitions also compose existing `predict-test-revise`, `observe-and-classify`, `sequence-and-infer` and `argue-and-revise` primitives.

## What C1 does not claim

C1 is the conceptual and source-runtime foundation. It does not yet claim that all water, air, soil, species, energy-grid or AI simulations are fully implemented in Unreal. The stable hooks and adventure producers intentionally expose that next implementation boundary instead of fabricating native systems that do not exist.

A later C2 phase should bind the framework to **Planetary Commons & Living World Simulation**: persistent water/air/soil state, habitat connectivity, species needs/stress and cross-system consequence propagation.
