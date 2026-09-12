# C2 — Planetary Commons & Living World Simulation

C2 is the first material simulation layer for the **XXII Century Citizenship Framework** introduced in C1. Its purpose is to make ecological, technological and interspecies consequences exist as deterministic world state rather than as explanatory text.

## Design rule

> Every action enters a web of relations.

C2 therefore models consequences across coupled systems rather than treating water, animals, infrastructure or technology as isolated meters.

## Relationship to M3.4 and M3.5

C2 does **not** delete or silently replace the existing reactive ecosystem work.

- M3.4 remains the local care-action authority for `vegetation health`, `water flow`, `soil protection` and `shade coverage`.
- M3.5 continues to route ecological building interventions through M3.4.
- C2 depends on M3.4 and projects accepted legacy changes into the broader citizenship simulation as bounded deltas.
- C2 owns the eleven simulation hooks defined by C1.

This avoids two independent systems both claiming authority over the same M3.4 variables while allowing the project to evolve toward a richer planetary model.

## Eleven C1 hooks

C2 exposes:

1. `water-quality`
2. `air-quality`
3. `soil-health`
4. `biodiversity`
5. `habitat-connectivity`
6. `animal-stress`
7. `energy-reliability`
8. `material-demand`
9. `compute-energy-demand`
10. `human-wellbeing`
11. `civic-legitimacy`

`habitat-connectivity` and `animal-stress` are **derived state**. Content cannot set them directly.

## Habitat connectivity

Habitat is modeled as a set of named ecological links with bounded permeability `[0,1]`.

The prototype contains:

- river ↔ understory;
- understory ↔ canopy;
- north forest ↔ south forest.

Global connectivity is derived from those links. An intervention can improve or fragment an individual link; it cannot assign the global connectivity score directly.

This makes a road, corridor or riparian restoration causally legible instead of reducing fragmentation to a decorative status bar.

## Living World: no generic animal

C2 has no single mutable `Animal` entity. It stores multiple species-need profiles. Each profile has different thresholds for:

- water quality;
- air quality;
- soil health;
- biodiversity;
- habitat connectivity;
- disturbance sensitivity.

The initial prototype profiles are deliberately synthetic and marked `scientificReviewState=draft`:

- `species.prototype.river-frog`;
- `species.prototype.canopy-pollinator`;
- `species.prototype.forest-cat`.

They are **simulation prototypes, not conservation claims**. Production species profiles require ecological/scientific review before their names or quantitative thresholds can be treated as factual representations.

Per-species stress is derived from that species' own deficits plus disturbance pressure. `animal-stress` is only an aggregate read model; the individual states remain available so UI and missions do not erase singular differences.

## Deterministic propagation

The model advances in explicit discrete steps. It never depends on rendering framerate.

Each step currently applies these bounded couplings:

- material demand places pressure on soil health;
- low soil health creates an erosion pressure on water quality;
- water + air + soil + habitat connectivity pull biodiversity toward a coupled ecological state;
- compute-energy demand places pressure on energy reliability;
- water + air + energy reliability + biodiversity influence human wellbeing;
- species stress is recalculated from the resulting state.

All primary and derived values are clamped to `[0,1]`.

`civic-legitimacy` is intentionally not automatically equated with ecological quality. Later governance gameplay must change it through accountable political processes rather than by assuming that one environmental outcome automatically legitimizes a decision.

## Interventions and trade-offs

The source prototype includes both restorative and harmful/trade-off interventions:

- riparian buffer restoration;
- wildlife corridor construction;
- road fragmentation;
- wastewater discharge;
- renewable microgrid;
- compute expansion.

A renewable microgrid, for example, can improve air quality and reliability while still increasing material demand. Road infrastructure can improve a human service variable while fragmenting habitat and harming biodiversity.

The point is not to teach that infrastructure is bad or that one technology is morally pure. The player must learn to read **multi-system consequences**.

## Legacy projection

`UWMPlanetaryCommonsSubsystem` declares an initialization dependency on `UWMEnvironmentStateSubsystem`.

At startup it seeds ecological channels from the accepted M3.4 state. Later M3.4 state changes are projected as **deltas** instead of overwriting C2 wholesale:

- M3.4 water-flow change → C2 water-quality delta;
- M3.4 soil-protection change → C2 soil-health delta;
- vegetation/habitat-quality change → C2 biodiversity delta.

This is a migration bridge, not a claim that the old four-channel ecosystem is scientifically equivalent to the new variables.

## Pedagogy and philosophy boundary

C2 simulates consequences; it does not issue a moral verdict.

There is no `correctOptionId`, doctrinal score or automatic reward for reaching a designer-preferred state. C1 continues to evaluate reasoning quality, affected perspectives, trade-offs, uncertainty, power and revision.

The philosophical objective is compatible with the **Sovereignty of Hospitality** framework: power over a world becomes visible as responsibility for relations and consequences, while the differences among living beings remain structurally represented.

## Privacy and safety

The runtime stores stable IDs and bounded numeric state only. It contains no child-authored free text, biometric information, advertising identifiers, engagement timers or monetization hooks.

## Current evidence boundary

C2 is **source-complete simulation infrastructure** when its source gates pass. Ordinary CI can validate deterministic contracts, data integrity and source automation tests.

Native Unreal execution remains a separate evidence boundary. The self-hosted Unreal runner must compile and execute `WorldMakers.Citizenship.C2.*` before native runtime behavior is certified.

## Next phase

The logical C3 phase is **Sustainable Infrastructure, Energy & Responsible Automation Integration**: connect real build/energy/AI gameplay producers to C2 interventions and expose causal feedback through contextual HUD, missions and Laboratory mode without turning the simulation into a moral scoreboard.
