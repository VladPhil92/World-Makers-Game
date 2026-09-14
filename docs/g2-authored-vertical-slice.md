# G2 — Authored Vertical Slice Foundation

Status: **SOURCE READY / NATIVE AUTHORSHIP REQUIRED**

G2 is the first World Makers production gate that requires a real Unreal-authored level. It converts the source/runtime systems proven by G0/G1 into one deterministic playable Caribbean Rainforest route without prematurely imposing the final G3 visual-polish bar.

## Gate boundary

G1 proves native code/runtime health: UE 5.8.2, `WorldMakersEditor Win64 Development` and the complete `WorldMakers.*` automation namespace.

G2 owns:

- the real `/Game/WorldMakers/Maps/WM_PrototypeCertification` level;
- stable map and asset paths;
- a deterministic first-person certification route;
- explicit authored placement of the rainforest prototype and mission geometry;
- native loadability and deliberate approval of the nine P2 rainforest asset families;
- a human PIE/editor review proving that the complete gameplay-learning loop is actually traversable.

G3 owns final lighting/material/animation/VFX quality, placeholder elimination, camera-comfort final review and Reduced Motion polish. G4 owns representative-device performance certification.

A source validator passing is **not** G2 certification.

## Certification map

Package:

`/Game/WorldMakers/Maps/WM_PrototypeCertification`

Repository binary:

`game/Content/WorldMakers/Maps/WM_PrototypeCertification.umap`

The `.umap` must be created by Unreal Engine and committed through Git LFS. Do not create a placeholder text file with the `.umap` extension.

The map contract requires exactly one authored actor for each of these roles:

- `WM_G2_PlayerStart` — `/Script/Engine.PlayerStart`;
- `WM_G2_Rainforest` — `/Script/WorldMakers.WMCaribbeanRainforestPrototype`;
- `WM_G2_MissionGeometry` — `/Script/WorldMakers.WMMissionGeometryActor`.

WorldSettings must use `/Script/WorldMakers.WMGameMode`.

The existing `AWMGameMode` runtime remains useful as a fallback, but an authored certification route must no longer depend on runtime spawning to prove that the level itself is correctly composed.

## One-command authoring

From an existing UE 5.8.2 Windows workstation:

```bat
WorldMakers-G2-Author.cmd
```

The launcher resolves the locked engine and executes:

`scripts/unreal/author-g2-certification-map.py`

inside Unreal Editor. The operation is idempotent: it loads an existing certification map when present, reconciles the required labelled actors, assigns `AWMGameMode`, and saves the level through Unreal.

Expected source change after authoring:

`game/Content/WorldMakers/Maps/WM_PrototypeCertification.umap`

After the command succeeds, open the map in the editor and review the composition before committing it.

## Rainforest authored takeover

G2 requires the nine environment families already defined by P1/P2:

1. ground;
2. terrain mound;
3. Tree A;
4. Tree B;
5. Tree C;
6. understory;
7. rock;
8. water edge;
9. hero ceiba.

The source pack and native import bridge already exist under P2. Run the P2 native Blender → FBX → Unreal import workflow when those assets have not yet been imported.

The P1 registry remains authoritative:

`content/visual/authored/authored-assets-p1.json`

G2 certification deliberately requires all nine environment entries to be `authoredPresent=true`. Do not flip those flags merely because the import script ran. First verify the actual native `.uasset`, silhouette, scale/orientation, UVs, LODs, material assignment and collision/fallback behavior in Unreal as required by `docs/p2-authored-rainforest-asset-pack.md`.

The procedural environment path remains a fallback; G2 approval means the authored render set is real and loadable, not that fallback code has been deleted.

## Native inspection

`scripts/unreal/inspect-g2-certification-map.py` loads the real map in Unreal and fail-closes unless it can prove:

- the correct WorldSettings GameMode;
- exactly one required actor for each G2 label;
- the expected native class for each required actor;
- every required rainforest P1 entry is deliberately `authoredPresent=true`;
- every required rainforest object path loads as a real Unreal asset.

Its canonical evidence is:

`artifacts/g2-authored/g2-native-map-inspection.json`

## Playable route review

Native map structure is necessary but insufficient. G2 also requires a real editor/PIE traversal of the complete route:

`spawn/control -> observe/scan -> collect -> science interaction -> craft/transform -> build/place -> visible ecosystem consequence -> mission evidence -> save/reload`

Copy:

`content/production/g2-route-review-template.json`

into:

`artifacts/g2-authored/g2-route-review.json`

only when performing the review. Record the exact repository commit, reviewer and UTC review time. Every capability and route check must pass and `criticalIssues` must be empty. The repository template intentionally remains `pending` and cannot certify anything by itself.

## One-command certification

After the map and approved native assets are committed through Git LFS on clean current `main`, and the route review evidence has been produced:

```bat
WorldMakers-G2-Certify.cmd
```

A real `CERTIFIED` result requires:

1. G1 returns `CERTIFIED` on the same clean current `main` commit;
2. `WM_PrototypeCertification.umap` exists and is tracked by Git LFS;
3. all nine P2/P1 rainforest entries are deliberately `authoredPresent=true`;
4. the Unreal-native map/asset inspection passes;
5. the manual route review has `status=passed`, all nine capabilities pass, all route checks pass, and the review commit equals `HEAD`;
6. local `HEAD` equals fetched `origin/main` and the worktree is clean.

The canonical result is:

`artifacts/g2-authored/g2-authored-vertical-slice.json`

Possible states are:

- `CERTIFIED` — full G2 evidence on clean current main;
- `NON_CERTIFYING_PASS` — checks passed under explicit development exceptions;
- `BLOCKED` — one or more G2 requirements are missing or failed.

## CI behavior

Hosted CI validates the G2 contract, route template, LFS policy, Unreal Python authoring/inspection wiring and integration with the existing asset registry. Hosted CI cannot create or certify a `.umap`.

The optional self-hosted UE 5.8.2 job may inspect an already committed authored map. It remains fail-closed and does not synthesize manual route-review evidence.

## Exit criterion

G2 is complete only when `WorldMakers-G2-Certify.cmd` returns `CERTIFIED` using the real Unreal-authored map and native assets.

Until then the correct project state is **G2 SOURCE READY / NATIVE AUTHORSHIP REQUIRED**. This distinction is intentional: it lets code, import tooling and level contracts advance without representing generated proxies or future asset paths as finished game art.
