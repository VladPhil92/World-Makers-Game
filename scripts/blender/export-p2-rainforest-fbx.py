#!/usr/bin/env python3
"""Run inside Blender: convert generated P2 OBJ source meshes into one FBX per LOD.

Usage:
  blender --background --python scripts/blender/export-p2-rainforest-fbx.py -- \
    --input-dir <GeneratedP2> --output-dir <FbxP2>
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

import bpy


def parse_args():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    p = argparse.ArgumentParser()
    p.add_argument("--input-dir", required=True)
    p.add_argument("--output-dir", required=True)
    return p.parse_args(argv)


def clear_scene():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)


def import_obj(path: Path):
    if hasattr(bpy.ops.wm, "obj_import"):
        bpy.ops.wm.obj_import(filepath=str(path), forward_axis="X", up_axis="Z")
    else:
        bpy.ops.import_scene.obj(filepath=str(path), axis_forward="X", axis_up="Z")


def export_lod(objects, out_path: Path):
    bpy.ops.object.select_all(action="DESELECT")
    for obj in objects:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = objects[0]
    if len(objects) > 1:
        bpy.ops.object.join()
    joined = bpy.context.view_layer.objects.active
    joined.name = out_path.stem
    bpy.ops.export_scene.fbx(
        filepath=str(out_path),
        use_selection=True,
        apply_unit_scale=True,
        apply_scale_options="FBX_SCALE_UNITS",
        axis_forward="X",
        axis_up="Z",
        add_leaf_bones=False,
        bake_anim=False,
        object_types={"MESH"},
    )


def main():
    args = parse_args()
    src, dst = Path(args.input_dir), Path(args.output_dir)
    dst.mkdir(parents=True, exist_ok=True)
    for obj_path in sorted(src.glob("SM_WM_RF_*.obj")):
        clear_scene()
        import_obj(obj_path)
        by_lod = {}
        for obj in list(bpy.context.scene.objects):
            if obj.type != "MESH":
                continue
            match = re.search(r"_LOD(\d+)(?:_|$)", obj.name)
            if not match:
                raise RuntimeError(f"{obj_path.name}: object without LOD tag: {obj.name}")
            by_lod.setdefault(int(match.group(1)), []).append(obj)
        if not by_lod or sorted(by_lod) != list(range(max(by_lod) + 1)):
            raise RuntimeError(f"{obj_path.name}: non-contiguous LOD groups {sorted(by_lod)}")
        # Export from high LOD to low LOD. Re-import the OBJ before each export because join() mutates the scene.
        lod_indices = sorted(by_lod)
        for lod_index in lod_indices:
            clear_scene()
            import_obj(obj_path)
            current = []
            for obj in list(bpy.context.scene.objects):
                match = re.search(r"_LOD(\d+)(?:_|$)", obj.name)
                if obj.type == "MESH" and match and int(match.group(1)) == lod_index:
                    current.append(obj)
            export_lod(current, dst / f"{obj_path.stem}_LOD{lod_index}.fbx")
        print(f"P2 FBX export: {obj_path.stem} -> {len(lod_indices)} LOD files")


if __name__ == "__main__":
    main()
