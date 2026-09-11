#!/usr/bin/env python3
"""Assess whether the authored visual stack is ready for deliberate P1 activation.

N1 never mutates the repository registry. When every native contract passes, it
writes a candidate copy of the P1 manifest with only `authoredPresent` changed
to true. A human-reviewed commit is still required to apply that candidate.
"""
from __future__ import annotations

import argparse
import copy
import hashlib
import json
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CONTRACT = ROOT / "content/visual/authored/n1-native-integration.json"
P1 = ROOT / "content/visual/authored/authored-assets-p1.json"


def load(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def sha256_file(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def full_sha(value: str) -> bool:
    return isinstance(value, str) and len(value) == 40 and all(c in "0123456789abcdef" for c in value)


def assess(p2_path: Path, p3_path: Path, p4_path: Path, p5_path: Path, expected_commit: str) -> tuple[dict, dict | None]:
    contract = load(CONTRACT)
    p1 = load(P1)
    result = {
        "schemaVersion": 1,
        "phase": "N1",
        "status": "BLOCKED",
        "activationCandidate": False,
        "buildCommit": expected_commit,
        "automaticRegistryMutation": False,
        "inputSha256": {},
        "checks": {},
        "blockers": [],
    }

    if not full_sha(expected_commit):
        result["blockers"].append("expected commit must be a full lowercase SHA")
        return result, None

    paths = {"p2": p2_path, "p3": p3_path, "p4": p4_path, "p5": p5_path}
    for key, path in paths.items():
        if not path.is_file():
            result["blockers"].append(f"missing {key} native report")
    if result["blockers"]:
        return result, None

    for key, path in paths.items():
        result["inputSha256"][key] = sha256_file(path)

    p2, p3, p4, p5 = map(load, (p2_path, p3_path, p4_path, p5_path))
    required = contract["required"]

    p2_assets = p2.get("assets", [])
    p2_ok = (
        p2.get("schemaVersion") == 1
        and p2.get("readyForAuthoredPresentReview") is True
        and len(p2_assets) == required["p2RainforestAssets"]
        and all(row.get("passedNativeImportContract") is True for row in p2_assets)
    )
    result["checks"]["p2RainforestImports"] = p2_ok
    if not p2_ok:
        result["blockers"].append("P2 rainforest native import contract is incomplete")

    p3_assets = p3.get("assets", [])
    p3_ok = (
        p3.get("schemaVersion") == 1
        and p3.get("readyForHumanDeformationReview") is True
        and p3.get("authoredPresentMutated") is False
        and p3.get("skeletonCreated") is True
        and len(p3_assets) == required["p3CharacterModules"]
        and all(row.get("passedNativeImportContract") is True for row in p3_assets)
    )
    result["checks"]["p3CharacterImports"] = p3_ok
    if not p3_ok:
        result["blockers"].append("P3 character native import contract is incomplete")

    p4_animation = p4.get("animation", [])
    p4_sequences = p4.get("levelSequences", [])
    p4_ok = (
        p4.get("schemaVersion") == 1
        and p4.get("authoredPresentMutated") is False
        and p4.get("readyForAnimationReview") is True
        and p4.get("readyForPresentationReview") is True
        and len(p4_animation) == required["p4AnimationAssets"]
        and all(row.get("passedNativeImportContract") is True for row in p4_animation)
        and len(p4_sequences) == required["p4LevelSequences"]
        and all(row.get("containerCreated") is True for row in p4_sequences)
    )
    result["checks"]["p4AnimationPresentationImports"] = p4_ok
    if not p4_ok:
        result["blockers"].append("P4 animation/presentation native handoff is incomplete")

    registry = p5.get("registryAssets", [])
    animations = p5.get("animationAssets", [])
    vfx = p5.get("vfxAssets", [])
    presentation = p5.get("presentationAssets", [])
    p5_shape_ok = (
        p5.get("schemaVersion") == 1
        and p5.get("buildCommit") == expected_commit
        and p5.get("unrealVersion") == contract["unrealVersion"]
        and len(registry) == required["p1RegistryAssets"]
        and len(animations) == required["p4AnimationAssets"]
        and len(vfx) == required["p5VfxAssets"]
        and len(presentation) == required["p5PresentationAssets"]
    )
    p5_exists_ok = p5_shape_ok and all(
        row.get("exists") is True
        for collection in (registry, animations, vfx, presentation)
        for row in collection
    )
    result["checks"]["p5NativeInventory"] = p5_exists_ok
    if not p5_shape_ok:
        result["blockers"].append("P5 native inventory shape/build commit is invalid")
    elif not p5_exists_ok:
        missing = [row.get("id", "unknown") for collection in (registry, animations, vfx, presentation) for row in collection if row.get("exists") is not True]
        result["blockers"].append("P5 native inventory is missing assets: " + ", ".join(missing))

    p1_assets = p1.get("assets", [])
    p1_registry_ok = len(p1_assets) == required["p1RegistryAssets"] and len({a.get("id") for a in p1_assets}) == len(p1_assets)
    if p1_registry_ok:
        expected_registry = {(a["id"], a["objectPath"]) for a in p1_assets}
        inventory_registry = {(a.get("id"), a.get("objectPath")) for a in registry}
        p1_registry_ok = expected_registry == inventory_registry
    result["checks"]["p1RegistryMatchesInventory"] = p1_registry_ok
    if not p1_registry_ok:
        result["blockers"].append("P1 registry IDs/object paths do not match the native inventory")

    current_flags_safe = all(a.get("authoredPresent") is False for a in p1_assets)
    result["checks"]["repositoryRegistryStillFailClosed"] = current_flags_safe
    if not current_flags_safe:
        result["blockers"].append("N1 source branch must not pre-activate P1 authoredPresent flags")

    if result["blockers"]:
        return result, None

    candidate = copy.deepcopy(p1)
    for asset in candidate["assets"]:
        asset["authoredPresent"] = True

    for before, after in zip(p1["assets"], candidate["assets"]):
        before_copy = dict(before)
        after_copy = dict(after)
        before_copy.pop("authoredPresent", None)
        after_copy.pop("authoredPresent", None)
        if before_copy != after_copy or after.get("authoredPresent") is not True:
            result["blockers"].append(f"candidate changed fields other than authoredPresent for {before.get('id')}")
            return result, None

    result["status"] = "ACTIVATION_CANDIDATE"
    result["activationCandidate"] = True
    result["candidateAssetCount"] = len(candidate["assets"])
    result["humanReviewRequiredBeforeApplyingCandidate"] = True
    return result, candidate


def write_json(path: Path, payload: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")


def synthetic_reports(root: Path, commit: str) -> tuple[Path, Path, Path, Path]:
    p1 = load(P1)
    p2 = {
        "schemaVersion": 1,
        "readyForAuthoredPresentReview": True,
        "assets": [{"id": f"env-{i}", "passedNativeImportContract": True} for i in range(9)],
    }
    p3 = {
        "schemaVersion": 1,
        "readyForHumanDeformationReview": True,
        "authoredPresentMutated": False,
        "skeletonCreated": True,
        "assets": [{"slot": f"slot-{i}", "passedNativeImportContract": True} for i in range(8)],
    }
    p4 = {
        "schemaVersion": 1,
        "authoredPresentMutated": False,
        "readyForAnimationReview": True,
        "readyForPresentationReview": True,
        "animation": [{"id": f"anim-{i}", "passedNativeImportContract": True} for i in range(17)],
        "levelSequences": [{"id": f"seq-{i}", "containerCreated": True} for i in range(2)],
    }
    registry = [{"id": a["id"], "objectPath": a["objectPath"], "exists": True} for a in p1["assets"]]
    p5 = {
        "schemaVersion": 1,
        "buildCommit": commit,
        "unrealVersion": "5.8.2",
        "registryAssets": registry,
        "animationAssets": [{"id": f"anim-{i}", "exists": True} for i in range(17)],
        "vfxAssets": [{"id": f"vfx-{i}", "exists": True} for i in range(17)],
        "presentationAssets": [{"id": f"presentation-{i}", "exists": True} for i in range(7)],
    }
    paths = []
    for name, payload in (("p2.json", p2), ("p3.json", p3), ("p4.json", p4), ("p5.json", p5)):
        path = root / name
        write_json(path, payload)
        paths.append(path)
    return tuple(paths)


def self_test() -> int:
    commit = "1" * 40
    with tempfile.TemporaryDirectory(prefix="wm-n1-") as temp:
        root = Path(temp)
        p2, p3, p4, p5 = synthetic_reports(root, commit)
        result, candidate = assess(p2, p3, p4, p5, commit)
        if result["status"] != "ACTIVATION_CANDIDATE" or candidate is None:
            raise SystemExit("complete synthetic native reports must create an activation candidate")
        if not all(a["authoredPresent"] is True for a in candidate["assets"]):
            raise SystemExit("activation candidate must enable every P1 row")

        broken = load(p5)
        broken["vfxAssets"][0]["exists"] = False
        write_json(p5, broken)
        result, candidate = assess(p2, p3, p4, p5, commit)
        if result["status"] != "BLOCKED" or candidate is not None:
            raise SystemExit("missing native VFX must block N1")

        p2, p3, p4, p5 = synthetic_reports(root, commit)
        wrong = load(p5)
        wrong["buildCommit"] = "2" * 40
        write_json(p5, wrong)
        result, _ = assess(p2, p3, p4, p5, commit)
        if result["status"] != "BLOCKED":
            raise SystemExit("cross-commit inventory must block N1")

    print("N1 assessor self-test passed: all-or-nothing activation, missing-asset and same-commit rules verified.")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--p2-report", type=Path)
    parser.add_argument("--p3-report", type=Path)
    parser.add_argument("--p4-report", type=Path)
    parser.add_argument("--p5-inventory", type=Path)
    parser.add_argument("--expected-commit")
    parser.add_argument("--output", type=Path)
    parser.add_argument("--candidate-manifest", type=Path)
    parser.add_argument("--require-candidate", action="store_true")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()

    if args.self_test:
        return self_test()
    required = (args.p2_report, args.p3_report, args.p4_report, args.p5_inventory, args.expected_commit, args.output, args.candidate_manifest)
    if any(value is None for value in required):
        parser.error("native report paths, expected commit, output and candidate manifest are required")

    result, candidate = assess(args.p2_report, args.p3_report, args.p4_report, args.p5_inventory, args.expected_commit)
    write_json(args.output, result)
    if candidate is not None:
        write_json(args.candidate_manifest, candidate)
    elif args.candidate_manifest.exists():
        args.candidate_manifest.unlink()
    print(json.dumps(result, indent=2))
    if args.require_candidate and result["status"] != "ACTIVATION_CANDIDATE":
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
