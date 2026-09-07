#!/usr/bin/env python3
"""Cheap structural checks for the M1 building prototype when Unreal is unavailable."""

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]

REQUIRED_FILES = [
    "game/Source/WorldMakers/Building/WMBuildPieceActor.h",
    "game/Source/WorldMakers/Building/WMBuildPieceActor.cpp",
    "game/Source/WorldMakers/Building/WMBuildGridLibrary.h",
    "game/Source/WorldMakers/Building/WMBuildGridLibrary.cpp",
    "game/Source/WorldMakers/Building/WMBuildingComponent.h",
    "game/Source/WorldMakers/Building/WMBuildingComponent.cpp",
    "game/Source/WorldMakers/Building/WMWorldSaveGame.h",
    "game/Source/WorldMakers/Player/WMPlayerCharacter.h",
    "game/Source/WorldMakers/Player/WMPlayerCharacter.cpp",
    "game/Source/WorldMakers/Game/WMGameMode.h",
    "game/Source/WorldMakers/Game/WMGameMode.cpp",
    "game/Source/WorldMakers/Private/Tests/WMBuildGridTests.cpp",
    "game/Config/DefaultInput.ini",
]

errors: list[str] = []

for relative in REQUIRED_FILES:
    if not (ROOT / relative).is_file():
        errors.append(f"missing required M1 file: {relative}")

engine_config = (ROOT / "game/Config/DefaultEngine.ini").read_text(encoding="utf-8")
if "GlobalDefaultGameMode=/Script/WorldMakers.WMGameMode" not in engine_config:
    errors.append("World Makers game mode is not configured as the global default")

input_config = (ROOT / "game/Config/DefaultInput.ini").read_text(encoding="utf-8")
for action in ("PlaceBuild", "RotateBuild", "RotateBuildBack", "RemoveBuild", "UndoBuild", "RedoBuild", "SaveWorld", "LoadWorld"):
    if f'ActionName="{action}"' not in input_config:
        errors.append(f"missing input action mapping: {action}")

building_cpp = (ROOT / "game/Source/WorldMakers/Building/WMBuildingComponent.cpp").read_text(encoding="utf-8")
for capability in ("TryPlaceCurrentPiece", "TryRemoveTargetPiece", "UndoLastAction", "RedoLastAction", "SaveWorld", "LoadWorld"):
    if f"UWMBuildingComponent::{capability}" not in building_cpp:
        errors.append(f"missing M1 building capability: {capability}")

save_header = (ROOT / "game/Source/WorldMakers/Building/WMWorldSaveGame.h").read_text(encoding="utf-8").lower()
for forbidden in ("email", "birth", "parentname", "childname", "phone"):
    if forbidden in save_header:
        errors.append(f"prototype save model must not contain child/parent PII field: {forbidden}")

if errors:
    print("M1 prototype validation failed:")
    for error in errors:
        print(f"- {error}")
    sys.exit(1)

print("M1 prototype structural validation passed")
