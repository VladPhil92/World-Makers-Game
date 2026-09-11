#!/usr/bin/env python3
"""Repository Quality gate for N2 Authored Activation & Release Candidate."""
from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CONTRACT = ROOT / "content/visual/authored/n2-authored-release-candidate.json"
CANONICAL = ROOT / "content/visual/authored/authored-assets-p1.json"
PACKAGED = ROOT / "game/Content/WorldMakers/Visual/Authored/authored-assets-p1.json"
PROVENANCE = ROOT / "content/visual/authored/n2-activation-provenance.json"
APPLIER = ROOT / "scripts/apply-n2-authored-activation.py"
ASSESSOR = ROOT / "scripts/assess-n2-release-candidate.py"
WORKFLOW = ROOT / ".github/workflows/n2-authored-release-candidate.yml"
DOC = ROOT / "docs/n2-authored-activation-release-candidate.md"
REPO_QUALITY = ROOT / ".github/workflows/repo-quality.yml"
BRIDGE_H = ROOT / "game/Source/WorldMakers/Visual/WMAuthoredVisualBridgeSubsystem.h"
BRIDGE_CPP = ROOT / "game/Source/WorldMakers/Visual/WMAuthoredVisualBridgeSubsystem.cpp"
ASSET_H = ROOT / "game/Source/WorldMakers/Visual/WMAuthoredAssetSubsystem.h"
ASSET_CPP = ROOT / "game/Source/WorldMakers/Visual/WMAuthoredAssetSubsystem.cpp"
VFX_H = ROOT / "game/Source/WorldMakers/Visual/WMVFXSubsystem.h"
VFX_CPP = ROOT / "game/Source/WorldMakers/Visual/WMVFXSubsystem.cpp"
BUILD_CS = ROOT / "game/Source/WorldMakers/WorldMakers.Build.cs"
UPROJECT = ROOT / "game/WorldMakers.uproject"


def require(condition: bool, message: str) -> None:
    if not condition:
        raise SystemExit("N2 validation failed: " + message)


def load(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def main() -> None:
    required_files = (
        CONTRACT, CANONICAL, PACKAGED, APPLIER, ASSESSOR, WORKFLOW, DOC, REPO_QUALITY,
        BRIDGE_H, BRIDGE_CPP, ASSET_H, ASSET_CPP, VFX_H, VFX_CPP, BUILD_CS, UPROJECT,
    )
    missing = [str(path.relative_to(ROOT)) for path in required_files if not path.is_file()]
    require(not missing, f"missing N2 files: {missing}")

    contract = load(CONTRACT)
    require(contract.get("schemaVersion") == 1 and contract.get("phase") == "N2", "invalid N2 contract identity")
    require(contract.get("states") == ["BLOCKED", "ACTIVATION_READY", "RELEASE_CANDIDATE"], "N2 state model drifted")
    require(contract.get("activationPolicy") == "human-reviewed-commit", "N2 activation must require a reviewed commit")
    require(contract.get("allOrNothing") is True, "N2 activation must remain all-or-nothing")
    require(contract.get("automaticRegistryCommit") is False, "N2 may not auto-commit registry activation")

    required = contract.get("required", {})
    require(required == {
        "p1RegistryAssets": 16,
        "rainforestAssets": 9,
        "animationAssets": 17,
        "vfxAssets": 17,
        "presentationAssets": 7,
    }, "N2 required-count contract drifted")

    rules = contract.get("activationRules", {})
    for key in (
        "n1StatusMustBeActivationCandidate",
        "candidateHashMustMatchApproval",
        "candidateMayOnlySetAuthoredPresentTrue",
        "canonicalAndPackagedRegistryMustMatch",
        "humanApprovalRequired",
        "rehearsalCannotBeReleaseCandidate",
        "committedModeRequiresAllAuthoredPresent",
    ):
        require(rules.get(key) is True, f"activation rule {key} must remain true")

    takeover = contract.get("runtimeTakeover", {})
    for key in (
        "authoredEnvironmentActive",
        "proceduralEnvironmentHidden",
        "authoredAvatarActive",
        "proceduralAvatarHidden",
        "authoredAnimationBlueprintActive",
        "authoredNiagaraPreferred",
        "proceduralVfxFallbackRetained",
        "nativePresentationAssetsRequired",
        "collisionAuthorityUnchanged",
        "gameplayAuthorityUnchanged",
    ):
        require(takeover.get(key) is True, f"runtime takeover rule {key} must remain true")

    require(CANONICAL.read_bytes() == PACKAGED.read_bytes(), "canonical and packaged P1 registries must be byte-equivalent")
    registry = load(CANONICAL)
    assets = registry.get("assets", [])
    require(len(assets) == 16, "N2 requires exactly 16 P1 registry assets")
    flags = [row.get("authoredPresent") for row in assets]
    all_false = all(flag is False for flag in flags)
    all_true = all(flag is True for flag in flags)
    require(all_false or all_true, "N2 forbids partial P1 activation")
    if all_false:
        require(not PROVENANCE.exists(), "fail-closed source branch must not contain committed activation provenance")
    else:
        require(PROVENANCE.is_file(), "activated registry requires n2-activation-provenance.json")
        provenance = load(PROVENANCE)
        require(provenance.get("phase") == "N2", "activation provenance phase mismatch")
        require(provenance.get("status") == "ACTIVATION_READY" and provenance.get("activationReady") is True, "activation provenance is not ready")
        require(provenance.get("activationMode") == "committed", "activated registry requires committed-mode provenance")
        require(provenance.get("humanApprovalRequired") is True, "human approval requirement was lost")

    subprocess.run([sys.executable, str(APPLIER), "--self-test"], check=True, cwd=ROOT)
    subprocess.run([sys.executable, str(ASSESSOR), "--self-test"], check=True, cwd=ROOT)

    asset_source = ASSET_H.read_text(encoding="utf-8") + "\n" + ASSET_CPP.read_text(encoding="utf-8")
    for token in ("LoadAnimationBlueprintClass", "FSoftClassPath", "AnimationBlueprint", "_C"):
        require(token in asset_source, f"authored AnimBP loader missing: {token}")

    bridge_source = BRIDGE_H.read_text(encoding="utf-8") + "\n" + BRIDGE_CPP.read_text(encoding="utf-8")
    for token in (
        "IsAuthoredAvatarActive",
        "IsAuthoredAnimationBlueprintActive",
        "IsFullAuthoredTakeoverActive",
        "WMN2TakeoverReport",
        "WMBuildCommit",
        "SetAnimInstanceClass",
        "bUseProceduralAvatarArt = false",
        "proceduralEnvironmentVisible",
        "proceduralAvatarActive",
        "authoredVfxReady",
        "authoredPresentationReady",
    ):
        require(token in bridge_source, f"N2 authored bridge missing takeover contract: {token}")

    vfx_source = VFX_H.read_text(encoding="utf-8") + "\n" + VFX_CPP.read_text(encoding="utf-8")
    for token in (
        "IsAuthoredNiagaraEnabled",
        "WasLastAcceptedEffectAuthored",
        "TrySpawnAuthoredNiagara",
        "UNiagaraFunctionLibrary::SpawnSystemAtLocation",
        "vfx.science.master",
        "User.Intensity",
        "User.MotionScale",
        "ActiveAuthoredEffects",
        "AWMProceduralVFXActor",
    ):
        require(token in vfx_source, f"N2 authored Niagara/fallback contract missing: {token}")
    require("Aced." not in vfx_source, "N2 VFX source contains invalid Accepted-event typo")

    build_text = BUILD_CS.read_text(encoding="utf-8")
    uproject = UPROJECT.read_text(encoding="utf-8")
    require('"Niagara"' in build_text and '"Niagara"' in uproject, "N2 must enable Niagara module/plugin")

    workflow = WORKFLOW.read_text(encoding="utf-8")
    for token in (
        "workflow_dispatch",
        "rehearsal",
        "committed",
        "actions/download-artifact@v4",
        "apply-n2-authored-activation.py",
        "collect-p5-native-inventory.py",
        "WMN2TakeoverReport",
        "assess-n2-release-candidate.py",
        "build-unreal.ps1",
        "upload-artifact",
    ):
        require(token in workflow, f"N2 workflow missing: {token}")

    repo_quality = REPO_QUALITY.read_text(encoding="utf-8")
    require("Validate N2 Authored Activation Release Candidate" in repo_quality, "Repository Quality does not execute N2 gate")

    docs = DOC.read_text(encoding="utf-8").replace("`", "").lower()
    for phrase in ("activation_ready", "release_candidate", "rehearsal", "committed", "human review", "runtime takeover", "procedural fallback", "p5", "v8"):
        require(phrase in docs, f"N2 documentation missing: {phrase}")

    mode = "fail-closed" if all_false else "committed authored activation"
    print(f"N2 authored release contract validated: {mode} registry, reviewed activation, runtime takeover, Niagara fallback and exact-build release assessment are wired.")


if __name__ == "__main__":
    main()
