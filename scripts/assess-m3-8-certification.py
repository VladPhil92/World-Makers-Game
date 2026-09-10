#!/usr/bin/env python3
"""Fail-closed M3.8 Caribbean Rainforest vertical-slice certification assessment."""

from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
import sys
import tempfile
from pathlib import Path, PurePosixPath
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
EXPECTED_MAP = "game/Content/WorldMakers/Maps/WM_PrototypeCertification.umap"
REQUIRED_ROUTE_CHECKS = (
    "biomeLoaded",
    "semanticExploration",
    "deliberateObservations",
    "scienceMissionCompleted",
    "reactiveEcosystemVisible",
    "ecologicalBuildApplied",
    "creativeUnlockGranted",
    "myAdventuresProgressVisible",
    "stopWithoutPenalty",
    "resumeRestoresProgress",
    "noCommercialPressure",
)
REQUIRED_TABLET_PLATFORMS = {"iPadOS", "Android"}
MIN_FRAME_SAMPLES = 1800
FORBIDDEN_NORMALIZED_KEYS = {
    "childname",
    "childprofileid",
    "accountid",
    "userid",
    "email",
    "advertisingid",
    "deviceadvertisingid",
    "serialnumber",
    "physicallocation",
    "preciselocation",
    "freetext",
    "chat",
    "missionanswer",
    "learningresponse",
    "learningresponsetext",
    "purchase",
    "commerce",
    "paymenttoken",
}


def normalize_key(value: str) -> str:
    return "".join(ch.lower() for ch in value if ch.isalnum())


def read_json(path: Path) -> Any:
    with path.open("r", encoding="utf-8-sig") as handle:
        return json.load(handle)


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def git_head() -> str:
    result = subprocess.run(
        ["git", "-C", str(ROOT), "rev-parse", "HEAD"],
        check=False,
        capture_output=True,
        text=True,
    )
    return result.stdout.strip() if result.returncode == 0 else ""


def expected_unreal_version() -> str:
    path = ROOT / "game/UNREAL_ENGINE_VERSION"
    return path.read_text(encoding="utf-8").strip() if path.is_file() else "5.8.2"


def find_forbidden_key_paths(value: Any, prefix: str = "$") -> list[str]:
    violations: list[str] = []
    if isinstance(value, dict):
        for key, child in value.items():
            if normalize_key(str(key)) in FORBIDDEN_NORMALIZED_KEYS:
                violations.append(f"{prefix}.{key}")
            violations.extend(find_forbidden_key_paths(child, f"{prefix}.{key}"))
    elif isinstance(value, list):
        for index, child in enumerate(value):
            violations.extend(find_forbidden_key_paths(child, f"{prefix}[{index}]"))
    return violations


def safe_evidence_path(evidence_dir: Path, relative: str) -> Path | None:
    candidate = PurePosixPath(relative)
    if candidate.is_absolute() or ".." in candidate.parts or not candidate.parts:
        return None
    resolved_root = evidence_dir.resolve()
    resolved = (evidence_dir / Path(*candidate.parts)).resolve()
    try:
        resolved.relative_to(resolved_root)
    except ValueError:
        return None
    return resolved


def load_profile_catalog() -> dict[str, dict[str, Any]]:
    path = ROOT / "content/performance/tablet-performance-profiles.json"
    payload = read_json(path)
    return {entry["id"]: entry for entry in payload.get("profiles", [])}


def add_blocker(blockers: list[str], message: str) -> None:
    if message not in blockers:
        blockers.append(message)


def assess(evidence_dir: Path, expected_commit: str) -> dict[str, Any]:
    evidence_dir = evidence_dir.resolve()
    expected_version = expected_unreal_version()
    profiles = load_profile_catalog()
    blockers: list[str] = []
    checks = {
        "nativeRuntime": False,
        "manualSmoke": False,
        "verticalSliceRoute": False,
        "representativeTabletCoverage": False,
        "tabletPerformanceBudgets": False,
        "evidenceIntegrity": False,
        "privacyBoundary": False,
    }
    privacy_payloads: list[Any] = []
    integrity_ok = True

    manifest_path = evidence_dir / "certification-manifest.json"
    manifest: dict[str, Any] | None = None
    if not manifest_path.is_file():
        add_blocker(blockers, "missing certification-manifest.json")
        integrity_ok = False
    else:
        try:
            manifest = read_json(manifest_path)
            privacy_payloads.append(manifest)
        except (OSError, json.JSONDecodeError) as exc:
            add_blocker(blockers, f"invalid certification-manifest.json: {exc}")
            integrity_ok = False

    if manifest is not None:
        commit_ok = manifest.get("repositoryCommit") == expected_commit
        version_ok = (
            manifest.get("expectedUnrealVersion") == expected_version
            and manifest.get("actualUnrealVersion") == expected_version
        )
        authored = manifest.get("authoredMap", {})
        map_ok = (
            authored.get("path") == EXPECTED_MAP
            and authored.get("present") is True
            and isinstance(authored.get("sha256"), str)
            and len(authored.get("sha256")) == 64
        )
        native_ok = manifest.get("nativeAutomation", {}).get("status") == "passed"
        runtime_ok = manifest.get("runtimeCertification", {}).get("certified") is True
        checks["nativeRuntime"] = commit_ok and version_ok and map_ok and native_ok and runtime_ok
        if not commit_ok:
            add_blocker(blockers, "native evidence commit does not match assessed commit")
        if not version_ok:
            add_blocker(blockers, f"native evidence must use exact Unreal Engine {expected_version}")
        if not map_ok:
            add_blocker(blockers, "authored certification map evidence is missing or invalid")
        if not native_ok:
            add_blocker(blockers, "complete WorldMakers native automation evidence has not passed")
        if not runtime_ok:
            add_blocker(blockers, "base runtime certification is not certified")

        manual_ok = manifest.get("manualSmoke", {}).get("status") == "passed"
        checks["manualSmoke"] = manual_ok
        if not manual_ok:
            add_blocker(blockers, "manual smoke evidence has not passed")

    route_path = evidence_dir / "vertical-slice-route.json"
    route: dict[str, Any] | None = None
    if not route_path.is_file():
        add_blocker(blockers, "missing vertical-slice-route.json")
        integrity_ok = False
    else:
        try:
            route = read_json(route_path)
            privacy_payloads.append(route)
        except (OSError, json.JSONDecodeError) as exc:
            add_blocker(blockers, f"invalid vertical-slice-route.json: {exc}")
            integrity_ok = False

    if route is not None:
        route_checks = route.get("checks", {})
        all_route_checks = all(route_checks.get(name) == "passed" for name in REQUIRED_ROUTE_CHECKS)
        route_ok = (
            route.get("schemaVersion") == 1
            and route.get("status") == "passed"
            and route.get("testedAtUtc")
            and route.get("operatorRole") in {"qa", "developer", "pedagogy-reviewer"}
            and route.get("unrealVersion") == expected_version
            and route.get("commit") == expected_commit
            and route.get("map") == EXPECTED_MAP
            and set(route_checks) == set(REQUIRED_ROUTE_CHECKS)
            and all_route_checks
        )
        checks["verticalSliceRoute"] = bool(route_ok)
        if not route_ok:
            add_blocker(blockers, "integrated M3 vertical-slice route is incomplete, failed, or mismatched")

    devices_dir = evidence_dir / "devices"
    device_files = sorted(devices_dir.glob("*.json")) if devices_dir.is_dir() else []
    if len(device_files) < 2:
        add_blocker(blockers, "at least two representative tablet evidence records are required")

    platforms: set[str] = set()
    valid_device_count = 0
    all_device_budgets_ok = bool(device_files)
    device_summaries: list[dict[str, Any]] = []

    for device_path in device_files:
        try:
            device = read_json(device_path)
            privacy_payloads.append(device)
        except (OSError, json.JSONDecodeError) as exc:
            add_blocker(blockers, f"invalid device evidence {device_path.name}: {exc}")
            integrity_ok = False
            all_device_budgets_ok = False
            continue

        platform = device.get("platform")
        if platform in REQUIRED_TABLET_PLATFORMS:
            platforms.add(platform)
        profile_id = device.get("performanceProfileId")
        profile = profiles.get(profile_id)
        basic_ok = (
            device.get("schemaVersion") == 1
            and device.get("status") == "passed"
            and bool(device.get("testedAtUtc"))
            and platform in REQUIRED_TABLET_PLATFORMS
            and isinstance(device.get("hardwareClass"), str)
            and len(device.get("hardwareClass", "")) >= 3
            and isinstance(device.get("deviceModel"), str)
            and len(device.get("deviceModel", "")) >= 3
            and isinstance(device.get("osVersion"), str)
            and bool(device.get("osVersion"))
            and device.get("unrealVersion") == expected_version
            and device.get("repositoryCommit") == expected_commit
            and device.get("routeStatus") == "passed"
            and profile is not None
            and profile.get("deviceClass") == "tablet"
        )
        if not basic_ok:
            add_blocker(blockers, f"device evidence {device_path.name} failed metadata/commit/profile validation")
            integrity_ok = False
            all_device_budgets_ok = False
            continue

        capture_rel = device.get("captureFile", "")
        capture_path = safe_evidence_path(evidence_dir, capture_rel) if isinstance(capture_rel, str) else None
        if capture_path is None or not capture_path.is_file():
            add_blocker(blockers, f"device evidence {device_path.name} references a missing/unsafe capture")
            integrity_ok = False
            all_device_budgets_ok = False
            continue

        actual_hash = sha256_file(capture_path)
        if actual_hash != device.get("captureSha256"):
            add_blocker(blockers, f"capture hash mismatch for {device_path.name}")
            integrity_ok = False
            all_device_budgets_ok = False
            continue

        try:
            capture = read_json(capture_path)
            privacy_payloads.append(capture)
        except (OSError, json.JSONDecodeError) as exc:
            add_blocker(blockers, f"invalid capture for {device_path.name}: {exc}")
            integrity_ok = False
            all_device_budgets_ok = False
            continue

        capture_ok = (
            capture.get("schemaVersion") == 1
            and capture.get("profileId") == profile_id
            and isinstance(capture.get("frameSampleCount"), int)
            and capture.get("frameSampleCount", 0) >= MIN_FRAME_SAMPLES
            and capture.get("withinBudget") is True
            and capture.get("frameTimeWithinBudget") is True
            and capture.get("actorCountWithinBudget") is True
            and capture.get("buildPieceCountWithinBudget") is True
            and capture.get("interactableCountWithinBudget") is True
            and float(capture.get("p95FrameTimeMs", 10**9)) <= float(profile["frameTimeBudgetMs"])
            and int(capture.get("maxWorldActors", 10**9)) <= int(profile["maxWorldActors"])
            and int(capture.get("maxPlacedBuildPieces", 10**9)) <= int(profile["maxPlacedBuildPieces"])
            and int(capture.get("maxActiveInteractables", 10**9)) <= int(profile["maxActiveInteractables"])
        )
        if not capture_ok:
            add_blocker(blockers, f"capture for {device_path.name} does not satisfy canonical profile budget or minimum sample count")
            all_device_budgets_ok = False
            continue

        valid_device_count += 1
        device_summaries.append(
            {
                "evidenceFile": device_path.name,
                "platform": platform,
                "hardwareClass": device["hardwareClass"],
                "deviceModel": device["deviceModel"],
                "performanceProfileId": profile_id,
                "frameSampleCount": capture["frameSampleCount"],
                "p95FrameTimeMs": capture["p95FrameTimeMs"],
                "withinBudget": True,
                "captureSha256": actual_hash,
            }
        )

    checks["representativeTabletCoverage"] = (
        len(device_files) >= 2
        and valid_device_count >= 2
        and REQUIRED_TABLET_PLATFORMS.issubset(platforms)
    )
    if not REQUIRED_TABLET_PLATFORMS.issubset(platforms):
        missing_platforms = sorted(REQUIRED_TABLET_PLATFORMS - platforms)
        add_blocker(blockers, f"missing representative tablet platform evidence: {', '.join(missing_platforms)}")

    checks["tabletPerformanceBudgets"] = (
        all_device_budgets_ok
        and valid_device_count == len(device_files)
        and valid_device_count >= 2
    )

    privacy_violations: list[str] = []
    for index, payload in enumerate(privacy_payloads):
        for path in find_forbidden_key_paths(payload):
            privacy_violations.append(f"payload[{index}]{path[1:]}")
    checks["privacyBoundary"] = not privacy_violations
    if privacy_violations:
        add_blocker(blockers, "forbidden personal/commercial evidence keys detected: " + ", ".join(privacy_violations[:8]))

    checks["evidenceIntegrity"] = integrity_ok
    if not integrity_ok:
        add_blocker(blockers, "evidence integrity validation failed")

    certified = all(checks.values()) and not blockers
    return {
        "schemaVersion": 1,
        "status": "CERTIFIED" if certified else "BLOCKED",
        "certified": certified,
        "assessedCommit": expected_commit,
        "expectedUnrealVersion": expected_version,
        "checks": checks,
        "requiredTabletPlatforms": sorted(REQUIRED_TABLET_PLATFORMS),
        "observedTabletPlatforms": sorted(platforms),
        "minimumFrameSamplesPerCapture": MIN_FRAME_SAMPLES,
        "validDeviceEvidenceCount": valid_device_count,
        "devices": device_summaries,
        "blockers": blockers,
    }


def write_json(path: Path, payload: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def make_capture(profile_id: str, p95: float, profile: dict[str, Any]) -> dict[str, Any]:
    return {
        "schemaVersion": 1,
        "profileId": profile_id,
        "frameSampleCount": MIN_FRAME_SAMPLES,
        "averageFrameTimeMs": p95 * 0.8,
        "p95FrameTimeMs": p95,
        "worstFrameTimeMs": p95 * 1.3,
        "maxWorldActors": max(1, int(profile["maxWorldActors"]) // 2),
        "maxPlacedBuildPieces": max(1, int(profile["maxPlacedBuildPieces"]) // 2),
        "maxActiveInteractables": max(1, int(profile["maxActiveInteractables"]) // 2),
        "frameTimeWithinBudget": True,
        "actorCountWithinBudget": True,
        "buildPieceCountWithinBudget": True,
        "interactableCountWithinBudget": True,
        "withinBudget": True,
    }


def run_self_test() -> None:
    expected_commit = "a" * 40
    expected_version = expected_unreal_version()
    profiles = load_profile_catalog()
    with tempfile.TemporaryDirectory() as temp:
        evidence = Path(temp)
        blocked = assess(evidence, expected_commit)
        assert blocked["certified"] is False and blocked["status"] == "BLOCKED"

        manifest = {
            "schemaVersion": 1,
            "repositoryCommit": expected_commit,
            "expectedUnrealVersion": expected_version,
            "actualUnrealVersion": expected_version,
            "authoredMap": {"path": EXPECTED_MAP, "present": True, "sha256": "b" * 64},
            "nativeAutomation": {"status": "passed", "testFilter": "WorldMakers."},
            "manualSmoke": {"status": "passed"},
            "runtimeCertification": {"certified": True, "status": "certified"},
        }
        write_json(evidence / "certification-manifest.json", manifest)
        route = {
            "schemaVersion": 1,
            "status": "passed",
            "testedAtUtc": "2026-09-10T10:00:00Z",
            "operatorRole": "qa",
            "platform": "Windows",
            "unrealVersion": expected_version,
            "commit": expected_commit,
            "map": EXPECTED_MAP,
            "checks": {name: "passed" for name in REQUIRED_ROUTE_CHECKS},
        }
        write_json(evidence / "vertical-slice-route.json", route)
        (evidence / "performance").mkdir()
        (evidence / "devices").mkdir()

        device_inputs = (
            ("iPadOS", "ipados", "performance.tablet.medium", "Representative iPad"),
            ("Android", "android", "performance.tablet.low", "Representative Android tablet"),
        )
        for platform, slug, profile_id, model in device_inputs:
            profile = profiles[profile_id]
            capture_rel = f"performance/capture-{slug}.json"
            capture_path = evidence / capture_rel
            write_json(capture_path, make_capture(profile_id, float(profile["frameTimeBudgetMs"]) * 0.8, profile))
            device = {
                "schemaVersion": 1,
                "evidenceId": f"device.{slug}.reference-01",
                "status": "passed",
                "testedAtUtc": "2026-09-10T10:00:00Z",
                "platform": platform,
                "hardwareClass": "reference-tablet",
                "deviceModel": model,
                "osVersion": "reference-os",
                "unrealVersion": expected_version,
                "repositoryCommit": expected_commit,
                "performanceProfileId": profile_id,
                "captureFile": capture_rel,
                "captureSha256": sha256_file(capture_path),
                "routeStatus": "passed",
            }
            write_json(evidence / "devices" / f"{slug}.json", device)

        passed = assess(evidence, expected_commit)
        assert passed["certified"] is True and passed["status"] == "CERTIFIED", passed

        tampered = read_json(evidence / "devices/ipados.json")
        tampered["captureSha256"] = "0" * 64
        write_json(evidence / "devices/ipados.json", tampered)
        failed_integrity = assess(evidence, expected_commit)
        assert failed_integrity["certified"] is False
        assert failed_integrity["checks"]["evidenceIntegrity"] is False

    print("M3.8 assessor self-test passed: blocked, certified and tamper-detection paths behave fail-closed.")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--evidence-dir", type=Path, default=ROOT / "artifacts/certification")
    parser.add_argument("--expected-commit", default=None)
    parser.add_argument("--output", type=Path, default=None)
    parser.add_argument("--require-certified", action="store_true")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()

    if args.self_test:
        run_self_test()
        return 0

    expected_commit = args.expected_commit or git_head()
    if not expected_commit:
        print("Unable to resolve assessed commit.", file=sys.stderr)
        return 2

    assessment = assess(args.evidence_dir, expected_commit)
    output = args.output or (args.evidence_dir / "m3-vertical-slice-assessment.json")
    write_json(output, assessment)
    print(json.dumps(assessment, indent=2, sort_keys=True))

    if args.require_certified and not assessment["certified"]:
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
