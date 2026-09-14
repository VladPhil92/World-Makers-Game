#!/usr/bin/env python3
"""Fail-closed G3 visual fidelity and presentation assessor.

G3 intentionally stops before representative-device performance certification.
It requires G2, native authored inventory, native map inspection, human visual QA,
and hash-bound evidence from the same commit.
"""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CONTRACT = ROOT / "content/production/g3-visual-fidelity-v1.json"
P5_ASSESSOR = ROOT / "scripts/assess-p5-art-polish-certification.py"


def load_json(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8-sig"))


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def load_p5_module():
    spec = importlib.util.spec_from_file_location("wm_p5_assessor", P5_ASSESSOR)
    if spec is None or spec.loader is None:
        raise RuntimeError("Unable to load P5 assessor helpers")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def safe_payload(root: Path, rel: str) -> Path | None:
    if not isinstance(rel, str) or not rel or Path(rel).is_absolute() or ".." in Path(rel).parts:
        return None
    path = (root / rel).resolve()
    resolved_root = root.resolve()
    if path != resolved_root and resolved_root not in path.parents:
        return None
    return path


def add_reason(result: dict, message: str) -> None:
    result["reasons"].append(message)


def validate_g2(path: Path, expected_commit: str, result: dict) -> bool:
    if not path.is_file():
        add_reason(result, "G2 certification result is missing")
        return False
    try:
        payload = load_json(path)
    except Exception as exc:
        add_reason(result, f"G2 result is invalid JSON: {exc}")
        return False
    valid = True
    if payload.get("schema") != "worldmakers.g2-authored-vertical-slice-result.v1":
        valid = False; add_reason(result, "G2 result schema mismatch")
    if payload.get("status") != "CERTIFIED" or payload.get("certified") is not True:
        valid = False; add_reason(result, "G2 must be CERTIFIED before G3 can certify")
    if payload.get("repositoryCommit") != expected_commit:
        valid = False; add_reason(result, "G2 commit does not match G3 certification commit")
    result["g2CertifiedSameCommit"] = valid
    return valid


def validate_map_inspection(path: Path, expected_commit: str, result: dict) -> bool:
    if not path.is_file():
        add_reason(result, "G3 native map inspection is missing")
        return False
    try:
        payload = load_json(path)
    except Exception as exc:
        add_reason(result, f"G3 map inspection is invalid JSON: {exc}")
        return False
    valid = True
    if payload.get("schema") != "worldmakers.g3-native-map-inspection.v1":
        valid = False; add_reason(result, "G3 native map inspection schema mismatch")
    if payload.get("status") != "passed":
        valid = False; add_reason(result, "G3 native map inspection did not pass")
    if payload.get("buildCommit") != expected_commit:
        valid = False; add_reason(result, "G3 native map inspection commit mismatch")
    if payload.get("mapPackagePath") != "/Game/WorldMakers/Maps/WM_PrototypeCertification":
        valid = False; add_reason(result, "G3 inspected the wrong certification map")
    components = payload.get("lightingComponents") or {}
    for key, expected in (
        ("directionalLight", 1),
        ("skyLight", 1),
        ("skyAtmosphere", 1),
        ("heightFog", 1),
    ):
        if components.get(key) != expected:
            valid = False; add_reason(result, f"G3 map lighting component count invalid: {key}")
    result["nativeMapInspectionValid"] = valid
    return valid


def validate_review(path: Path, evidence_root: Path, expected_commit: str, result: dict) -> bool:
    contract = load_json(CONTRACT)
    quality = contract["qualityBar"]
    if not path.is_file():
        add_reason(result, "G3 human visual review is missing")
        return False
    try:
        review = load_json(path)
    except Exception as exc:
        add_reason(result, f"G3 review is invalid JSON: {exc}")
        return False

    valid = True
    if review.get("schemaVersion") != 1 or review.get("status") != "passed":
        valid = False; add_reason(result, "G3 review must be schemaVersion=1 and status=passed")
    if review.get("buildCommit") != expected_commit:
        valid = False; add_reason(result, "G3 review commit mismatch")
    if review.get("unrealVersion") != contract["engine"]["expectedVersion"]:
        valid = False; add_reason(result, "G3 review Unreal version mismatch")
    roles = set(review.get("reviewerRoles") or [])
    if not {"tech-art", "animation", "visual-qa"}.issubset(roles):
        valid = False; add_reason(result, "G3 review requires tech-art, animation and visual-qa roles")

    ml = review.get("materialsLightingReview") or {}
    for key in (
        "masterSurfaceApproved", "waterMaterialApproved", "exposureApproved",
        "singlePrimaryDirectionalSunConfirmed", "skyAtmosphereApproved",
        "skyLightApproved", "heightFogApproved", "shadowQualityApproved",
    ):
        if ml.get(key) is not True:
            valid = False; add_reason(result, f"materialsLightingReview.{key} must be true")

    env = review.get("environmentReview") or {}
    for key in (
        "authoredRainforestTakeoverApproved", "lodTransitionsApproved",
        "hlodAndCullingApproved", "foliageOverdrawApproved",
        "textureStreamingApproved", "noCriticalMipPopIn", "noPlaceholderGeometry",
    ):
        if env.get(key) is not True:
            valid = False; add_reason(result, f"environmentReview.{key} must be true")
    pool = env.get("texturePoolOverBudgetMB")
    if not isinstance(pool, (int, float)) or float(pool) < 0 or float(pool) > float(quality["environment"]["maxTexturePoolOverBudgetMB"]):
        valid = False; add_reason(result, "G3 texture pool is over budget or missing")

    anim = review.get("characterAnimationReview") or {}
    for key in (
        "authoredCharacterApproved", "animationBlueprintApproved", "ikApproved",
        "physicsAssetApproved", "deformationApproved", "cosmeticClippingApproved",
    ):
        if anim.get(key) is not True:
            valid = False; add_reason(result, f"characterAnimationReview.{key} must be true")
    slide = anim.get("maxObservedFootSlideCm")
    if not isinstance(slide, (int, float)) or float(slide) < 0 or float(slide) > float(quality["characterAnimation"]["maxObservedFootSlideCm"]):
        valid = False; add_reason(result, "Observed foot sliding exceeds G3 threshold")
    for key, allowed in (
        ("criticalDeformationCount", quality["characterAnimation"]["criticalDeformationCountAllowed"]),
        ("criticalCosmeticClippingCount", quality["characterAnimation"]["criticalCosmeticClippingCountAllowed"]),
    ):
        value = anim.get(key)
        if not isinstance(value, int) or value < 0 or value > int(allowed):
            valid = False; add_reason(result, f"characterAnimationReview.{key} exceeds allowed count")

    vfx = review.get("vfxReview") or {}
    for key in ("allRequiredEffectsPresent", "semanticParityApproved", "reducedMotionApproved", "overdrawApproved"):
        if vfx.get(key) is not True:
            valid = False; add_reason(result, f"vfxReview.{key} must be true")

    presentation = review.get("presentationReview") or {}
    for key in (
        "cameraDataApproved", "levelSequencesApproved", "uiMotionStyleApproved",
        "cameraComfortApproved", "uiLegibilityApproved", "safeAreaApproved",
        "reducedMotionApproved", "noInputLock", "noForcedViewTarget", "noGlobalTimeScaleChange",
    ):
        if presentation.get(key) is not True:
            valid = False; add_reason(result, f"presentationReview.{key} must be true")
    duration = presentation.get("maxObservedRevealDurationSeconds")
    if not isinstance(duration, (int, float)) or float(duration) < 0 or float(duration) > float(quality["presentation"]["maxRevealDurationSeconds"]):
        valid = False; add_reason(result, "Observed reveal duration exceeds G3 ceiling")

    required_kinds = set(contract["requiredEvidenceKinds"])
    artifacts = review.get("artifacts") or []
    by_kind = {item.get("kind"): item for item in artifacts if isinstance(item, dict)}
    if set(by_kind) != required_kinds:
        valid = False; add_reason(result, "G3 review must contain exactly the required evidence kinds")
    for kind in required_kinds:
        item = by_kind.get(kind) or {}
        actual = safe_payload(evidence_root, item.get("file", ""))
        if actual is None or not actual.is_file():
            valid = False; add_reason(result, f"Missing/unsafe G3 evidence artifact: {kind}")
            continue
        expected_hash = item.get("sha256")
        if not isinstance(expected_hash, str) or len(expected_hash) != 64 or sha256_file(actual) != expected_hash.lower():
            valid = False; add_reason(result, f"G3 evidence SHA-256 mismatch: {kind}")

    result["humanVisualReviewValid"] = valid
    return valid


def assess(evidence_root: Path, expected_commit: str, g2_result: Path) -> dict:
    result = {
        "schema": "worldmakers.g3-visual-fidelity-result.v1",
        "gate": "G3",
        "status": "BLOCKED",
        "certified": False,
        "repositoryCommit": expected_commit,
        "reasons": [],
    }
    p5 = load_p5_module()
    g2_ok = validate_g2(g2_result, expected_commit, result)
    inventory_ok = p5.validate_inventory(evidence_root / "native-inventory.json", expected_commit, result)
    map_ok = validate_map_inspection(evidence_root / "g3-native-map-inspection.json", expected_commit, result)
    review_ok = validate_review(evidence_root / "g3-review.json", evidence_root, expected_commit, result)
    if g2_ok and inventory_ok and map_ok and review_ok and not result["reasons"]:
        result["status"] = "CERTIFIED"
        result["certified"] = True
    return result


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--evidence-root", required=True)
    parser.add_argument("--commit", required=True)
    parser.add_argument("--g2-result", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    if len(args.commit) != 40:
        raise SystemExit("--commit must be a full 40-character SHA")
    result = assess(Path(args.evidence_root), args.commit, Path(args.g2_result))
    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(f"World Makers G3 visual fidelity: {result['status']}")
    for reason in result["reasons"]:
        print(f"  - {reason}")
    return 0 if result["status"] == "CERTIFIED" else 1


if __name__ == "__main__":
    raise SystemExit(main())
