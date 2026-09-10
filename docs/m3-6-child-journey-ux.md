# M3.6 — Child Journey UX / My Adventures

## Purpose

M3.6 turns the technical read models built across M2–M3.5 into one coherent child-facing experience: **My Adventures**.

The UI is a projection, not a new progression authority. Missions remain authoritative for learning progress, the biome remains authoritative for observation/discovery, ecological building remains authoritative for interventions, Environment remains authoritative for ecosystem state, and Build Unlocks remains authoritative for creative rewards.

The child-facing loop becomes:

`explore → notice → try → build → see what changed → discover a new possibility → choose what to do next`

Stopping is always safe. Nothing in My Adventures decays because a child closes the game or chooses another activity.

## Architecture

```text
Mission Runtime -------------------\
Biome Runtime ----------------------\
Ecological Build --------------------> UWMChildJourneySubsystem -> UWMChildJourneyWidget
Environment State ------------------/
Build Unlocks ---------------------/
```

`UWMChildJourneySubsystem` composes existing read models into `FWMChildJourneySnapshot`. It does not persist a second copy of progression.

The only supported write is explicit activation of an already-available mission through `ActivateAdventure` / `ActivateRecommendedMission`. The child journey layer cannot grant evidence, rewards, ecological effects or commerce state.

## Child adventure model

Each card contains only stable projection metadata:

- `CardId` — internal UI projection identifier;
- `SourceId` — trusted domain identifier used for traceability;
- `Kind` — mission, discovery, ecosystem care or creative unlock;
- `State` — `Hidden`, `Ready`, `InProgress` or `Complete`;
- bounded progress units/fraction;
- whether the source mission is selectable.

Technical IDs are never primary display copy. `ResolveChildTitle` and `ResolveChildDescription` map known sources to localization-ready `LOCTEXT` strings.

## Progressive reveal

My Adventures deliberately avoids presenting the full future progression tree as a giant checklist.

- `Hidden`: prerequisites are not yet relevant; the card is not rendered.
- `Ready`: the activity is relevant now.
- `InProgress`: the child has begun naturally producing evidence or build requirements.
- `Complete`: the accomplishment remains visible and does not decay.

This is deterministic and contains no clock-dependent state.

## Current Caribbean Rainforest presentation

The projection can represent:

- **Rainforest Detective** — aggregate deliberate observations in the active biome;
- **Measure and Build** — mathematics mission 01;
- **The Short Span Challenge** — mathematics mission 02;
- **Discover the Rainforest** — science observation mission;
- **Protect the Forest Floor** — soil-buffer ecological build;
- **Make a Shady Shelter** — shade-shelter ecological build;
- **Create a Habitat Garden** — habitat-garden ecological build;
- **Leaf Roof**, **Rainforest Planter**, and **Bamboo Bridge** — persistent creative unlocks.

The panel also gives calm, localized context for the current rainforest zone and ecological reaction state.

## HUD integration

The legacy M2.2 `NextMissionButton` widget identifier is retained to protect source compatibility, but its visible label and child behavior are now **My Adventures**. It toggles the journey panel instead of blindly cycling missions.

The existing keyboard/gamepad prototype mission-cycle input remains available as a technical fallback; the touch-facing journey path is intentional selection rather than blind cycling.

The main HUD also resolves mission and ecological piece IDs into child-facing labels instead of printing identifiers such as `mission.*` or `eco.*`.

## Tablet interaction

The panel is programmatic UMG and sized as a compact tablet-side overlay. Footer actions preserve or exceed the existing M1.7 touch-target baseline:

- width: 168 px;
- height: 72 px.

The panel provides **Continue Adventure** and **Close**. Hidden future cards are not rendered.

## Wellbeing and commerce boundary

My Adventures intentionally contains none of the following mechanics:

- XP or grind scoring;
- leaderboards or public status comparison;
- streaks;
- countdowns;
- daily-return rewards;
- expiring tasks;
- loss for stopping;
- urgency copy;
- storefront prompts or purchasable progression.

Creative unlocks are read from the Trust Economy / Building unlock state established in M3.5. The UI cannot manufacture them.

## Privacy

M3.6 adds no child profile, name, email, free-text journal, chat, advertising identifier or commercial transaction data. `FWMChildJourneySnapshot` is ephemeral and derived from existing stable IDs and bounded progress state.

## Tests and CI

Automation coverage:

- `WorldMakers.UI.ChildJourney.StateMapping`
- `WorldMakers.UI.ChildJourney.ProgressiveReveal`
- `WorldMakers.UI.ChildJourney.FriendlyLabelsHideTechnicalIds`

Repository gate:

- `scripts/validate-m3-6-child-journey-ux.py`

The gate runs in both Repository Quality and Unreal CI while retaining M1–M3.5 gates.

## Certification boundary

M3.6 is source-level implementation. It must not be called native/device certified until Issue #9 is resolved with the locked Windows x64 Unreal Engine 5.8.2 runner, authored certification map, successful `WorldMakers.*` automation execution, and retained runtime evidence.

## Next phase

**M3.7 — Tablet Performance Profiles & Capture** should quantify frame time, foliage/actor budgets, rendering tiers and representative tablet performance for the full M3.1–M3.6 vertical-slice loop before M3.8 certification.
