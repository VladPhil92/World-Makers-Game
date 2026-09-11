#!/usr/bin/env python3
"""Collect fail-closed P5 native authored-asset inventory inside Unreal Editor."""
from __future__ import annotations

import json
import os
from pathlib import Path

import unreal

ROOT = Path(__file__).resolve().parents[2]
P1 = ROOT / "content/visual/authored/authored-assets-p1.json"
P3 = ROOT / "content/visual/authored/character-p3-source-pack.json"
P4 = ROOT / "content/visual/authored/p4-motion-vfx-presentation.json"
P5 = ROOT / "content/visual/authored/p5-art-polish-certification.json"
V7 = ROOT / "content/visual/presentation/presentation-camera-ui-v7.json"


def load(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def canonical_object_path(path: str) -> str:
    if "." in path.rsplit("/", 1)[-1]:
        return path
    name = path.rsplit("/", 1)[-1]
    return f"{path}.{name}"


def exists(object_path: str) -> bool:
    return bool(unreal.EditorAssetLibrary.does_asset_exist(canonical_object_path(object_path)))


def main() -> None:
    source_path = Path(os.environ.get("WM_P4_SOURCE_BUNDLE", ""))
    report_path = Path(os.environ.get("WM_P5_NATIVE_INVENTORY", ""))
    build_commit = os.environ.get("WM_BUILD_COMMIT", "")
    if not source_path.is_file():
        raise RuntimeError("WM_P4_SOURCE_BUNDLE must point to the generated P4 source bundle")
    if not str(report_path):
        raise RuntimeError("WM_P5_NATIVE_INVENTORY is required")
    if len(build_commit) != 40:
        raise RuntimeError("WM_BUILD_COMMIT must be a full 40-character commit SHA")

    p1, p3, p4, p5, v7, source = map(load, (P1, P3, P4, P5, V7, source_path))
    engine_raw = unreal.SystemLibrary.get_engine_version()
    target_version = p5["unrealVersion"]
    if not str(engine_raw).startswith(target_version):
        raise RuntimeError(f"P5 requires Unreal {target_version}; runner reports {engine_raw}")

    registry_assets = [
        {"id": asset["id"], "objectPath": asset["objectPath"], "exists": exists(asset["objectPath"])}
        for asset in p1["assets"]
    ]
    animation_assets = [
        {"id": clip["id"], "objectPath": clip["objectPath"], "exists": exists(clip["objectPath"])}
        for clip in source["animation"]["clips"]
    ]
    vfx_assets = [
        {"id": effect["id"], "objectPath": effect["objectPath"], "exists": exists(effect["objectPath"])}
        for effect in source["vfx"]["effects"]
    ]

    p1_by_id = {asset["id"]: asset for asset in p1["assets"]}
    presentation_targets = {
        "animation.player.blueprint": p1_by_id["animation.player.blueprint"]["objectPath"],
        "character.player.ik-rig": p3["skeleton"]["ikRigObjectPath"],
        "character.player.physics-asset": p3["skeleton"]["physicsAssetObjectPath"],
        "presentation.camera-data": p1_by_id["presentation.camera-data"]["objectPath"],
        "presentation.ui-motion-style": canonical_object_path(v7["authoredTargets"]["uiMotionStyle"]),
        "presentation.adventure-reveal": p4["presentation"]["adventureRevealObjectPath"],
        "presentation.science-reveal": p4["presentation"]["scienceRevealObjectPath"],
    }
    presentation_assets = [
        {"id": asset_id, "objectPath": object_path, "exists": exists(object_path)}
        for asset_id, object_path in sorted(presentation_targets.items())
    ]

    payload = {
        "schemaVersion": 1,
        "buildCommit": build_commit,
        "unrealVersion": target_version,
        "engineVersionRaw": str(engine_raw),
        "registryAssets": registry_assets,
        "animationAssets": animation_assets,
        "vfxAssets": vfx_assets,
        "presentationAssets": presentation_assets,
        "summary": {
            "registryPresent": sum(1 for row in registry_assets if row["exists"]),
            "registryRequired": len(registry_assets),
            "animationsPresent": sum(1 for row in animation_assets if row["exists"]),
            "animationsRequired": len(animation_assets),
            "vfxPresent": sum(1 for row in vfx_assets if row["exists"]),
            "vfxRequired": len(vfx_assets),
            "presentationPresent": sum(1 for row in presentation_assets if row["exists"]),
            "presentationRequired": len(presentation_assets),
        },
    }
    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
    unreal.log(f"P5 native inventory written to {report_path}")
    unreal.log(json.dumps(payload["summary"], sort_keys=True))


if __name__ == "__main__":
    main()
