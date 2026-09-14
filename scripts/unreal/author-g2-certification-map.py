#!/usr/bin/env python3
"""Author or reconcile the G2 certification map inside Unreal Editor.

This script must run through UnrealEditor-Cmd.exe / Unreal Editor Python. It never
pretends that a text file is a .umap; the map is created and saved by Unreal.
"""

from __future__ import annotations

import json
import os
from pathlib import Path
import traceback

import unreal

MAP_PACKAGE = "/Game/WorldMakers/Maps/WM_PrototypeCertification"
ACTORS = (
    ("WM_G2_PlayerStart", "/Script/Engine.PlayerStart", unreal.Vector(0.0, 0.0, 160.0)),
    ("WM_G2_Rainforest", "/Script/WorldMakers.WMCaribbeanRainforestPrototype", unreal.Vector(0.0, 0.0, 0.0)),
    ("WM_G2_MissionGeometry", "/Script/WorldMakers.WMMissionGeometryActor", unreal.Vector(0.0, 0.0, 0.0)),
)
GAME_MODE_CLASS = "/Script/WorldMakers.WMGameMode"
REPO_ROOT = Path(__file__).resolve().parents[2]
REPORT = Path(
    os.environ.get(
        "WM_G2_AUTHOR_REPORT",
        str(REPO_ROOT / "artifacts" / "g2-authored" / "g2-author-report.json"),
    )
)


def write_report(status: str, **extra: object) -> None:
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    payload = {
        "schema": "worldmakers.g2-author-report.v1",
        "status": status,
        "mapPackagePath": MAP_PACKAGE,
        **extra,
    }
    REPORT.write_text(json.dumps(payload, indent=2), encoding="utf-8")


def load_class(path: str):
    loaded = unreal.load_class(None, path)
    if not loaded:
        raise RuntimeError(f"Unable to load required class: {path}")
    return loaded


def ensure_level() -> None:
    if unreal.EditorAssetLibrary.does_asset_exist(MAP_PACKAGE):
        if not unreal.EditorLevelLibrary.load_level(MAP_PACKAGE):
            raise RuntimeError(f"Unable to load existing map {MAP_PACKAGE}")
        return

    if not unreal.EditorLevelLibrary.new_level(MAP_PACKAGE):
        raise RuntimeError(f"Unable to create map {MAP_PACKAGE}")


def actor_class_path(actor) -> str:
    cls = actor.get_class()
    return cls.get_path_name() if cls else ""


def ensure_actor(label: str, class_path: str, location: unreal.Vector):
    actor_class = load_class(class_path)
    existing = [a for a in unreal.EditorLevelLibrary.get_all_level_actors() if a.get_actor_label() == label]

    if len(existing) > 1:
        for duplicate in existing[1:]:
            unreal.EditorLevelLibrary.destroy_actor(duplicate)
        existing = existing[:1]

    actor = existing[0] if existing else None
    if actor is not None and actor_class_path(actor) != class_path:
        unreal.EditorLevelLibrary.destroy_actor(actor)
        actor = None

    if actor is None:
        actor = unreal.EditorLevelLibrary.spawn_actor_from_class(actor_class, location, unreal.Rotator())
        if actor is None:
            raise RuntimeError(f"Unable to spawn {class_path} for {label}")
        actor.set_actor_label(label, True)
    else:
        actor.set_actor_location(location, False, False)

    return actor


def main() -> None:
    ensure_level()
    game_mode = load_class(GAME_MODE_CLASS)

    world = unreal.EditorLevelLibrary.get_editor_world()
    if world is None:
        raise RuntimeError("Editor world is unavailable after loading the G2 map")

    world_settings = world.get_world_settings()
    if world_settings is None:
        raise RuntimeError("WorldSettings is unavailable")
    world_settings.set_editor_property("default_game_mode", game_mode)

    authored = []
    for label, class_path, location in ACTORS:
        actor = ensure_actor(label, class_path, location)
        actor_location = actor.get_actor_location()
        authored.append(
            {
                "label": label,
                "classPath": actor_class_path(actor),
                "location": [actor_location.x, actor_location.y, actor_location.z],
            }
        )

    if not unreal.EditorLevelLibrary.save_current_level():
        raise RuntimeError("Unreal failed to save the G2 certification level")

    write_report(
        "passed",
        gameModeClass=GAME_MODE_CLASS,
        actors=authored,
        note="Map authored by Unreal. Human route review and native asset takeover are still required before G2 certification.",
    )
    unreal.log(f"G2 certification map authored: {MAP_PACKAGE}")


try:
    main()
except Exception as exc:  # Unreal needs a persisted diagnostic before the process exits.
    write_report("failed", error=str(exc), traceback=traceback.format_exc())
    unreal.log_error(f"G2 map authoring failed: {exc}")
    raise
