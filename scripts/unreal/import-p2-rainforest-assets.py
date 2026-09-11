"""Run inside Unreal Editor 5.8.2 to import the P2 rainforest FBX LOD payload.

Environment:
  WM_P2_FBX_DIR       directory produced by export-p2-rainforest-fbx.py
  WM_P2_IMPORT_REPORT optional JSON report path

Import does NOT set P1 authoredPresent=true. Human/native review remains required.
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


def import_base_mesh(filename: Path, package_path: str, asset_name: str):
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
    sm = options.static_mesh_import_data
    sm.combine_meshes = True
    sm.auto_generate_collision = False
    sm.generate_lightmap_u_vs = False
    task.options = options

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    return unreal.EditorAssetLibrary.load_asset(f"{package_path}/{asset_name}")


def main():
    root = repo_root()
    fbx_dir = Path(os.environ.get("WM_P2_FBX_DIR", "")).resolve()
    if not fbx_dir.is_dir():
        raise RuntimeError("WM_P2_FBX_DIR must point to the P2 FBX payload")

    p1 = load_json(root / "content/visual/authored/authored-assets-p1.json")
    p2 = load_json(root / "content/visual/authored/rainforest-p2-source-pack.json")
    p1_by_id = {a["id"]: a for a in p1["assets"]}
    static_mesh_subsystem = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    report = {"schemaVersion": 1, "assets": [], "readyForAuthoredPresentReview": True}

    for source in p2["assets"]:
        target = p1_by_id[source["id"]]
        object_path = target["objectPath"]
        package_asset = object_path.rsplit(".", 1)[0]
        package_path, asset_name = package_asset.rsplit("/", 1)
        stem = Path(source["filename"]).stem
        lod0 = fbx_dir / f"{stem}_LOD0.fbx"
        if not lod0.exists():
            raise RuntimeError(f"missing base FBX: {lod0}")

        mesh = import_base_mesh(lod0, package_path, asset_name)
        if not isinstance(mesh, unreal.StaticMesh):
            raise RuntimeError(f"failed to import StaticMesh {source['id']}")

        for lod_index in range(1, int(source["lodCount"])):
            lod_path = fbx_dir / f"{stem}_LOD{lod_index}.fbx"
            if not lod_path.exists():
                raise RuntimeError(f"missing LOD FBX: {lod_path}")
            imported = static_mesh_subsystem.import_lod(mesh, lod_index, str(lod_path))
            if imported != lod_index:
                raise RuntimeError(f"{source['id']}: ImportLOD({lod_index}) returned {imported}")

        lod_count = int(mesh.get_num_lods())
        material_count = len(mesh.get_editor_property("static_materials"))
        uv0_channels = int(mesh.get_num_uv_channels(0))
        triangles_lod0 = int(mesh.get_num_triangles(0))
        passed = (
            lod_count >= int(source["lodCount"])
            and material_count <= int(source["materialSlots"])
            and uv0_channels >= 1
            and triangles_lod0 > 0
        )
        if not passed:
            report["readyForAuthoredPresentReview"] = False
        report["assets"].append({
            "id": source["id"],
            "objectPath": object_path,
            "lodCount": lod_count,
            "materialSlots": material_count,
            "uvChannelsLOD0": uv0_channels,
            "trianglesLOD0": triangles_lod0,
            "passedNativeImportContract": passed,
        })
        unreal.EditorAssetLibrary.save_loaded_asset(mesh, only_if_is_dirty=False)

    report_path = Path(os.environ.get("WM_P2_IMPORT_REPORT", str(root / "artifacts/p2-rainforest-import-report.json")))
    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    if not report["readyForAuthoredPresentReview"]:
        raise RuntimeError("P2 native import contract failed; see report")
    print(f"P2 native import contract passed for {len(report['assets'])} rainforest assets. Human visual review still required.")


if __name__ == "__main__":
    main()
