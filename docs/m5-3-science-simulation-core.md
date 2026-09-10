# M5.3 — Science Simulation Core

## Purpose

M5.3 gives World Makers a deterministic scientific world model that can produce authentic gameplay evidence for the composable mission runtime introduced in M5.2.

The design objective is not to imitate a school worksheet. Scientific ideas become properties of the world: matter changes state, solutes reach saturation, filtration behaves differently from dissolution, balanced reactions consume limiting reagents, forces change motion, circuits obey relationships among voltage/current/resistance, cells depend on interacting systems, and plants respond to environmental constraints.

## Matter and chemistry

The first chemistry core includes:

- substance identity through stable IDs;
- molar mass;
- melting and boiling thresholds for the simplified phase model;
- water solubility and saturation;
- conceptual filtration that retains undissolved solids while dissolved solute remains in the filtrate;
- balanced reaction definitions;
- stoichiometric coefficients;
- limiting-reagent calculation;
- conservation-of-mass validation before a reaction is accepted;
- deterministic post-reaction inventories;
- stable learning evidence event IDs.

The first canonical reaction is iron oxidation. It exists to prove that the runtime can validate and execute a balanced transformation rather than merely play a reaction animation.

The phase model is intentionally a simplified educational model. It does not yet represent pressure-dependent phase diagrams, polymorphism, sublimation, non-ideal solutions or reaction kinetics. Those can be layered later without replacing the substance identity contract.

## Safety boundary

Chemistry simulation is **virtual-only**. Canonical content stores scientific identities, properties, relationships and reaction models, but not child-facing real-world procedures, recipes, step lists or instructions for carrying out reactions outside the game.

The safety boundary must not be interpreted as lowering scientific depth. Children may encounter authentic ideas such as molar relationships, limiting reagents, conservation laws, acids/bases, rates or energy changes as the simulation expands. The restriction is on actionable hazardous procedure, not conceptual complexity.

## Physics

The first physics core provides deterministic primitives for:

- mass;
- position and velocity vectors;
- constant-force integration using `F = ma`;
- momentum `p = mv`;
- kinetic energy `Ek = 1/2 mv^2`;
- ideal direct-current circuits;
- Ohm's law `I = V/R`;
- electrical power `P = VI`.

These primitives are world mechanics. A future Moonforge mission can therefore require a machine to actually move, balance or energize correctly instead of accepting a multiple-choice answer about force or electricity.

## Biology and botany

M5.3 now contains two biological layers.

### Cellular systems

`FWMCellSystemState` models the functional relationships among:

- membrane integrity;
- energy availability;
- transport efficiency;
- information integrity;
- waste load;
- nutrient availability;
- oxygen availability;
- temperature suitability.

A metabolism step calculates energy production, transport work and waste production/clearance from those interacting constraints. This is the first technical foundation for **The City Inside a Cell**: restoring a cell will require repairing relationships among systems rather than matching organelle names in a quiz.

The cellular quantities are a **normalized gameplay proxy**. They represent causal system state, not literal ATP counts, molecules, concentrations or biological time scales.

### Plant lifecycle

The plant model introduces a lifecycle state machine:

`Seed -> Germinating -> Seedling -> Vegetative -> Flowering -> Fruiting`

Growth is constrained by water, nutrients, light and temperature suitability. Germination depends on environmental conditions. Fruiting additionally depends on pollinator availability.

The current biomass, root, leaf, photosynthesis and demand quantities are a **normalized gameplay proxy**, not literal grams, leaf area, oxygen volume or nutrient concentration. Their purpose is to preserve causal biological relationships while allowing later species-specific calibration.

Age bands must change scaffolding and formalization, not disable the underlying biological concept. The same system can support a young player noticing that a dry plant grows poorly and an older player reasoning about limiting factors, resource allocation, cellular interdependence and ecological interactions.

## Ecology coupling

Plant simulation can generate a bounded `FWMEnvironmentStateDelta` for the existing ecosystem model. Growth can contribute to:

- vegetation health;
- soil protection through root-development proxy;
- shade coverage through photosynthesis/leaf-development proxy.

This is the first explicit bridge from biology/botany into the persistent ecological state already used by the Caribbean Rainforest vertical slice.

The coupling is intentionally one-way in M5.3 source implementation: science produces trusted bounded effects that Environment may consume. Future iterations can make soil moisture, nutrient state, habitat conditions and construction impacts fully bidirectional.

## Data pipeline

Canonical source:

`content/science/science-simulation-core-v1.json`

Packaged runtime copy:

`game/Content/WorldMakers/Science/science-simulation-core-v1.json`

`UWMScienceSimulationSubsystem` loads and semantically validates the staged catalog. Repository Quality requires canonical/runtime parity.

The initial canonical catalog includes matter properties, one mass-balanced reaction and one configurable learning-plant profile. Physics and cellular-system models are generic deterministic laws rather than scenario-specific content, so adventures can provide their own initial conditions without changing the domain model.

## M5.2 integration

M5.3 separates **simulation truth** from **mission evidence semantics**.

Example flow:

1. a player predicts what will happen;
2. a science interaction calls a deterministic M5.3 simulation;
3. the simulation produces an accepted result and stable evidence event;
4. an adapter submits that event to M5.2 `RecordComposableEvidence` with the appropriate primitive, such as `predict-test-revise` or `model-system`;
5. M5.2 attributes evidence to the learning objective and updates mission progress.

This prevents mission code from inventing scientific outcomes and prevents the science core from knowing which school subject or adventure is currently active.

## Source-complete scope

M5.3 source completion requires:

- deterministic chemistry operations for matter state, saturation, filtration and at least one balanced transformation;
- deterministic mechanics and ideal circuit relationships;
- deterministic cellular-system behavior with limiting factors;
- deterministic plant lifecycle/growth behavior;
- explicit ecology coupling;
- a staged data-driven science catalog;
- source quality validation;
- Unreal Automation tests across chemistry, physics, biology and botany/ecology;
- preservation of the M5.2 composable evidence boundary.

Native Unreal and representative-device execution remain separate certification evidence. A green source gate must not be represented as device certification.

## Next phase

M5.4 adds language, literature and thought runtime: contextual English/Spanish communication, myth/legend narrative graphs, interpretation mechanics, ethical dilemmas and philosophical argument/revision models.
