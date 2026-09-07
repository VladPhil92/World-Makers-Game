#!/usr/bin/env python3
"""M1.5 certification-source gates for the World Makers Unreal prototype."""

from __future__ import annotations

import argparse
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CERTIFICATION_MAP = ROOT / "game/Content/WorldMakers/Maps/WM_PrototypeCertification.umap"


def fail(message: str) -> None:
    raise SystemExit(message)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--require-authored-map", action="store_true")
    args = parser.parse_args()

    project = json.loads((ROOT / "game/WorldMakers.uproject").read_text(encoding="utf-8"))
    if project.get("EngineAssociation") != "5.8":
        fail("WorldMakers.uproject must be locked to Unreal Engine 5.8")

    version = (ROOT / "game/UNREAL_ENGINE_VERSION").read_text(encoding="utf-8").strip()
    if version != "5.8.2":
        fail(f"Certification baseline must be UE 5.8.2, found {version!r}")

    component = (ROOT / "game/Source/WorldMakers/Building/WMBuildingComponent.cpp").read_text(encoding="utf-8")
    required_tokens = (
        "IsPlacementValid",
        "OverlapMultiByChannel",
        "MinPlacementSurfaceUpDot",
        "MaxSavedPieces",
        "IsSafeBuildTransform",
        "SnapLocationToSurfaceGrid",
    )
    missing = [token for token in required_tokens if token not in component]
    if missing:
        fail(f"M1.5 building hardening missing tokens: {missing}")

    tests = (ROOT / "game/Source/WorldMakers/Private/Tests/WMBuildGridTests.cpp").read_text(encoding="utf-8")
    for test_name in (
        "WorldMakers.Building.Grid.SnapLocation",
        "WorldMakers.Building.Grid.SnapSurfaceLocation",
        "WorldMakers.Building.Grid.SnapRotation",
    ):
        if test_name not in tests:
            fail(f"Missing Unreal automation test: {test_name}")

    if args.require_authored_map and not CERTIFICATION_MAP.is_file():
        fail(
            "Runtime certification requires the exact authored map "
            "game/Content/WorldMakers/Maps/WM_PrototypeCertification.umap"
        )

    status = "present" if CERTIFICATION_MAP.is_file() else "BLOCKED: exact certification .umap not yet authored in Unreal Editor"
    print(f"M1.5 source hardening passed. Certification map: {status}")


if __name__ == "__main__":
    main()
