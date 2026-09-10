# World Makers

World Makers is a child-safe 3D open-world building, exploration and learning sandbox built in Unreal Engine. Children can build, destroy, explore and experiment freely, while optional fantastic adventures make authentic learning part of solving world problems rather than a worksheet layered on top of play.

## Product principles

- **High conceptual ceiling, low interaction friction.** Children may encounter advanced ideas through direct experience before formal notation.
- **Free World and Adventures are equally valid.** Open-ended creativity does not require mission completion.
- **Learning is structural.** Missions should require observation, construction, experimentation, interpretation, communication or reasoning to progress.
- **Child safety is architectural.** No third-party advertising in child gameplay, no purchasable premium currency, no paid randomness and no direct child payment flows.
- **Privacy is minimized by default.** Learning evidence uses stable IDs and bounded numeric state; current thought-runtime evidence does not persist child-authored free text or voice transcripts.

## Learning universe

First-class learning streams:

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

History and culture operate as cross-curricular context layers.

## Current architecture

- `Building` — safe creative construction and world-state persistence.
- `Mission` — legacy and composable mission evaluation, progression and minimized learning evidence.
- `Environment` — biome, exploration, ecosystem state and ecological building interactions.
- `Science` — deterministic matter, chemistry, physics, cellular and plant simulation.
- `Thought` — contextual English/Spanish communication, provenance-aware narrative graphs, ethical reasoning and philosophy argument revision.
- `Economy` — deterministic gameplay rewards separated from family-owned commerce entitlements.
- `Performance` / `Visual` — tablet-aware runtime quality and visual-density profiles.
- `apps/parent-portal` — parent-facing family/privacy/commerce boundary prototype.

The M5 composable mission layer uses reusable gameplay/learning primitives such as `predict-test-revise`, `model-system`, `communicate-in-language`, `interpret-text-world`, `reason-through-dilemma` and `argue-and-revise`. Subject runtimes produce truthful domain outcomes; Mission Runtime attributes those outcomes to learning objectives and progression.

## Content pipeline

Human-reviewable canonical content lives under `content/`. Runtime-staged JSON lives under `game/Content/WorldMakers/`. Repository Quality gates enforce schema/semantic contracts and canonical/runtime parity for staged science and thought catalogs.

Key design references:

- `docs/GDD.md`
- `docs/TDD.md`
- `docs/roadmap.md`
- `docs/fantastic-learning-universe.md`
- `docs/m5-2-composable-mission-runtime.md`
- `docs/m5-3-science-simulation-core.md`
- `docs/m5-4-language-literature-thought-runtime.md`

## Verification

GitHub-hosted source CI validates content, policy and source contracts. Native runtime certification is separate: Unreal Engine 5.8.2 on the locked self-hosted environment must compile `WorldMakersEditor` and execute the full `WorldMakers.*` automation namespace. Representative-device evidence is required before claiming tablet certification.

A green lightweight source check is therefore evidence of source integrity, not a substitute for native/device certification.
