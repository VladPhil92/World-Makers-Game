#!/usr/bin/env python3
"""Inspect the G3 certification map lighting/presentation prerequisites in Unreal Editor."""
from __future__ import annotations

import json
import os
from pathlib import Path
import traceback

import unreal

MAP_PACKAGE = "/Game/WorldMakers/Maps/WM_PrototypeCertification"
RAINFOREST_LABEL = "WM_G2_Rainforest"
EXPECTED_RAINFOREST_CLASS = "/Script/WorldMakers.WMCaribbeanRainforestPrototype"
ROOT = Path(__file__).resolve().parents[2]
REPORT = Path(
    os.environ.get(
        "WM_G3_MAP_INSPECTION",
        str(ROOT / "artifacts" / "g3-visual" / "g3-native-map-inspection.json"),
    )
)
BUILD_COMMIT = os.environ.get("WM_BUILD_COMMIT", "")


def write_report(status: str, blockers: list[str], **extra: object) -> None:
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(
        json.dumps(
            {
                "schema": "worldmakers.g3-native-map-inspection.v1",
                "status": status,
                "buildCommit": BUILD_COMMIT,
                "mapPackagePath": MAP_PACKAGE,
                "blockers": blockers,
                **extra,
            },
            indent=2,
        ) + "\n",
        encoding="utf-8",
    )


def component_count(actor, component_class) -> int:
    try:
        return len(actor.get_components_by_class(component_class))
    except Exception:
        return sum(1 for component in actor.get_components_by_class(unreal.ActorComponent) if component.is_a(component_class))


def main() -> None:
    blockers: list[str] = []
    if len(BUILD_COMMIT) != 40:
        raise RuntimeError("WM_BUILD_COMMIT must be a full 40-character SHA")
    engine = str(unreal.SystemLibrary.get_engine_version())
    if not engine.startswith("5.8.2"):
        blockers.append(f"G3 requires Unreal 5.8.2; editor reports {engine}")
    if not unreal.EditorAssetLibrary.does_asset_exist(MAP_PACKAGE):
        blockers.append(f"Certification map is missing: {MAP_PACKAGE}")
        write_report("blocked", blockers, engineVersionRaw=engine)
        raise RuntimeError(blockers[0])
    if not unreal.EditorLevelLibrary.load_level(MAP_PACKAGE):
        blockers.append(f"Unable to load certification map: {MAP_PACKAGE}")
        write_report("blocked", blockers, engineVersionRaw=engine)
        raise RuntimeError(blockers[0])

    actors = unreal.EditorLevelLibrary.get_all_level_actors()
    rainforest = [actor for actor in actors if actor.get_actor_label() == RAINFOREST_LABEL]
    if len(rainforest) != 1:
        blockers.append(f"Expected exactly one {RAINFOREST_LABEL}; found {len(rainforest)}")
        lighting = {"directionalLight": 0, "skyLight": 0, "skyAtmosphere": 0, "heightFog": 0}
    else:
        actor = rainforest[0]
        actual_class = actor.get_class().get_path_name()
        if actual_class != EXPECTED_RAINFOREST_CLASS:
            blockers.append(f"Rainforest actor class mismatch: {actual_class}")
        lighting = {
            "directionalLight": component_count(actor, unreal.DirectionalLightComponent),
            "skyLight": component_count(actor, unreal.SkyLightComponent),
            "skyAtmosphere": component_count(actor, unreal.SkyAtmosphereComponent),
            "heightFog": component_count(actor, unreal.ExponentialHeightFogComponent),
        }
        for key, count in lighting.items():
            if count != 1:
                blockers.append(f"Expected exactly one rainforest lighting component {key}; found {count}")

    status = "passed" if not blockers else "blocked"
    write_report(status, blockers, engineVersionRaw=engine, lightingComponents=lighting)
    if blockers:
        raise RuntimeError("G3 native map inspection blocked: " + " | ".join(blockers))
    unreal.log("World Makers G3 native map inspection: PASS")


try:
    main()
except Exception as exc:
    if not REPORT.exists():
        write_report("failed", [str(exc)], traceback=traceback.format_exc())
    unreal.log_error(f"G3 native map inspection failed: {exc}")
    raise
