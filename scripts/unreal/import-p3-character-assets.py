"""Run inside Unreal Editor 5.8.2 to import the P3 Child Explorer FBX payload.

Environment:
  WM_P3_FBX_DIR       directory produced by build-p3-character-fbx.py
  WM_P3_IMPORT_REPORT optional JSON report path

This importer never changes P1 `authoredPresent`. Native deformation, sockets,
Physics Asset, IK Rig and device review remain explicit certification steps.
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


def split_object_path(object_path: str):
    package_asset = object_path.rsplit(".", 1)[0]
    return package_asset.rsplit("/", 1)


def import_skeletal_base(filename: Path, package_path: str, asset_name: str, skeleton=None, create_physics=False):
    task = unreal.AssetImportTask()
    task.filename = str(filename)
    task.destination_path = package_path
    task.destination_name = asset_name
    task.automated = True
    task.replace_existing = True
    task.save = True

    options = unreal.FbxImportUI()
    options.import_mesh = True
    options.import_as_skeletal = True
    options.import_materials = False
    options.import_textures = False
    options.import_animations = False
    options.mesh_type_to_import = unreal.FBXImportType.FBXIT_SKELETAL_MESH
    options.create_physics_asset = bool(create_physics)
    if skeleton is not None:
        options.skeleton = skeleton
    task.options = options
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    return unreal.EditorAssetLibrary.load_asset(f"{package_path}/{asset_name}")


def import_static_base(filename: Path, package_path: str, asset_name: str):
    task = unreal.AssetImportTask()
    task.filename = str(filename)
    task.destination_path = package_path
    task.destination_name = asset_name
    task.automated = True
    task.replace_existing = True
    task.save = True
    options = unreal.FbxImportUI()
    options.import_mesh = True
    options.import_as_skeletal = False
    options.import_materials = False
    options.import_textures = False
    options.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
    options.static_mesh_import_data.combine_meshes = True
    options.static_mesh_import_data.auto_generate_collision = False
    options.static_mesh_import_data.generate_lightmap_u_vs = False
    task.options = options
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    return unreal.EditorAssetLibrary.load_asset(f"{package_path}/{asset_name}")


def skeletal_lod_count(subsystem, mesh):
    if hasattr(subsystem, "get_lod_count"):
        return int(subsystem.get_lod_count(mesh))
    if hasattr(mesh, "get_lod_num"):
        return int(mesh.get_lod_num())
    return -1


def main():
    root = repo_root()
    fbx_dir = Path(os.environ.get("WM_P3_FBX_DIR", "")).resolve()
    if not fbx_dir.is_dir():
        raise RuntimeError("WM_P3_FBX_DIR must point to the P3 FBX payload")

    manifest = load_json(root / "content/visual/authored/character-p3-source-pack.json")
    skeletal_subsystem = unreal.get_editor_subsystem(unreal.SkeletalMeshEditorSubsystem)
    static_subsystem = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)

    report = {
        "schemaVersion": 1,
        "assets": [],
        "readyForHumanDeformationReview": True,
        "authoredPresentMutated": False,
    }

    body = next(m for m in manifest["modules"] if m["slot"] == "body")
    body_package, body_name = split_object_path(body["objectPath"])
    body_lod0 = fbx_dir / f"{body['assetName']}_LOD0.fbx"
    body_mesh = import_skeletal_base(body_lod0, body_package, body_name, create_physics=True)
    if not isinstance(body_mesh, unreal.SkeletalMesh):
        raise RuntimeError("failed to import P3 base SkeletalMesh")

    shared_skeleton = body_mesh.get_editor_property("skeleton")
    if shared_skeleton is None:
        raise RuntimeError("P3 body import did not create a Skeleton")

    for lod_index in range(1, int(body["lodCount"])):
        path = fbx_dir / f"{body['assetName']}_LOD{lod_index}.fbx"
        result = skeletal_subsystem.import_lod(body_mesh, lod_index, str(path))
        if result != lod_index:
            raise RuntimeError(f"body LOD{lod_index} import returned {result}")
    unreal.EditorAssetLibrary.save_loaded_asset(body_mesh, only_if_is_dirty=False)

    for module in manifest["modules"]:
        package_path, asset_name = split_object_path(module["objectPath"])
        lod0 = fbx_dir / f"{module['assetName']}_LOD0.fbx"
        if not lod0.exists():
            raise RuntimeError(f"missing P3 FBX: {lod0}")

        if module["slot"] == "body":
            asset = body_mesh
            lod_count = skeletal_lod_count(skeletal_subsystem, asset)
        elif module["kind"] == "SkeletalMesh":
            asset = import_skeletal_base(lod0, package_path, asset_name, skeleton=shared_skeleton)
            if not isinstance(asset, unreal.SkeletalMesh):
                raise RuntimeError(f"failed to import skeletal cosmetic {module['slot']}")
            for lod_index in range(1, int(module["lodCount"])):
                path = fbx_dir / f"{module['assetName']}_LOD{lod_index}.fbx"
                result = skeletal_subsystem.import_lod(asset, lod_index, str(path))
                if result != lod_index:
                    raise RuntimeError(f"{module['slot']} LOD{lod_index} import returned {result}")
            lod_count = skeletal_lod_count(skeletal_subsystem, asset)
        else:
            asset = import_static_base(lod0, package_path, asset_name)
            if not isinstance(asset, unreal.StaticMesh):
                raise RuntimeError(f"failed to import static cosmetic {module['slot']}")
            for lod_index in range(1, int(module["lodCount"])):
                path = fbx_dir / f"{module['assetName']}_LOD{lod_index}.fbx"
                result = static_subsystem.import_lod(asset, lod_index, str(path))
                if result != lod_index:
                    raise RuntimeError(f"{module['slot']} LOD{lod_index} import returned {result}")
            lod_count = int(asset.get_num_lods())

        passed = lod_count >= int(module["lodCount"])
        if not passed:
            report["readyForHumanDeformationReview"] = False
        report["assets"].append({
            "slot": module["slot"],
            "kind": module["kind"],
            "objectPath": module["objectPath"],
            "lodCount": lod_count,
            "expectedLodCount": module["lodCount"],
            "passedNativeImportContract": passed,
        })
        unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)

    report["skeletonCreated"] = shared_skeleton is not None
    report["physicsAssetReviewRequired"] = True
    report["ikRigReviewRequired"] = True
    report["skeletonContractPath"] = manifest["skeleton"]["skeletonObjectPath"]

    report_path = Path(os.environ.get(
        "WM_P3_IMPORT_REPORT",
        str(root / "artifacts/p3/p3-character-import-report.json"),
    ))
    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")

    if not report["readyForHumanDeformationReview"]:
        raise RuntimeError("P3 native import contract failed; see report")
    print(f"P3 native import completed for {len(report['assets'])} modules. Human deformation/socket/device review remains required.")


if __name__ == "__main__":
    main()
