#!/usr/bin/env python3
"""Validate P5 art-polish and device-certification source contracts."""
from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
P5 = ROOT / "content/visual/authored/p5-art-polish-certification.json"
TEMPLATE = ROOT / "content/visual/certification/p5-art-polish-evidence.template.json"
P1 = ROOT / "content/visual/authored/authored-assets-p1.json"
P4 = ROOT / "content/visual/authored/p4-motion-vfx-presentation.json"
V8 = ROOT / "content/visual/certification/visual-certification-v8.json"
ASSESSOR = ROOT / "scripts/assess-p5-art-polish-certification.py"
COLLECTOR = ROOT / "scripts/unreal/collect-p5-native-inventory.py"
WORKFLOW = ROOT / ".github/workflows/p5-art-polish-device-certification.yml"
REPO_QUALITY = ROOT / ".github/workflows/repo-quality.yml"
DOC = ROOT / "docs/p5-art-polish-device-certification.md"
ROADMAP = ROOT / "docs/authored-production-roadmap.md"


def fail(message: str) -> None:
    raise SystemExit("P5 validation failed: " + message)


def require(condition: bool, message: str) -> None:
    if not condition:
        fail(message)


def load(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def main() -> None:
    required = (P5, TEMPLATE, P1, P4, V8, ASSESSOR, COLLECTOR, WORKFLOW, REPO_QUALITY, DOC, ROADMAP)
    missing = [str(path.relative_to(ROOT)) for path in required if not path.is_file()]
    require(not missing, f"missing required files: {missing}")

    p5, template, p1, p4, v8 = map(load, (P5, TEMPLATE, P1, P4, V8))
    require(p5.get("schemaVersion") == 1 and p5.get("phase") == "P5", "invalid P5 manifest identity")
    require(p5.get("unrealVersion") == v8.get("unrealVersion") == "5.8.2", "P5/V8 Unreal version contract drifted")
    require(p5["dependencies"]["authoredRegistry"].endswith("authored-assets-p1.json"), "P5 must depend on P1 registry")
    require(p5["dependencies"]["p4SourceManifest"].endswith("p4-motion-vfx-presentation.json"), "P5 must depend on P4 source manifest")

    targets = p5["polishTargets"]
    env_ids = set(targets["environment"]["requiredRainforestAssetIds"])
    p1_ids = {asset["id"] for asset in p1["assets"]}
    require(len(env_ids) == 9 and env_ids.issubset(p1_ids), "P5 rainforest polish set must cover all nine P1 environment assets")
    require(targets["environment"]["maxTexturePoolOverBudgetMB"] == 0, "P5 texture pool over-budget allowance must remain zero")
    require(targets["characterAnimation"]["requiredAnimationClipCount"] == len(p4["animation"]["requiredClipIds"]) == 17, "P5 animation review must cover all 17 P4 clips")
    require(0 < float(targets["characterAnimation"]["maxObservedFootSlideCm"]) <= 3.0, "P5 foot-slide threshold must stay at or below 3 cm")
    require(targets["characterAnimation"]["criticalDeformationCountAllowed"] == 0, "P5 cannot allow critical deformation")
    require(targets["characterAnimation"]["criticalCosmeticClippingCountAllowed"] == 0, "P5 cannot allow critical cosmetic clipping")
    require(targets["vfx"]["requiredEffectCount"] == len(p4["vfx"]["requiredEffectIds"]) == 17, "P5 VFX review must cover all 17 P4/V6 effects")
    gpu = targets["vfx"]["maxGpuP95MsByProfile"]
    require(set(gpu) == {"performance.tablet.low", "performance.tablet.medium", "performance.tablet.high"}, "P5 VFX GPU ceilings must cover all three tablet profiles")
    require(all(0 < float(value) <= 3.5 for value in gpu.values()), "P5 VFX GPU p95 ceilings are invalid")
    require(targets["presentation"]["maxRevealDurationSeconds"] <= 2.5, "P5 reveal duration cannot exceed V7 ceiling")
    require(targets["presentation"]["inputLockAllowed"] is False, "P5 cannot enable input locking")
    require(targets["presentation"]["forcedViewTargetAllowed"] is False, "P5 cannot enable forced ViewTarget")
    require(targets["presentation"]["globalTimeScaleChangeAllowed"] is False, "P5 cannot enable global time-scale changes")

    device = p5["deviceCertification"]
    require(set(device["requiredPlatforms"]) == {"Android", "iPadOS"}, "P5 requires Android and iPadOS")
    require(set(device["requiredProfiles"]) == {"performance.tablet.low", "performance.tablet.medium", "performance.tablet.high"}, "P5 requires Low/Mid/High profiles")
    require(device["requireEveryPlatformProfilePair"] is True, "P5 must require all six platform/profile packages")
    require(device["minimumFrameSamplesPerScenario"] >= int(v8["minimumFrameSamplesPerScenario"]), "P5 cannot weaken V8 sample count")
    require(set(device["requiredV8Scenarios"]) == set(v8["requiredScenarios"]), "P5 must certify the same four V8 scenarios")

    rules = p5["certificationRules"]
    for key in ("allP1AuthoredFlagsMustBeTrue", "allDeclaredP1UassetsMustExist", "nativeInventoryRequired", "humanPolishReviewRequired", "v8CertifiedEvidenceRequired", "allReviewArtifactsHashVerified", "certificationIsFailClosed"):
        require(rules.get(key) is True, f"P5 fail-closed rule missing: {key}")
    require(rules.get("sourceCiCanSelfCertify") is False, "source CI may not self-certify P5")
    require(p5["productionBoundary"]["nativeArtCertified"] is False and p5["productionBoundary"]["deviceCertified"] is False, "P5 source manifest must not claim native/device certification")

    require(template.get("status") == "pending", "P5 evidence template must stay pending")
    require(template.get("buildCommit") == "0" * 40, "P5 evidence template cannot claim a real build")
    for section in ("materialsLightingReview", "environmentReview", "characterAnimationReview", "vfxReview", "presentationReview"):
        require(section in template, f"P5 evidence template missing {section}")
    require({item.get("kind") for item in template.get("artifacts", [])} == {"visual-contact-sheet", "animation-review", "vfx-overdraw", "camera-ui-review"}, "P5 evidence template artifact set drifted")

    subprocess.run([sys.executable, str(ASSESSOR), "--self-test"], check=True, cwd=ROOT)

    collector = COLLECTOR.read_text(encoding="utf-8")
    for token in ("EditorAssetLibrary.does_asset_exist", "registryAssets", "animationAssets", "vfxAssets", "presentationAssets", "WM_BUILD_COMMIT", "WM_P4_SOURCE_BUNDLE"):
        require(token in collector, f"P5 native collector missing {token}")
    require("authoredPresent" not in collector, "P5 native collector may not mutate authoredPresent")

    workflow = WORKFLOW.read_text(encoding="utf-8")
    for token in ("self-hosted", "Windows", "X64", "unreal", "evidence_dir", "collect-p5-native-inventory.py", "assess-p5-art-polish-certification.py", "--require-certified", "${{ github.sha }}"):
        require(token in workflow, f"P5 native certification workflow missing {token}")

    repo_quality = REPO_QUALITY.read_text(encoding="utf-8")
    require("Validate P5 Art Polish Device Certification" in repo_quality, "Repository Quality must execute the P5 source gate")

    doc = DOC.read_text(encoding="utf-8").replace("`", "")
    for phrase in ("six platform/profile packages", "16 P1 targets", "17 AnimSequence", "17 Niagara", "BLOCKED", "Android", "iPadOS"):
        require(phrase.lower() in doc.lower(), f"P5 documentation missing: {phrase}")
    roadmap = ROADMAP.read_text(encoding="utf-8").replace("`", "")
    require("P5" in roadmap and "source-complete" in roadmap.lower() and "device certification" in roadmap.lower(), "Authored roadmap must record the P5 source/device boundary")

    authored_flags = [asset.get("authoredPresent") is True for asset in p1["assets"]]
    require(all(authored_flags) or not any(authored_flags), "P5/P1 activation state must be all-off before N2 or all-on after committed N2 activation; partial activation is forbidden")
    print("P5 art-polish/device-certification source contract validated: strict authored inventory, human polish evidence, six Android/iPadOS tier packages and V8 fail-closed certification are wired.")


if __name__ == "__main__":
    main()
