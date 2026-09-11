#!/usr/bin/env python3
"""Import the first-person authored FBX set into the stable World Makers asset paths.

This script is intended to run inside Unreal Editor Python. It never changes the source
manifest's `authoredPresent` flags; native import and human review remain separate steps.
"""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

import unreal


ASSETS = {
    "arms.firstperson": ("SK_WM_FirstPersonArms", "/Game/WorldMakers/Characters/FirstPerson", True),
    "tool.scanner": ("SM_WM_Scanner", "/Game/WorldMakers/Tools/Scanner", False),
    "tool.build": ("SM_WM_BuildTool", "/Game/WorldMakers/Tools/Build", False),
    "tool.measure": ("SM_WM_MeasureTool", "/Game/WorldMakers/Tools/Measure", False),
    "device.wrist": ("SM_WM_WristDevice", "/Game/WorldMakers/Tools/Wrist", False),
}

ANIMATIONS = {
    "tool-raise": "A_FP_ToolRaise",
    "tool-lower": "A_FP_ToolLower",
    "scan-anticipate": "A_FP_ScanAnticipate",
    "scan-hold": "A_FP_ScanHold",
    "scan-settle": "A_FP_ScanSettle",
    "build-point": "A_FP_BuildPoint",
    "build-confirm": "A_FP_BuildConfirm",
    "measure-focus": "A_FP_MeasureFocus",
    "observe-focus": "A_FP_ObserveFocus",
}


def arguments():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--fbx-dir", required=True)
    parser.add_argument("--report", required=True)
    return parser.parse_args(argv)


def import_base(filename: Path, destination_path: str, destination_name: str, skeletal: bool, skeleton=None):
    task = unreal.AssetImportTask()
    task.filename = str(filename)
    task.destination_path = destination_path
    task.destination_name = destination_name
    task.automated = True
    task.replace_existing = True
    task.save = True

    options = unreal.FbxImportUI()
    options.import_mesh = True
    options.import_as_skeletal = skeletal
    options.import_animations = False
    options.mesh_type_to_import = unreal.FBXImportType.FBXIT_SKELETAL_MESH if skeletal else unreal.FBXImportType.FBXIT_STATIC_MESH
    if skeletal and skeleton is not None:
        options.skeleton = skeleton
    task.options = options
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    return unreal.EditorAssetLibrary.load_asset(f"{destination_path}/{destination_name}")


def import_lod(asset, filename: Path, lod_index: int, skeletal: bool):
    try:
        if skeletal and hasattr(unreal, "EditorSkeletalMeshLibrary") and hasattr(unreal.EditorSkeletalMeshLibrary, "import_lod"):
            return bool(unreal.EditorSkeletalMeshLibrary.import_lod(asset, lod_index, str(filename)))
        if not skeletal and hasattr(unreal, "EditorStaticMeshLibrary") and hasattr(unreal.EditorStaticMeshLibrary, "import_lod"):
            return bool(unreal.EditorStaticMeshLibrary.import_lod(asset, lod_index, str(filename)))
    except Exception as exc:
        unreal.log_warning(f"LOD import failed for {filename}: {exc}")
    return False


def lod_count(asset):
    for method_name in ("get_num_lods", "get_lod_num"):
        method = getattr(asset, method_name, None)
        if callable(method):
            try:
                return int(method())
            except Exception:
                pass
    try:
        lod_info = asset.get_editor_property("lod_info")
        return len(lod_info) if lod_info is not None else 1
    except Exception:
        return 1


def import_animation(filename: Path, asset_name: str, skeleton):
    task = unreal.AssetImportTask()
    task.filename = str(filename)
    task.destination_path = "/Game/WorldMakers/Animations/FirstPerson"
    task.destination_name = asset_name
    task.automated = True
    task.replace_existing = True
    task.save = True

    options = unreal.FbxImportUI()
    options.import_mesh = False
    options.import_animations = True
    options.mesh_type_to_import = unreal.FBXImportType.FBXIT_ANIMATION
    options.skeleton = skeleton
    options.anim_sequence_import_data.import_custom_attribute = False
    task.options = options
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    return unreal.EditorAssetLibrary.load_asset(f"/Game/WorldMakers/Animations/FirstPerson/{asset_name}")


def main():
    args = arguments()
    fbx_dir = Path(args.fbx_dir)
    report_path = Path(args.report)
    report_path.parent.mkdir(parents=True, exist_ok=True)

    report = {
        "schemaVersion": 1,
        "status": "imported-pending-human-review",
        "assets": {},
        "animations": {},
        "humanReviewApproved": False,
        "deformationReviewApproved": False,
        "tabletReviewApproved": False,
    }

    arms = None
    for slot_id, (asset_name, destination, skeletal) in ASSETS.items():
        base_file = fbx_dir / f"{asset_name}_LOD0.fbx"
        asset = import_base(base_file, destination, asset_name, skeletal)
        imported_lods = [0] if asset else []
        if asset:
            for lod_index in (1, 2):
                lod_file = fbx_dir / f"{asset_name}_LOD{lod_index}.fbx"
                if lod_file.is_file() and import_lod(asset, lod_file, lod_index, skeletal):
                    imported_lods.append(lod_index)
            unreal.EditorAssetLibrary.save_loaded_asset(asset)
        report["assets"][slot_id] = {
            "assetPath": f"{destination}/{asset_name}",
            "exists": bool(asset),
            "importedLods": imported_lods,
            "lodCountObserved": lod_count(asset) if asset else 0,
        }
        if slot_id == "arms.firstperson":
            arms = asset

    skeleton = arms.get_editor_property("skeleton") if arms else None
    for action_id, asset_name in ANIMATIONS.items():
        animation_file = fbx_dir / f"{asset_name}.fbx"
        animation = import_animation(animation_file, asset_name, skeleton) if skeleton and animation_file.is_file() else None
        report["animations"][action_id] = {
            "assetPath": f"/Game/WorldMakers/Animations/FirstPerson/{asset_name}",
            "exists": bool(animation),
        }

    report["allFiveAssetsImported"] = all(item["exists"] for item in report["assets"].values()) and len(report["assets"]) == 5
    report["allNineAnimationsImported"] = all(item["exists"] for item in report["animations"].values()) and len(report["animations"]) == 9
    report["allSourceImportsComplete"] = report["allFiveAssetsImported"] and report["allNineAnimationsImported"]
    report_path.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    unreal.log(f"First-person authored import report written to {report_path}")


if __name__ == "__main__":
    main()
