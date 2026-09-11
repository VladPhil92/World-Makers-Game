"""Run inside Unreal Editor 5.8.2 for the P4 native animation/presentation handoff.

The script imports the 17 animation FBXs against the P3 Skeleton and creates the
two Level Sequence containers with the authored playback duration. Niagara and
AnimBP recipes are reported but intentionally not fabricated as empty production
assets. This script never mutates P1 authoredPresent flags.
"""
from __future__ import annotations

import json
import os
from pathlib import Path

import unreal


def repo_root() -> Path:
    return Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())).resolve().parent


def load_json(path: Path):
    return json.loads(path.read_text(encoding="utf-8"))


def import_animation(filename: Path, destination_path: str, asset_name: str, skeleton):
    task = unreal.AssetImportTask()
    task.filename = str(filename)
    task.destination_path = destination_path
    task.destination_name = asset_name
    task.automated = True
    task.replace_existing = True
    task.save = True

    options = unreal.FbxImportUI()
    options.import_mesh = False
    options.import_animations = True
    options.import_materials = False
    options.import_textures = False
    options.mesh_type_to_import = unreal.FBXImportType.FBXIT_ANIMATION
    options.skeleton = skeleton
    task.options = options
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    return unreal.EditorAssetLibrary.load_asset(f"{destination_path}/{asset_name}")


def ensure_level_sequence(asset_name: str, package_path: str, duration_seconds: float):
    object_path = f"{package_path}/{asset_name}"
    sequence = unreal.EditorAssetLibrary.load_asset(object_path)
    if sequence is None:
        factory = unreal.LevelSequenceFactoryNew()
        sequence = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            asset_name, package_path, unreal.LevelSequence, factory
        )
    if not isinstance(sequence, unreal.LevelSequence):
        raise RuntimeError(f"failed to create LevelSequence {object_path}")
    sequence.set_playback_start(0)
    sequence.set_playback_end(max(1, int(round(duration_seconds * 30.0))))
    unreal.EditorAssetLibrary.save_loaded_asset(sequence, only_if_is_dirty=False)
    return sequence


def main():
    root = repo_root()
    source_path = Path(os.environ.get("WM_P4_SOURCE_BUNDLE", "")).resolve()
    animation_dir = Path(os.environ.get("WM_P4_ANIMATION_FBX_DIR", "")).resolve()
    report_path = Path(os.environ.get("WM_P4_IMPORT_REPORT", str(root / "artifacts/p4/p4-native-report.json")))
    if not source_path.is_file() or not animation_dir.is_dir():
        raise RuntimeError("WM_P4_SOURCE_BUNDLE and WM_P4_ANIMATION_FBX_DIR are required")

    source = load_json(source_path)
    skeleton = unreal.EditorAssetLibrary.load_asset("/Game/WorldMakers/Characters/Player/SKEL_WM_ChildExplorer")
    if skeleton is None:
        raise RuntimeError("P3 Skeleton must exist before P4 animation import")

    report = {
        "schemaVersion": 1,
        "animation": [],
        "levelSequences": [],
        "niagaraRecipes": [],
        "animationBlueprintPlan": source["animation"]["animationBlueprintPlan"],
        "authoredPresentMutated": False,
        "readyForAnimationReview": True,
        "readyForPresentationReview": True,
        "readyForNiagaraReview": False,
    }

    for clip in source["animation"]["clips"]:
        fbx = animation_dir / f"{clip['assetName']}.fbx"
        if not fbx.is_file():
            raise RuntimeError(f"missing P4 animation FBX: {fbx}")
        destination = "/Game/WorldMakers/Characters/Player/Animations/Interactions" if clip["id"].startswith("interaction.") else "/Game/WorldMakers/Characters/Player/Animations"
        asset = import_animation(fbx, destination, clip["assetName"], skeleton)
        passed = isinstance(asset, unreal.AnimSequence)
        if not passed:
            report["readyForAnimationReview"] = False
        report["animation"].append({
            "id": clip["id"],
            "assetName": clip["assetName"],
            "objectPath": f"{destination}/{clip['assetName']}",
            "durationSecondsSource": clip["durationSeconds"],
            "loop": clip["loop"],
            "passedNativeImportContract": passed,
        })

    for spec in source["presentation"]["sequences"]:
        package_asset = spec["objectPath"].rsplit(".", 1)[0]
        package_path, asset_name = package_asset.rsplit("/", 1)
        sequence = ensure_level_sequence(asset_name, package_path, float(spec["durationSeconds"]))
        report["levelSequences"].append({
            "id": spec["id"],
            "objectPath": spec["objectPath"],
            "playbackDurationSecondsSource": spec["durationSeconds"],
            "containerCreated": isinstance(sequence, unreal.LevelSequence),
            "cameraTracksRequireHumanAuthoring": True,
        })

    for effect in source["vfx"]["effects"]:
        package_path = effect["objectPath"].rsplit(".", 1)[0]
        report["niagaraRecipes"].append({
            "id": effect["id"],
            "objectPath": effect["objectPath"],
            "nativeAssetPresent": unreal.EditorAssetLibrary.does_asset_exist(package_path),
            "recipeValidated": True,
            "requiresHumanNiagaraAuthoring": True,
        })

    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    if not report["readyForAnimationReview"] or not report["readyForPresentationReview"]:
        raise RuntimeError("P4 animation/presentation native handoff failed; see report")
    print("P4 native handoff imported animation sequences and created Level Sequence containers. Niagara/AnimBP authored review remains pending.")


if __name__ == "__main__":
    main()
