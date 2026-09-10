#!/usr/bin/env python3
"""Validate V7 camera, micro-cinematics and UI motion contracts."""

from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CANONICAL = ROOT / "content/visual/presentation/presentation-camera-ui-v7.json"
PACKAGED = ROOT / "game/Content/WorldMakers/Visual/presentation-camera-ui-v7.json"

EXPECTED_MODES = {"explore", "build", "observe", "science", "dialogue", "adventure-reveal"}
EXPECTED_CUES = {
    "presentation.build.confirm",
    "presentation.observe.focus",
    "presentation.science.focus",
    "presentation.adventure.reveal",
    "presentation.mission.changed",
    "presentation.ecology.changed",
}

REQUIRED_FILES = (
    "game/Source/WorldMakers/Visual/WMPresentationRuntime.h",
    "game/Source/WorldMakers/Visual/WMPresentationRuntime.cpp",
    "game/Source/WorldMakers/Visual/WMPresentationSubsystem.h",
    "game/Source/WorldMakers/Visual/WMPresentationSubsystem.cpp",
    "game/Source/WorldMakers/UI/WMPresentationOverlayWidget.h",
    "game/Source/WorldMakers/UI/WMPresentationOverlayWidget.cpp",
    "game/Source/WorldMakers/Private/Tests/WMPresentationRuntimeTests.cpp",
    "docs/v7-camera-cinematics-ui-motion.md",
)


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit(message)


def read(path: str) -> str:
    source = ROOT / path
    require(source.is_file(), f"Missing V7 file: {path}")
    return source.read_text(encoding="utf-8")


def require_tokens(path: str, *tokens: str) -> str:
    text = read(path)
    missing = [token for token in tokens if token not in text]
    require(not missing, f"{path} missing V7 contract tokens: {missing}")
    return text


def main() -> None:
    for path in REQUIRED_FILES:
        require((ROOT / path).is_file(), f"Missing V7 file: {path}")
    require(CANONICAL.is_file() and PACKAGED.is_file(), "V7 manifest source/package is incomplete")
    require(CANONICAL.read_bytes() == PACKAGED.read_bytes(), "Packaged V7 manifest must be byte-equivalent to canonical JSON")

    payload = json.loads(CANONICAL.read_text(encoding="utf-8"))
    require(payload.get("schemaVersion") == 1, "V7 schemaVersion must be 1")
    require(payload.get("runtimeId") == "presentation.camera-ui.v1", "V7 runtime identity drifted")
    require(payload.get("presentationOnly") is True, "V7 must remain presentation-only")
    require(payload.get("gameplayAuthority") is False, "V7 must not own gameplay authority")
    require(payload.get("inputLockingAllowed") is False, "V7 must not lock normal gameplay input")
    require(payload.get("forcedCameraRotationAllowed") is False, "V7 must not force player rotation")
    require(payload.get("cameraShakeRequired") is False, "V7 cannot require camera shake")
    require(payload.get("authoredSequenceAssetsPresent") is False, "V7 cannot claim absent Level Sequence assets")

    principles = payload.get("principles", {})
    for key in ("playerControlPreserved", "microbeatsOverLongCutscenes", "cameraCollisionPreserved", "supportsReducedMotion", "semanticVfxDrivenFraming", "uiMeaningWithoutMotion"):
        require(principles.get(key) is True, f"V7 principle must remain true: {key}")

    modes = payload.get("cameraModes", [])
    ids = [item.get("id") for item in modes]
    require(len(ids) == 6 and set(ids) == EXPECTED_MODES, f"V7 camera vocabulary drifted: {ids}")
    for item in modes:
        require(220 <= item.get("armLengthCm", 0) <= 800, f"Invalid arm length: {item}")
        require(50 <= item.get("fovDegrees", 0) <= 90, f"Invalid FOV: {item}")
        require(0 <= item.get("blendSeconds", -1) <= 1.5, f"Invalid blend: {item}")
        require(0 <= item.get("holdSeconds", -1) <= 3.0, f"Invalid hold: {item}")

    require(set(payload.get("uiCues", [])) == EXPECTED_CUES, "V7 UI cue vocabulary drifted")

    reduced = payload.get("reducedMotion", {})
    require(reduced.get("maxArmDeltaFromExploreCm") <= 80, "Reduced-motion arm delta ceiling drifted")
    require(reduced.get("maxFovDeltaFromExploreDegrees") <= 3, "Reduced-motion FOV ceiling drifted")
    require(reduced.get("maxOffsetZDeltaFromExploreCm") <= 16, "Reduced-motion offset ceiling drifted")
    require(reduced.get("maxBlendSeconds") <= 0.18, "Reduced-motion blend ceiling drifted")
    require(reduced.get("maxHoldSeconds") <= 0.70, "Reduced-motion hold ceiling drifted")
    require(reduced.get("uiTranslationPx") == 0, "Reduced-motion UI must remove translation")
    require(reduced.get("uiScale") == 1.0, "Reduced-motion UI must remove scale animation")

    beat = payload.get("cinematicBeat", {})
    require(beat.get("phases") == ["enter", "hold", "exit"], "V7 microbeat phases drifted")
    require(beat.get("maxDurationSeconds") <= 2.5, "V7 microbeats must remain short")
    require(beat.get("locksInput") is False, "V7 microbeats must not lock input")
    require(beat.get("changesGameplayTimeScale") is False, "V7 microbeats must not alter gameplay time")
    require(beat.get("forcesViewTarget") is False, "V7 microbeats must not force view target")

    boundary = payload.get("certificationBoundary", {})
    require(boundary.get("sourceRuntimeCanBeValidatedInCI") is True, "V7 source runtime should be CI-validatable")
    require(boundary.get("authoredLevelSequencesPresent") is False, "V7 must not claim authored Level Sequences")
    require(boundary.get("nativeCameraReviewRequired") is True, "V7 must preserve native camera review")
    require(boundary.get("motionSicknessReviewRequired") is True, "V7 must preserve motion-sickness review")
    require(boundary.get("representativeTabletReviewRequired") is True, "V7 must preserve device review")

    runtime = require_tokens(
        "game/Source/WorldMakers/Visual/WMPresentationRuntime.cpp",
        "EWMPresentationCameraMode::Build",
        "EWMPresentationCameraMode::Observe",
        "EWMPresentationCameraMode::Science",
        "EWMPresentationCameraMode::Dialogue",
        "EWMPresentationCameraMode::AdventureReveal",
        "gameplay.build.",
        "science.",
        "fantasy.",
        "80.0f",
        "3.0f",
        "16.0f",
        "0.18f",
    )
    require("SetViewTarget" not in runtime and "SetGlobalTimeDilation" not in runtime, "V7 runtime must not take cinematic authority")

    subsystem = require_tokens(
        "game/Source/WorldMakers/Visual/WMPresentationSubsystem.cpp",
        "OnVFXAccepted.AddDynamic",
        "OnVFXAccepted.RemoveDynamic",
        "bDoCollisionTest = true",
        "TargetArmLength",
        "TargetOffset",
        "SetFieldOfView",
        "GetActiveMissionId",
        "presentation.mission.changed",
        "SetReducedMotion",
    )
    for forbidden in ("SetViewTarget", "DisableInput", "SetIgnoreMoveInput", "SetIgnoreLookInput", "SetGlobalTimeDilation", "CustomTimeDilation"):
        require(forbidden not in subsystem, f"V7 subsystem contains forbidden control takeover: {forbidden}")

    vfx_h = require_tokens(
        "game/Source/WorldMakers/Visual/WMVFXSubsystem.h",
        "FWMVFXAcceptedSignature",
        "OnVFXAccepted",
    )
    vfx_cpp = require_tokens(
        "game/Source/WorldMakers/Visual/WMVFXSubsystem.cpp",
        "OnVFXAccepted.Broadcast",
    )
    require("WMPresentationSubsystem" not in vfx_h + vfx_cpp, "V6 must not depend back on V7")

    overlay = require_tokens(
        "game/Source/WorldMakers/UI/WMPresentationOverlayWidget.cpp",
        "SetRenderOpacity",
        "SetRenderTranslation",
        "SetRenderScale",
        "EvaluateUIMotion",
        "HitTestInvisible",
    )
    require("purchase" not in overlay.lower() and "shop" not in overlay.lower(), "V7 overlay must not become commerce pressure UI")

    tests = read("game/Source/WorldMakers/Private/Tests/WMPresentationRuntimeTests.cpp")
    for name in (
        "WorldMakers.Visual.Presentation.CameraProfilesRemainBounded",
        "WorldMakers.Visual.Presentation.ReducedMotionConstrainsCamera",
        "WorldMakers.Visual.Presentation.SemanticEventsSelectContext",
        "WorldMakers.Visual.Presentation.UIMotionPreservesMeaningWithoutTravel",
        "WorldMakers.Visual.Presentation.MicrobeatsStayShortAndStable",
    ):
        require(name in tests, f"Missing V7 automation test: {name}")
    require(tests.count("IMPLEMENT_SIMPLE_AUTOMATION_TEST") >= 5, "V7 requires at least five automation tests")

    docs = require_tokens(
        "docs/v7-camera-cinematics-ui-motion.md",
        "Presentation frames the player's action; it does not take ownership of it.",
        "Six stable camera modes",
        "enter -> hold -> exit",
        "authoredSequenceAssetsPresent=false",
        "V8 — Visual Optimization & Device Certification",
    )
    require("source-complete" in docs.lower(), "V7 docs must state source-complete boundary")

    roadmap = read("docs/visual-production-roadmap.md")
    require("V7 status: source-complete presentation-runtime pass" in roadmap, "Visual roadmap must mark V7 source status explicitly")
    require("## V8 — Visual Optimization & Device Certification" in roadmap, "Visual roadmap must preserve V8 next phase")

    workflow = read(".github/workflows/repo-quality.yml")
    require("python scripts/validate-v7-camera-ui-motion.py" in workflow, "Repository Quality must execute V7 gate")

    print("V7 presentation passed: six camera modes, short non-blocking reveals, semantic VFX framing, reduced-motion limits and UI motion are wired.")


if __name__ == "__main__":
    main()
