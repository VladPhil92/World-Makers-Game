#!/usr/bin/env python3
"""Source-level M1.7 tablet/child UX gates when Unreal device execution is unavailable."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def fail(message: str) -> None:
    raise SystemExit(message)


def main() -> None:
    required = [
        "game/Source/WorldMakers/Input/WMTouchGestureLibrary.h",
        "game/Source/WorldMakers/Input/WMTouchGestureLibrary.cpp",
        "game/Source/WorldMakers/UI/WMBuildHUDWidget.h",
        "game/Source/WorldMakers/UI/WMBuildHUDWidget.cpp",
        "game/Source/WorldMakers/Private/Tests/WMTouchGestureTests.cpp",
    ]
    for relative in required:
        if not (ROOT / relative).is_file():
            fail(f"missing M1.7 source: {relative}")

    player = (ROOT / "game/Source/WorldMakers/Player/WMPlayerCharacter.cpp").read_text(encoding="utf-8")
    for token in (
        "BindTouch(IE_Pressed",
        "BindTouch(IE_Repeat",
        "BindTouch(IE_Released",
        "HasExceededDragDeadZone",
        "TouchLookDegreesPerPixel",
        "CreateWidget<UWMBuildHUDWidget>",
    ):
        if token not in player:
            fail(f"M1.7 player input missing token: {token}")

    pressed_start = player.find("void AWMPlayerCharacter::HandleTouchPressed")
    repeat_start = player.find("void AWMPlayerCharacter::HandleTouchRepeat")
    if pressed_start < 0 or repeat_start < 0 or repeat_start <= pressed_start:
        fail("touch handler boundaries could not be verified")
    pressed_body = player[pressed_start:repeat_start]
    if "PlaceBuild();" in pressed_body or "TryPlaceCurrentPiece" in pressed_body:
        fail("raw touch press must never place a build piece")

    hud = (ROOT / "game/Source/WorldMakers/UI/WMBuildHUDWidget.cpp").read_text(encoding="utf-8")
    for token in (
        "TouchTargetWidth",
        "TouchTargetHeight",
        "LOCTEXT(",
        "HandleConfirm",
        "HandleCancel",
        "HandlePreviousPiece",
        "HandleNextPiece",
        "SetIsEnabled(bPlacementValid)",
        "IsMoveInProgress",
    ):
        if token not in hud:
            fail(f"M1.7 HUD contract missing token: {token}")

    header = (ROOT / "game/Source/WorldMakers/UI/WMBuildHUDWidget.h").read_text(encoding="utf-8")
    if "TouchTargetWidth = 112.0f" not in header or "TouchTargetHeight = 72.0f" not in header:
        fail("M1.7 touch targets must retain the large prototype sizing baseline")

    build_rules = (ROOT / "game/Source/WorldMakers/WorldMakers.Build.cs").read_text(encoding="utf-8")
    for module in ('"UMG"', '"Slate"', '"SlateCore"'):
        if module not in build_rules:
            fail(f"M1.7 UI dependency missing: {module}")

    tests = (ROOT / "game/Source/WorldMakers/Private/Tests/WMTouchGestureTests.cpp").read_text(encoding="utf-8")
    if "WorldMakers.Input.Touch.DragDeadZone" not in tests:
        fail("M1.7 touch dead-zone automation test is missing")

    print("M1.7 tablet/child UX source validation passed")


if __name__ == "__main__":
    main()
