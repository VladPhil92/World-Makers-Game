# World Makers — Reference-Driven Visual & Animation Polish

## Status

Source implementation target derived from the approved World Makers reference set plus the latest first-person gameplay concepts reviewed during visual development. The images are **directional references**, not runtime assets or pixel-perfect UI specifications.

## What the references consistently communicate

The strongest identity is not “more HUD” or “more glow”. It is a bright eco-futurist adventure world with readable depth, rounded scientific architecture, child-scale tools and learning that appears through interaction with the world.

### World composition

Use three readable depth bands whenever a vista matters:

1. foreground foliage, hands/tool or character framing;
2. a playable midground with a clear route or interaction target;
3. an aspirational background landmark such as the Research Dome, waterfalls, bridges, terraces, mountains or renewable-energy silhouettes.

The **Research Dome / greenhouse** is promoted to a signature landmark family. Curved modules, arches, domes, rounded corners, glass/greenhouse structures and biophilic overgrowth should dominate important authored architecture. Box-only construction should remain functional scaffolding, not the visual identity.

### First-person presentation

The newer gameplay concepts make first-person exploration especially valuable for science and construction. The source runtime therefore defines five presentation modes:

- Explore — quiet HUD, no mandatory raised tool;
- Build — build tool, snap feedback and build palette;
- Scan — scanner + one science context panel;
- Measure — measurement tool/readout;
- Observe — minimal tool footprint and close observation framing.

Hands and tools exist to communicate action. They must not occupy the center reticle or more than roughly 22% of the intended screen footprint. The wrist device is a secondary diegetic information anchor, not another persistent dashboard.

### HUD density correction

Some concepts intentionally show many simultaneous panels to communicate product breadth. That density is **not** the runtime baseline.

Rules:

- no more than two large panels simultaneously;
- only one context panel at a time;
- build palette only in Build mode;
- science panels only while scanning/measuring/observing;
- Explore remains visually quiet;
- important state may never depend on color alone.

Compass, compact hotbar, mission summary and minimap may remain small baseline anchors, subject to final tablet safe-area testing.

### Science color language

Domain accents may aid recognition without replacing labels/icons:

- Physics — cyan;
- Biology — green;
- Ecology — growth green;
- Chemistry — violet;
- Construction — World Makers blue;
- Discovery — gold.

Runtime saturation and bloom should be lower than key-art intensity so silhouettes, text and depth remain readable on tablets.

## Character and animation improvements

The four character references should not collapse into palette swaps. Silhouette, gear distribution and motion response must reinforce role readability.

The source runtime defines four bounded motion personalities:

- **Curious Explorer** — open arm swing, responsive head look, lively secondary motion;
- **Scientist Inventor** — compact gait, controlled arms, quicker settle;
- **Nature Guardian** — grounded gait, softer arm swing, slightly longer settle;
- **Knowledge Explorer** — neutral gait with more attentive head response.

These modifiers are presentation-only. They must not alter locomotion speed, collision, stamina, interaction success or timing authority.

### First-person animation vocabulary

Production animation should include at minimum:

`tool-raise`, `tool-lower`, `scan-anticipate`, `scan-hold`, `scan-settle`, `build-point`, `build-confirm`, `measure-focus`, `observe-focus`.

The desired interaction rhythm is:

**anticipation → clear contact/hold → short settle**.

Scanner/build-tool motion should preserve tool-hand contact. Reduced Motion removes camera bob, tool lag and nonessential secondary motion while retaining the information state.

## Relationship to existing tracks

- V3/P2: authored environment families should expand toward Research Dome, curved modules, bridges/arches and layered vista composition without changing the existing rainforest fallback contract.
- V4/P3: character production keeps the five-head stylized child proportions but must differentiate silhouette and practical gear by role.
- V5/P4: authored animation should consume the bounded motion-personality and first-person action vocabulary rather than changing movement authority.
- V6/P4: VFX retains causal semantics; science panels and effects should not become decorative spectacle.
- V7: contextual camera/UI remains the presentation coordinator; first-person modes are an additional presentation path, not a replacement for third-person exploration.
- V8/P5/N3: new visuals remain subject to tablet budgets, reduced-motion rules and native certification.

## Rejected interpretations

Do not infer from the concepts that World Makers should:

- keep every science/build panel visible at once;
- maximize bloom, saturation or holographic opacity;
- copy another game's HUD arrangement or block language;
- introduce a companion animal before gameplay actually supports one;
- treat a concept image as proof that a production asset exists;
- trade child readability or device performance for marketing-shot density.

## Source implementation

The machine-readable contract is `content/visual/governance/reference-driven-visual-polish-v2.json`. Runtime policy lives in `FWMReferenceVisualPolishRuntime`, with automation coverage for first-person modes, contextual HUD, Reduced Motion and character motion personalities.
