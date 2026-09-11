#!/usr/bin/env python3
"""Fail-closed P5 authored-art polish and device certification assessor."""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
P5 = ROOT / "content/visual/authored/p5-art-polish-certification.json"
P1 = ROOT / "content/visual/authored/authored-assets-p1.json"
P4 = ROOT / "content/visual/authored/p4-motion-vfx-presentation.json"
V8_ASSESSOR = ROOT / "scripts/assess-v8-visual-certification.py"
REVIEW_FILE = "p5-review.json"
INVENTORY_FILE = "native-inventory.json"
REQUIRED_ARTIFACT_KINDS = {
    "visual-contact-sheet",
    "animation-review",
    "vfx-overdraw",
    "camera-ui-review",
}
PRESENTATION_NATIVE_TARGETS = {
    "animation.player.blueprint",
    "character.player.ik-rig",
    "character.player.physics-asset",
    "presentation.camera-data",
    "presentation.ui-motion-style",
    "presentation.adventure-reveal",
    "presentation.science-reveal",
}


def load_json(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def load_v8_module():
    spec = importlib.util.spec_from_file_location("wm_v8_assessor", V8_ASSESSOR)
    if spec is None or spec.loader is None:
        raise RuntimeError("Unable to load V8 assessor")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def object_path_to_uasset(object_path: str) -> Path:
    package = object_path.partition(".")[0]
    relative = package.removeprefix("/Game/") + ".uasset"
    return ROOT / "game/Content" / relative


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


def validate_repository_authored_state(result: dict) -> bool:
    registry = load_json(P1)
    complete = True
    for asset in registry.get("assets", []):
        asset_id = asset.get("id", "unknown")
        if asset.get("authoredPresent") is not True:
            complete = False
            add_reason(result, f"P1 asset is not approved authoredPresent=true: {asset_id}")
            continue
        uasset = object_path_to_uasset(asset.get("objectPath", ""))
        if not uasset.is_file():
            complete = False
            add_reason(result, f"P1 approved asset is missing native .uasset: {asset_id}")
    result["repositoryAuthoredComplete"] = complete
    return complete


def validate_inventory(path: Path, expected_commit: str, result: dict) -> bool:
    if not path.is_file():
        add_reason(result, "Native Unreal inventory is missing")
        return False
    try:
        payload = load_json(path)
    except Exception as exc:
        add_reason(result, f"Native inventory is invalid JSON: {exc}")
        return False

    p5 = load_json(P5)
    p1 = load_json(P1)
    p4 = load_json(P4)
    valid = True
    if payload.get("schemaVersion") != 1:
        valid = False; add_reason(result, "Native inventory schemaVersion must be 1")
    if payload.get("buildCommit") != expected_commit:
        valid = False; add_reason(result, "Native inventory commit does not match certification commit")
    if payload.get("unrealVersion") != p5.get("unrealVersion"):
        valid = False; add_reason(result, "Native inventory Unreal version mismatch")

    registry_expected = {a["id"] for a in p1.get("assets", [])}
    registry_rows = payload.get("registryAssets") or []
    registry_by_id = {row.get("id"): row for row in registry_rows if isinstance(row, dict)}
    if set(registry_by_id) != registry_expected:
        valid = False; add_reason(result, "Native inventory must cover every P1 registry asset exactly once")
    for asset_id in registry_expected:
        if registry_by_id.get(asset_id, {}).get("exists") is not True:
            valid = False; add_reason(result, f"Native P1 asset missing: {asset_id}")

    clip_expected = set(p4["animation"]["requiredClipIds"])
    clip_rows = payload.get("animationAssets") or []
    clip_by_id = {row.get("id"): row for row in clip_rows if isinstance(row, dict)}
    if set(clip_by_id) != clip_expected:
        valid = False; add_reason(result, "Native inventory must cover all 17 P4 animation clips")
    for clip_id in clip_expected:
        if clip_by_id.get(clip_id, {}).get("exists") is not True:
            valid = False; add_reason(result, f"Native AnimSequence missing: {clip_id}")

    effect_expected = set(p4["vfx"]["requiredEffectIds"])
    effect_rows = payload.get("vfxAssets") or []
    effect_by_id = {row.get("id"): row for row in effect_rows if isinstance(row, dict)}
    if set(effect_by_id) != effect_expected:
        valid = False; add_reason(result, "Native inventory must cover all 17 P4 VFX targets")
    for effect_id in effect_expected:
        if effect_by_id.get(effect_id, {}).get("exists") is not True:
            valid = False; add_reason(result, f"Native Niagara system missing: {effect_id}")

    presentation_rows = payload.get("presentationAssets") or []
    presentation_by_id = {row.get("id"): row for row in presentation_rows if isinstance(row, dict)}
    if set(presentation_by_id) != PRESENTATION_NATIVE_TARGETS:
        valid = False; add_reason(result, "Native inventory presentation/rig target set is incomplete")
    for target_id in PRESENTATION_NATIVE_TARGETS:
        if presentation_by_id.get(target_id, {}).get("exists") is not True:
            valid = False; add_reason(result, f"Native presentation/rig asset missing: {target_id}")

    result["nativeInventoryValid"] = valid
    return valid


def validate_review(path: Path, evidence_root: Path, expected_commit: str, result: dict) -> bool:
    if not path.is_file():
        add_reason(result, "P5 human polish review JSON is missing")
        return False
    try:
        review = load_json(path)
    except Exception as exc:
        add_reason(result, f"P5 review is invalid JSON: {exc}")
        return False

    p5 = load_json(P5)
    targets = p5["polishTargets"]
    valid = True
    if review.get("schemaVersion") != 1 or review.get("status") != "passed":
        valid = False; add_reason(result, "P5 review must be schemaVersion=1 and status=passed")
    if review.get("buildCommit") != expected_commit:
        valid = False; add_reason(result, "P5 review commit does not match certification commit")
    if review.get("unrealVersion") != p5["unrealVersion"]:
        valid = False; add_reason(result, "P5 review Unreal version mismatch")
    roles = set(review.get("reviewerRoles") or [])
    if not {"tech-art", "animation", "visual-qa"}.issubset(roles):
        valid = False; add_reason(result, "P5 review requires tech-art, animation and visual-qa roles")

    ml = review.get("materialsLightingReview") or {}
    for key in ("masterSurfaceApproved", "waterMaterialApproved", "exposureApproved", "shadowQualityApproved"):
        if ml.get(key) is not True:
            valid = False; add_reason(result, f"materialsLightingReview.{key} must be true")

    env = review.get("environmentReview") or {}
    for key in ("lodTransitionsApproved", "hlodAndCullingApproved", "foliageOverdrawApproved", "textureStreamingApproved", "noCriticalMipPopIn"):
        if env.get(key) is not True:
            valid = False; add_reason(result, f"environmentReview.{key} must be true")
    pool = env.get("texturePoolOverBudgetMB")
    if not isinstance(pool, (int, float)) or float(pool) > float(targets["environment"]["maxTexturePoolOverBudgetMB"]):
        valid = False; add_reason(result, "Texture pool must not be over budget")

    anim = review.get("characterAnimationReview") or {}
    for key in ("deformationApproved", "ikApproved", "physicsAssetApproved", "cosmeticClippingApproved"):
        if anim.get(key) is not True:
            valid = False; add_reason(result, f"characterAnimationReview.{key} must be true")
    foot_slide = anim.get("maxObservedFootSlideCm")
    if not isinstance(foot_slide, (int, float)) or float(foot_slide) < 0 or float(foot_slide) > float(targets["characterAnimation"]["maxObservedFootSlideCm"]):
        valid = False; add_reason(result, "Observed foot sliding exceeds P5 threshold")
    for key, allowed in (
        ("criticalDeformationCount", targets["characterAnimation"]["criticalDeformationCountAllowed"]),
        ("criticalCosmeticClippingCount", targets["characterAnimation"]["criticalCosmeticClippingCountAllowed"]),
    ):
        value = anim.get(key)
        if not isinstance(value, int) or value > int(allowed):
            valid = False; add_reason(result, f"characterAnimationReview.{key} exceeds allowed count")

    vfx = review.get("vfxReview") or {}
    for key in ("semanticParityApproved", "reducedMotionApproved", "overdrawApproved"):
        if vfx.get(key) is not True:
            valid = False; add_reason(result, f"vfxReview.{key} must be true")
    gpu = vfx.get("gpuP95MsByProfile") or {}
    for profile_id, ceiling in targets["vfx"]["maxGpuP95MsByProfile"].items():
        value = gpu.get(profile_id)
        if not isinstance(value, (int, float)) or float(value) <= 0 or float(value) > float(ceiling):
            valid = False; add_reason(result, f"VFX GPU p95 invalid/over budget for {profile_id}")

    presentation = review.get("presentationReview") or {}
    for key in ("cameraComfortApproved", "uiLegibilityApproved", "safeAreaApproved", "reducedMotionApproved", "noInputLock", "noForcedViewTarget", "noGlobalTimeScaleChange"):
        if presentation.get(key) is not True:
            valid = False; add_reason(result, f"presentationReview.{key} must be true")

    artifacts = review.get("artifacts") or []
    by_kind = {item.get("kind"): item for item in artifacts if isinstance(item, dict)}
    if set(by_kind) != REQUIRED_ARTIFACT_KINDS:
        valid = False; add_reason(result, "P5 review must include exactly four required artifact kinds")
    for kind in REQUIRED_ARTIFACT_KINDS:
        item = by_kind.get(kind) or {}
        actual = safe_payload(evidence_root, item.get("file", ""))
        if actual is None or not actual.is_file():
            valid = False; add_reason(result, f"Missing/unsafe review artifact: {kind}")
            continue
        if sha256_file(actual) != item.get("sha256"):
            valid = False; add_reason(result, f"Review artifact SHA-256 mismatch: {kind}")

    result["humanPolishReviewValid"] = valid
    return valid


def validate_v8(evidence_root: Path, expected_commit: str, result: dict) -> bool:
    v8 = load_v8_module()
    v8_root = evidence_root / "v8"
    assessment = v8.assess(v8_root, expected_commit)
    result["v8Status"] = assessment.get("status")
    valid = assessment.get("status") == "CERTIFIED"
    if not valid:
        add_reason(result, "V8 device evidence is not CERTIFIED")
        for reason in assessment.get("reasons", []):
            add_reason(result, "V8: " + reason)
        return False

    p5 = load_json(P5)
    required_pairs = {
        (platform, profile)
        for platform in p5["deviceCertification"]["requiredPlatforms"]
        for profile in p5["deviceCertification"]["requiredProfiles"]
    }
    present_pairs: set[tuple[str, str]] = set()
    for path in sorted(v8_root.glob("*.json")):
        try:
            payload = load_json(path)
        except Exception:
            continue
        if payload.get("status") == "passed" and payload.get("buildCommit") == expected_commit:
            present_pairs.add((payload.get("platform"), payload.get("profileId")))
    missing_pairs = required_pairs - present_pairs
    if missing_pairs:
        valid = False
        add_reason(result, "P5 requires all six platform/profile V8 packages; missing: " + ", ".join(f"{p}/{t}" for p, t in sorted(missing_pairs)))
    result["v8PlatformProfilePairsComplete"] = not missing_pairs
    return valid


def assess(evidence_dir: Path, expected_commit: str, enforce_repository: bool = True) -> dict:
    result = {
        "schemaVersion": 1,
        "status": "BLOCKED",
        "certified": False,
        "expectedCommit": expected_commit,
        "repositoryAuthoredComplete": False,
        "nativeInventoryValid": False,
        "humanPolishReviewValid": False,
        "v8Status": "BLOCKED",
        "v8PlatformProfilePairsComplete": False,
        "reasons": [],
    }
    if len(expected_commit) != 40 or any(c not in "0123456789abcdef" for c in expected_commit):
        add_reason(result, "Expected commit must be a full lowercase SHA")
        return result
    if not evidence_dir.is_dir():
        add_reason(result, "P5 evidence directory does not exist")
        return result

    repo_ok = validate_repository_authored_state(result) if enforce_repository else True
    if not enforce_repository:
        result["repositoryAuthoredComplete"] = True
    inventory_ok = validate_inventory(evidence_dir / INVENTORY_FILE, expected_commit, result)
    review_ok = validate_review(evidence_dir / REVIEW_FILE, evidence_dir, expected_commit, result)
    v8_ok = validate_v8(evidence_dir, expected_commit, result)

    certified = repo_ok and inventory_ok and review_ok and v8_ok and not result["reasons"]
    result["certified"] = certified
    result["status"] = "CERTIFIED" if certified else "BLOCKED"
    return result


def write_synthetic_inventory(root: Path, commit: str) -> None:
    p1 = load_json(P1)
    p4 = load_json(P4)
    payload = {
        "schemaVersion": 1,
        "buildCommit": commit,
        "unrealVersion": load_json(P5)["unrealVersion"],
        "registryAssets": [{"id": a["id"], "objectPath": a["objectPath"], "exists": True} for a in p1["assets"]],
        "animationAssets": [{"id": clip_id, "objectPath": "/Game/Synthetic/" + clip_id.replace(".", "_"), "exists": True} for clip_id in p4["animation"]["requiredClipIds"]],
        "vfxAssets": [{"id": effect_id, "objectPath": "/Game/Synthetic/" + effect_id.replace(".", "_"), "exists": True} for effect_id in p4["vfx"]["requiredEffectIds"]],
        "presentationAssets": [{"id": target_id, "objectPath": "/Game/Synthetic/" + target_id.replace(".", "_"), "exists": True} for target_id in sorted(PRESENTATION_NATIVE_TARGETS)],
    }
    (root / INVENTORY_FILE).write_text(json.dumps(payload, indent=2), encoding="utf-8")


def write_synthetic_review(root: Path, commit: str) -> None:
    targets = load_json(P5)["polishTargets"]
    artifacts = []
    for kind in sorted(REQUIRED_ARTIFACT_KINDS):
        rel = f"review/{kind}.bin"
        path = root / rel
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(("synthetic-p5-" + kind).encode("utf-8"))
        artifacts.append({"kind": kind, "file": rel, "sha256": sha256_file(path)})
    payload = {
        "schemaVersion": 1,
        "status": "passed",
        "buildCommit": commit,
        "unrealVersion": load_json(P5)["unrealVersion"],
        "reviewerRoles": ["tech-art", "animation", "visual-qa"],
        "materialsLightingReview": {"masterSurfaceApproved": True, "waterMaterialApproved": True, "exposureApproved": True, "shadowQualityApproved": True},
        "environmentReview": {"lodTransitionsApproved": True, "hlodAndCullingApproved": True, "foliageOverdrawApproved": True, "textureStreamingApproved": True, "noCriticalMipPopIn": True, "texturePoolOverBudgetMB": 0},
        "characterAnimationReview": {"deformationApproved": True, "ikApproved": True, "physicsAssetApproved": True, "cosmeticClippingApproved": True, "maxObservedFootSlideCm": 1.0, "criticalDeformationCount": 0, "criticalCosmeticClippingCount": 0},
        "vfxReview": {"semanticParityApproved": True, "reducedMotionApproved": True, "overdrawApproved": True, "gpuP95MsByProfile": {profile: ceiling * 0.5 for profile, ceiling in targets["vfx"]["maxGpuP95MsByProfile"].items()}},
        "presentationReview": {"cameraComfortApproved": True, "uiLegibilityApproved": True, "safeAreaApproved": True, "reducedMotionApproved": True, "noInputLock": True, "noForcedViewTarget": True, "noGlobalTimeScaleChange": True},
        "artifacts": artifacts,
    }
    (root / REVIEW_FILE).write_text(json.dumps(payload, indent=2), encoding="utf-8")


def self_test() -> int:
    commit = "1" * 40
    v8 = load_v8_module()
    with tempfile.TemporaryDirectory(prefix="wm-p5-") as temp:
        root = Path(temp)
        v8_root = root / "v8"
        v8_root.mkdir(parents=True)
        write_synthetic_inventory(root, commit)
        write_synthetic_review(root, commit)
        for platform in ("Android", "iPadOS"):
            for profile in ("performance.tablet.low", "performance.tablet.medium", "performance.tablet.high"):
                v8.write_synthetic_evidence(v8_root, platform, profile)
        if assess(root, commit, enforce_repository=False)["status"] != "CERTIFIED":
            raise SystemExit("Complete synthetic P5 evidence should certify in self-test")

        review_path = root / REVIEW_FILE
        review = load_json(review_path)
        review["characterAnimationReview"]["maxObservedFootSlideCm"] = 99.0
        review_path.write_text(json.dumps(review), encoding="utf-8")
        if assess(root, commit, enforce_repository=False)["status"] != "BLOCKED":
            raise SystemExit("Foot sliding above threshold must block P5 certification")

        write_synthetic_review(root, commit)
        (v8_root / "ipados-high.json").unlink()
        if assess(root, commit, enforce_repository=False)["status"] != "BLOCKED":
            raise SystemExit("Missing a platform/profile pair must block P5 certification")

    print("P5 assessor self-test passed: authored inventory, human review, six-package V8 cross-product, hashes and polish thresholds are fail-closed.")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--evidence-dir", type=Path)
    parser.add_argument("--expected-commit")
    parser.add_argument("--output", type=Path)
    parser.add_argument("--require-certified", action="store_true")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        return self_test()
    if not args.evidence_dir or not args.expected_commit:
        parser.error("--evidence-dir and --expected-commit are required")
    result = assess(args.evidence_dir, args.expected_commit, enforce_repository=True)
    rendered = json.dumps(result, indent=2, sort_keys=True)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(rendered + "\n", encoding="utf-8")
    print(rendered)
    if args.require_certified and result["status"] != "CERTIFIED":
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
