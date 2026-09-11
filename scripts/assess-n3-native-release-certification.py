#!/usr/bin/env python3
"""Fail-closed N3 native release certification assessor.

N3 is the final composition gate for the visual/native release chain. It does
not create device evidence. It verifies that N2, P5 and V8 all refer to the same
commit and that exactly six platform/profile package evidence records prove a
signed, installable, launchable and crash-free authored build on representative
Android and iPadOS device classes.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import tempfile
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CONTRACT = ROOT / "content/visual/certification/n3-native-release-certification.json"


def load(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def write_json(path: Path, payload: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def full_sha(value: object) -> bool:
    return isinstance(value, str) and len(value) == 40 and all(c in "0123456789abcdef" for c in value)


def safe_payload(root: Path, rel: object) -> Path | None:
    if not isinstance(rel, str) or not rel or Path(rel).is_absolute() or ".." in Path(rel).parts:
        return None
    resolved_root = root.resolve()
    candidate = (root / rel).resolve()
    if candidate != resolved_root and resolved_root not in candidate.parents:
        return None
    return candidate


def add_blocker(result: dict, message: str) -> None:
    result["blockers"].append(message)


def validate_dependency_assessments(root: Path, expected_commit: str, result: dict) -> bool:
    files = {
        "n2": root / "n2-release-assessment.json",
        "p5": root / "p5-assessment.json",
        "v8": root / "v8-assessment.json",
    }
    valid = True
    for label, path in files.items():
        if not path.is_file():
            valid = False
            add_blocker(result, f"missing {label.upper()} assessment")
            continue
        result["fingerprints"][f"{label}AssessmentSha256"] = sha256_file(path)

    if not valid:
        return False

    n2, p5, v8 = (load(files["n2"]), load(files["p5"]), load(files["v8"]))

    n2_ok = (
        n2.get("status") == "RELEASE_CANDIDATE"
        and n2.get("releaseCandidate") is True
        and n2.get("releaseCommit") == expected_commit
    )
    result["checks"]["n2ReleaseCandidate"] = n2_ok
    if not n2_ok:
        valid = False
        add_blocker(result, "N2 must be RELEASE_CANDIDATE for the exact release commit")

    p5_ok = (
        p5.get("status") == "CERTIFIED"
        and p5.get("certified") is True
        and p5.get("expectedCommit") == expected_commit
    )
    result["checks"]["p5Certified"] = p5_ok
    if not p5_ok:
        valid = False
        add_blocker(result, "P5 must be CERTIFIED for the exact release commit")

    v8_ok = (
        v8.get("status") == "CERTIFIED"
        and v8.get("certified") is True
        and v8.get("buildCommitsPresent") == [expected_commit]
    )
    result["checks"]["v8Certified"] = v8_ok
    if not v8_ok:
        valid = False
        add_blocker(result, "V8 must be CERTIFIED for the exact release commit")

    contract = load(CONTRACT)
    expected_pairs = set(contract["requiredPlatformProfilePairs"])
    v8_pairs = set(v8.get("platformProfilePairsPresent") or [])
    v8_pairs_ok = v8_pairs == expected_pairs
    result["checks"]["v8ExactSixPairs"] = v8_pairs_ok
    if not v8_pairs_ok:
        valid = False
        add_blocker(result, "V8 assessment must contain exactly all six Android/iPadOS profile pairs")

    return valid


def validate_release_review(root: Path, expected_commit: str, result: dict) -> bool:
    path = root / "release-review.json"
    if not path.is_file():
        add_blocker(result, "missing N3 release-review.json")
        return False
    result["fingerprints"]["releaseReviewSha256"] = sha256_file(path)
    try:
        review = load(path)
    except Exception as exc:
        add_blocker(result, f"release review is invalid JSON: {exc}")
        return False

    roles = set(review.get("reviewerRoles") or [])
    checks = review.get("checks") or {}
    required_checks = {
        "n2ReleaseCandidateApproved",
        "p5ArtPolishApproved",
        "v8DeviceMatrixApproved",
        "childSafetyRegressionReviewed",
        "noCriticalDefects",
        "releaseNotesApproved",
    }
    valid = (
        review.get("schemaVersion") == 1
        and review.get("status") == "approved"
        and review.get("buildCommit") == expected_commit
        and {"release-qa", "tech-art", "product-owner"}.issubset(roles)
        and set(checks) == required_checks
        and all(checks.get(key) is True for key in required_checks)
    )
    result["checks"]["humanReleaseReview"] = valid
    if not valid:
        add_blocker(result, "N3 human release review is incomplete or belongs to another commit")
    return valid


def validate_package_evidence(path: Path, root: Path, expected_commit: str, contract: dict) -> tuple[bool, list[str], str | None]:
    reasons: list[str] = []
    try:
        evidence = load(path)
    except Exception as exc:
        return False, [f"{path.name}: invalid JSON: {exc}"], None

    platform = evidence.get("platform")
    profile = evidence.get("profileId")
    pair = f"{platform}/{profile}" if platform and profile else None
    required_pairs = set(contract["requiredPlatformProfilePairs"])

    if evidence.get("schemaVersion") != 1:
        reasons.append(f"{path.name}: schemaVersion must be 1")
    if evidence.get("status") != "passed":
        reasons.append(f"{path.name}: status must be passed")
    if pair not in required_pairs:
        reasons.append(f"{path.name}: unsupported platform/profile pair")
    if evidence.get("unrealVersion") != contract["unrealVersion"]:
        reasons.append(f"{path.name}: Unreal version mismatch")
    if evidence.get("buildCommit") != expected_commit:
        reasons.append(f"{path.name}: buildCommit does not match release commit")

    kind = evidence.get("packageKind")
    allowed = set((contract.get("packageRules", {}).get(platform) or {}).get("allowedKinds", []))
    if kind not in allowed:
        reasons.append(f"{path.name}: package kind {kind!r} is invalid for {platform}")

    for key in ("signatureVerified", "installVerified", "launchVerified", "automationPassed", "runtimeTakeoverVerified"):
        if evidence.get(key) is not True:
            reasons.append(f"{path.name}: {key} must be true")

    if evidence.get("crashCount") != 0:
        reasons.append(f"{path.name}: crashCount must be exactly zero")
    if not isinstance(evidence.get("smokeSessionSeconds"), int) or evidence["smokeSessionSeconds"] < int(contract["smokeTest"]["minimumSessionSeconds"]):
        reasons.append(f"{path.name}: smoke session is shorter than the N3 minimum")

    smoke = evidence.get("smokeChecks") or {}
    required_smoke = set(contract["smokeTest"]["requiredChecks"])
    if set(smoke) != required_smoke or not all(smoke.get(key) is True for key in required_smoke):
        reasons.append(f"{path.name}: every required smoke check must be present and true")

    payload_fields = (
        ("packageFile", "packageSha256"),
        ("installLogFile", "installLogSha256"),
        ("runtimeLogFile", "runtimeLogSha256"),
        ("screenshotFile", "screenshotSha256"),
        ("v8EvidenceFile", "v8EvidenceSha256"),
    )
    resolved: dict[str, Path] = {}
    for file_key, hash_key in payload_fields:
        payload = safe_payload(root, evidence.get(file_key))
        if payload is None or not payload.is_file():
            reasons.append(f"{path.name}: missing or unsafe {file_key}")
            continue
        resolved[file_key] = payload
        if sha256_file(payload) != evidence.get(hash_key):
            reasons.append(f"{path.name}: {file_key} SHA-256 mismatch")

    package = resolved.get("packageFile")
    if package is not None:
        expected_suffix = "." + str(kind)
        if package.suffix.lower() != expected_suffix:
            reasons.append(f"{path.name}: package extension does not match packageKind")
        if evidence.get("packageSizeBytes") != package.stat().st_size:
            reasons.append(f"{path.name}: packageSizeBytes does not match package payload")

    v8_path = resolved.get("v8EvidenceFile")
    if v8_path is not None:
        try:
            v8 = load(v8_path)
        except Exception as exc:
            reasons.append(f"{path.name}: linked V8 evidence is invalid JSON: {exc}")
        else:
            if not (
                v8.get("status") == "passed"
                and v8.get("platform") == platform
                and v8.get("profileId") == profile
                and v8.get("buildCommit") == expected_commit
            ):
                reasons.append(f"{path.name}: linked V8 evidence does not match package pair/commit")

    return not reasons, reasons, pair


def assess(evidence_root: Path, expected_commit: str) -> dict:
    contract = load(CONTRACT)
    result = {
        "schemaVersion": 1,
        "phase": "N3",
        "status": "BLOCKED",
        "releaseCertified": False,
        "releaseCommit": expected_commit,
        "requiredPlatformProfilePairs": contract["requiredPlatformProfilePairs"],
        "platformProfilePairsPresent": [],
        "checks": {},
        "fingerprints": {},
        "packageEvidence": [],
        "blockers": [],
    }
    if not full_sha(expected_commit):
        add_blocker(result, "expected commit must be a full lowercase SHA")
        return result
    if not evidence_root.is_dir():
        add_blocker(result, "N3 evidence root does not exist")
        return result

    dependencies_ok = validate_dependency_assessments(evidence_root, expected_commit, result)
    review_ok = validate_release_review(evidence_root, expected_commit, result)

    package_dir = evidence_root / "packages"
    if not package_dir.is_dir():
        add_blocker(result, "N3 packages evidence directory does not exist")
        return result

    files = sorted(package_dir.glob("*.json"))
    pairs: list[str] = []
    packages_ok = True
    for path in files:
        valid, reasons, pair = validate_package_evidence(path, evidence_root, expected_commit, contract)
        result["packageEvidence"].append({"file": path.name, "valid": valid, "pair": pair})
        result["fingerprints"][f"packageEvidence:{path.name}"] = sha256_file(path)
        if pair:
            pairs.append(pair)
        if not valid:
            packages_ok = False
            result["blockers"].extend(reasons)

    required_pairs = set(contract["requiredPlatformProfilePairs"])
    pair_counts = Counter(pairs)
    missing = required_pairs - set(pair_counts)
    duplicate = {pair for pair, count in pair_counts.items() if count != 1}
    unexpected = set(pair_counts) - required_pairs
    exact_pairs = not missing and not duplicate and not unexpected and len(files) == len(required_pairs)
    if missing:
        add_blocker(result, "missing N3 package pairs: " + ", ".join(sorted(missing)))
    if duplicate:
        add_blocker(result, "duplicate N3 package pairs: " + ", ".join(sorted(duplicate)))
    if unexpected:
        add_blocker(result, "unexpected N3 package pairs: " + ", ".join(sorted(unexpected)))
    result["platformProfilePairsPresent"] = sorted(set(pairs))
    result["checks"]["exactSixPlatformProfilePairs"] = exact_pairs
    result["checks"]["packageEvidenceValid"] = packages_ok and exact_pairs

    certified = dependencies_ok and review_ok and packages_ok and exact_pairs and not result["blockers"]
    if certified:
        result["status"] = "RELEASE_CERTIFIED"
        result["releaseCertified"] = True
        result["deviceCertificationComplete"] = True
        result["sourceCiSelfCertified"] = False
    return result


def synthetic_dependency_files(root: Path, commit: str, pairs: list[str]) -> None:
    write_json(root / "n2-release-assessment.json", {
        "schemaVersion": 1, "status": "RELEASE_CANDIDATE", "releaseCandidate": True, "releaseCommit": commit,
    })
    write_json(root / "p5-assessment.json", {
        "schemaVersion": 1, "status": "CERTIFIED", "certified": True, "expectedCommit": commit,
    })
    write_json(root / "v8-assessment.json", {
        "schemaVersion": 1, "status": "CERTIFIED", "certified": True,
        "buildCommitsPresent": [commit], "platformProfilePairsPresent": pairs,
    })
    write_json(root / "release-review.json", {
        "schemaVersion": 1,
        "status": "approved",
        "buildCommit": commit,
        "reviewerRoles": ["release-qa", "tech-art", "product-owner"],
        "checks": {
            "n2ReleaseCandidateApproved": True,
            "p5ArtPolishApproved": True,
            "v8DeviceMatrixApproved": True,
            "childSafetyRegressionReviewed": True,
            "noCriticalDefects": True,
            "releaseNotesApproved": True,
        },
    })


def write_synthetic_package(root: Path, platform: str, profile: str, commit: str) -> Path:
    tier = profile.rsplit(".", 1)[-1]
    stem = f"{platform.lower()}-{tier}"
    package_kind = "ipa" if platform == "iPadOS" else "apk"
    package = root / "payloads" / f"{stem}.{package_kind}"
    install_log = root / "logs" / f"{stem}-install.log"
    runtime_log = root / "logs" / f"{stem}-runtime.log"
    screenshot = root / "screenshots" / f"{stem}.png"
    v8 = root / "v8" / f"{stem}.json"
    for payload, data in (
        (package, b"synthetic-package-" + stem.encode()),
        (install_log, b"synthetic-install-log-" + stem.encode()),
        (runtime_log, b"synthetic-runtime-log-" + stem.encode()),
        (screenshot, b"synthetic-screenshot-" + stem.encode()),
    ):
        payload.parent.mkdir(parents=True, exist_ok=True)
        payload.write_bytes(data)
    write_json(v8, {
        "schemaVersion": 1, "status": "passed", "platform": platform,
        "profileId": profile, "buildCommit": commit,
    })

    rel = lambda p: p.relative_to(root).as_posix()
    smoke = {key: True for key in load(CONTRACT)["smokeTest"]["requiredChecks"]}
    evidence = {
        "schemaVersion": 1,
        "status": "passed",
        "platform": platform,
        "profileId": profile,
        "unrealVersion": "5.8.2",
        "buildCommit": commit,
        "packageKind": package_kind,
        "packageFile": rel(package),
        "packageSha256": sha256_file(package),
        "packageSizeBytes": package.stat().st_size,
        "signatureVerified": True,
        "installVerified": True,
        "launchVerified": True,
        "automationPassed": True,
        "runtimeTakeoverVerified": True,
        "crashCount": 0,
        "smokeSessionSeconds": 120,
        "smokeChecks": smoke,
        "installLogFile": rel(install_log),
        "installLogSha256": sha256_file(install_log),
        "runtimeLogFile": rel(runtime_log),
        "runtimeLogSha256": sha256_file(runtime_log),
        "screenshotFile": rel(screenshot),
        "screenshotSha256": sha256_file(screenshot),
        "v8EvidenceFile": rel(v8),
        "v8EvidenceSha256": sha256_file(v8),
    }
    path = root / "packages" / f"{stem}.json"
    write_json(path, evidence)
    return path


def self_test() -> int:
    commit = "1" * 40
    contract = load(CONTRACT)
    required_pairs = list(contract["requiredPlatformProfilePairs"])
    with tempfile.TemporaryDirectory(prefix="wm-n3-") as temp:
        root = Path(temp)
        synthetic_dependency_files(root, commit, required_pairs)
        evidence_paths: list[Path] = []
        for pair in required_pairs:
            platform, profile = pair.split("/", 1)
            evidence_paths.append(write_synthetic_package(root, platform, profile, commit))

        if assess(root, commit)["status"] != "RELEASE_CERTIFIED":
            raise SystemExit("complete six-pair native release evidence must certify")

        missing_path = evidence_paths[-1]
        cached = missing_path.read_text(encoding="utf-8")
        missing_path.unlink()
        if assess(root, commit)["status"] != "BLOCKED":
            raise SystemExit("five of six package pairs must fail closed")
        missing_path.write_text(cached, encoding="utf-8")

        broken = load(evidence_paths[0])
        broken["crashCount"] = 1
        write_json(evidence_paths[0], broken)
        if assess(root, commit)["status"] != "BLOCKED":
            raise SystemExit("any crash must block N3 release certification")

        broken["crashCount"] = 0
        write_json(evidence_paths[0], broken)
        package = root / broken["packageFile"]
        package.write_bytes(package.read_bytes() + b"tamper")
        if assess(root, commit)["status"] != "BLOCKED":
            raise SystemExit("package hash mismatch must block N3 release certification")

    print("N3 assessor self-test passed: exact six pairs, dependency chain, zero-crash and package-integrity rules verified.")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--evidence-root", type=Path)
    parser.add_argument("--expected-commit")
    parser.add_argument("--output", type=Path)
    parser.add_argument("--require-release-certified", action="store_true")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()

    if args.self_test:
        return self_test()
    if not args.evidence_root or not args.expected_commit:
        parser.error("--evidence-root and --expected-commit are required unless --self-test is used")

    result = assess(args.evidence_root, args.expected_commit)
    rendered = json.dumps(result, indent=2, sort_keys=True)
    print(rendered)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(rendered + "\n", encoding="utf-8")
    if args.require_release_certified and result["status"] != "RELEASE_CERTIFIED":
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
