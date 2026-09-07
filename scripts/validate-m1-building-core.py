#!/usr/bin/env python3
"""Validate the source-level M1.6 Building Core contract."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def require(path: str, *needles: str) -> None:
    text = (ROOT / path).read_text(encoding="utf-8")
    missing = [needle for needle in needles if needle not in text]
    if missing:
        raise SystemExit(f"{path}: missing M1.6 contract markers: {missing}")


def main() -> None:
    require(
        "game/Config/DefaultGame.ini",
        "prototype.cube",
        "prototype.floor",
        "prototype.wall",
        "prototype.pillar",
        "WMBuildCatalogSettings",
    )
    require(
        "game/Source/WorldMakers/Building/WMBuildCatalogSettings.h",
        "FWMBuildPieceSpec",
        "DimensionsCm",
        "bCanBeSupport",
        "FindPieceSpec",
    )
    require(
        "game/Source/WorldMakers/Building/WMBuildingComponent.cpp",
        "TryBeginMoveTargetPiece",
        "CommitMove",
        "EWMBuildCommandType::Move",
        "Spec.DimensionsCm",
        "ResolvePieceSpec(Record.PieceId",
        "FCollisionResponseParams::DefaultResponseParam",
    )
    require(
        "game/Source/WorldMakers/Building/WMBuildPieceActor.cpp",
        "ApplyPieceSpec",
        "SetPreviewValidity",
        "SetCustomDepthStencilValue",
    )
    require(
        "game/Source/WorldMakers/Private/Tests/WMBuildCatalogTests.cpp",
        "WorldMakers.Building.Catalog.PrototypePieces",
        "prototype.unknown",
    )
    require(
        "game/Config/DefaultInput.ini",
        "MoveBuild",
        "CycleBuildPiece",
        "CancelBuildEdit",
    )
    print("M1.6 Building Core source contract validated.")


if __name__ == "__main__":
    main()
