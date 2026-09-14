# M5.6C — The Garden at the End of Winter

## Product intent

The Garden at the End of Winter is a **living system adventure**, not a sequence of lessons. The player enters a garden whose winter has stopped behaving like a season: roots are dark, water carries unstable material, flowers open without fruiting, and restoring one subsystem does not restore the whole world.

The core game loop is:

`discover -> manipulate -> predict -> test -> see visible consequence -> adapt -> stabilize`

The player should feel that they are repairing a mysterious ecosystem. Biology, chemistry, ecology, mathematics, ethics and philosophy are implementation truths behind the world, not subject buttons in the player UI.

## Four acts

### 1. Wake the Root Lanterns

The player tunes cellular resource conditions, restores transport flow and physically balances two irrigation channels. The chapter cannot release its world-state transition until both persistent biological evidence and the mathematics mastery gate are satisfied.

Visible consequence: root lanterns begin pulsing, water starts moving through the root network and previously dormant organism nodes respond.

### 2. Clean the Sleeping Water

The player changes a virtual solution until saturation becomes visible, then balances a virtual mineral transformation so the whole modeled reaction closes. This uses the deterministic M5.3 simulation only; it contains no real-world chemistry procedure.

Visible consequence: suspended crystals settle, soil darkens and the water loop begins circulating again.

### 3. Bring Back the Messengers

The player changes one environmental condition at a time to identify a plant growth bottleneck, then compares flowering with and without pollinator availability to restore the fruiting link.

Visible consequence: flowers become active habitat, pollinator routes return and fruit starts forming.

### 4. Choose What the Thaw Becomes

The player must reason across affected perspectives and acknowledge a real tradeoff before the shared restoration plan can proceed. A final counterexample then breaks the simple theory that one visible failure caused the winter. The player revises the causal model toward a network explanation.

Visible consequence: the seasonal mechanism releases, ice retreats and the living garden changes state.

## Evidence versus mastery

M5.6C intentionally distinguishes two layers:

- **minimized persistent evidence**: biology, chemistry, ecology and ethics continue through the existing Mission/Epic evidence contract;
- **session mastery gate**: mathematics and philosophy are structurally necessary to play but are represented only by stable, bounded session gates. They do not expand the persistent child profile.

This is not a loophole. A mastery gate must be validated by an executable mechanic before the final world-state action becomes available. The mathematics gate checks proportional irrigation flow. The philosophy gate evaluates a structured argument revision after a counterexample.

## Game-first rules

- **No school UI.** No lessons, quizzes, grades, worksheets, scoreboards or subject tabs are part of the player surface.
- Rewards are **intrinsic**: thaw, motion, color, sound, access, fruiting and the restoration of systems.
- Failure is **reversible**. A wrong configuration changes or fails to change the world; it does not remove currency, lives or progress.
- Hints are player-requested and progressive. Asking for help creates no score penalty or lasting deficit label.
- Every act ends in a **visible consequence**. A hidden ledger entry alone can never advance the Garden.
- The click itself is never evidence. World actors only advance a bounded mechanism state; trusted C++ simulation or thought runtimes decide whether it is valid.
- No loot boxes, streak pressure, FOMO, artificial scarcity or chance rewards.

## Privacy and authority

The experience preserves `stable-ids-no-child-free-text`. It does not collect child-authored prose, voice transcripts, ideology labels or personality scores. Ethical evaluation measures the structure of reasoning — supported reasons, multiple perspectives and acknowledged tradeoffs — rather than enforcing a morally correct answer.

The final causal-revision problem is Garden-specific. It contrasts a single-cause explanation with a network-causality explanation and requires revision after a failed single-system restoration. The result is used as a session mastery gate, not a personality or belief label.

## Source implementation

M5.6C adds:

- canonical and packaged `garden-end-winter-player-experience-v1.json`;
- `FWMGardenExperienceCatalog` and progressive hint runtime;
- `UWMGardenEndWinterExperienceSubsystem` with active-chapter visibility, mastery gates and fail-closed world-state transitions;
- `AWMGardenEndWinterInteractableActor` routed through the normal `IWMInteractable` focus loop;
- `FWMGardenSystemsRuntime` using cell, solution, reaction and plant simulation from M5.3;
- Garden-aware thought routing plus a Garden-specific causal philosophy problem;
- automation tests and `validate-m5-6c-garden-end-winter.py`.

## Source-complete boundary

Source-complete means the contracts, deterministic mechanics, world-space proxy interactions, authority boundaries, tests and CI gates exist. It does **not** claim final authored garden art, final water/plant VFX, audio, animation, native Unreal execution or representative-device certification. Those remain part of the native/art certification boundary.
