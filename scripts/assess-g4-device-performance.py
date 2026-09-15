#!/usr/bin/env python3
"""Fail-closed G4 representative-device performance and stability assessor."""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import tempfile
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CONTRACT = ROOT / "content/production/g4-device-performance-stability-v1.json"
V8_ASSESSOR = ROOT / "scripts/assess-v8-visual-certification.py"
REQUIRED_PAIRS = {
    (platform, profile)
    for platform in ("Android", "iPadOS")
    for profile in (
        "performance.tablet.low",
        "performance.tablet.medium",
        "performance.tablet.high",
    )
}


def load_json(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8-sig"))


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def safe_payload(root: Path, rel: str) -> Path | None:
    if not isinstance(rel, str) or not rel or Path(rel).is_absolute() or ".." in Path(rel).parts:
        return None
    candidate = (root / rel).resolve()
    resolved_root = root.resolve()
    if candidate != resolved_root and resolved_root not in candidate.parents:
        return None
    return candidate


def load_v8_module():
    spec = importlib.util.spec_from_file_location("wm_v8_assessor", V8_ASSESSOR)
    if spec is None or spec.loader is None:
        raise RuntimeError("Unable to load V8 assessor")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def add_reason(result: dict, message: str) -> None:
    result["reasons"].append(message)


def validate_g3_result(path: Path, expected_commit: str, result: dict) -> bool:
    if not path.is_file():
        add_reason(result, "G3 result is missing")
        return False
    try:
        payload = load_json(path)
    except Exception as exc:
        add_reason(result, f"G3 result is invalid JSON: {exc}")
        return False
    valid = True
    if payload.get("status") != "CERTIFIED" or payload.get("certified") is not True:
        valid = False
        add_reason(result, "G3 must be CERTIFIED")
    if payload.get("repositoryCommit") != expected_commit:
        valid = False
        add_reason(result, "G3 result commit does not match G4 certification commit")
    result["g3CertifiedSameCommit"] = valid
    return valid


def validate_stability_file(path: Path, root: Path, expected_commit: str, contract: dict):
    reasons: list[str] = []
    try:
        evidence = load_json(path)
    except Exception as exc:
        return False, [f"{path.name}: invalid JSON: {exc}"], None, None, None

    platform = evidence.get("platform")
    profile = evidence.get("profileId")
    commit = evidence.get("buildCommit")
    expected_version = contract["engine"]["expectedVersion"]
    stability = contract["stabilityEvidence"]

    if evidence.get("schemaVersion") != 1:
        reasons.append(f"{path.name}: schemaVersion must be 1")
    if evidence.get("status") != "passed":
        reasons.append(f"{path.name}: status must be passed")
    if (platform, profile) not in REQUIRED_PAIRS:
        reasons.append(f"{path.name}: unsupported platform/profile pair")
    if evidence.get("unrealVersion") != expected_version:
        reasons.append(f"{path.name}: Unreal version mismatch")
    if commit != expected_commit:
        reasons.append(f"{path.name}: buildCommit does not match certification commit")

    device = evidence.get("device") or {}
    if device.get("isPhysicalDevice") is not True:
        reasons.append(f"{path.name}: certification requires a physical device")
    for key in ("manufacturer", "model", "osVersion", "soc", "gpu"):
        if not isinstance(device.get(key), str) or not device.get(key).strip():
            reasons.append(f"{path.name}: device.{key} is required")
    physical_memory = device.get("physicalMemoryMB")
    if not isinstance(physical_memory, (int, float)) or float(physical_memory) <= 0:
        reasons.append(f"{path.name}: device.physicalMemoryMB must be measured")

    telemetry = safe_payload(root, evidence.get("telemetryFile", ""))
    if telemetry is None or not telemetry.is_file():
        reasons.append(f"{path.name}: telemetry payload is missing or unsafe")
    elif sha256_file(telemetry) != evidence.get("telemetrySha256"):
        reasons.append(f"{path.name}: telemetry SHA-256 mismatch")

    soak = evidence.get("soak") or {}
    duration = soak.get("durationSeconds")
    if not isinstance(duration, (int, float)) or float(duration) < float(stability["minimumSoakDurationSeconds"]):
        reasons.append(f"{path.name}: soak duration is below minimum")

    initial_frame = soak.get("initialP95FrameTimeMs")
    final_frame = soak.get("finalP95FrameTimeMs")
    reported_drift = soak.get("frameTimeDriftPercent")
    if not all(isinstance(value, (int, float)) and float(value) > 0 for value in (initial_frame, final_frame)):
        reasons.append(f"{path.name}: initial/final p95 frame time must be measured and positive")
    elif not isinstance(reported_drift, (int, float)):
        reasons.append(f"{path.name}: frameTimeDriftPercent is required")
    else:
        computed = ((float(final_frame) - float(initial_frame)) / float(initial_frame)) * 100.0
        if abs(computed - float(reported_drift)) > 0.5:
            reasons.append(f"{path.name}: frameTimeDriftPercent does not match measured values")
        if float(reported_drift) > float(stability["maxFrameTimeDriftPercent"]):
            reasons.append(f"{path.name}: frame-time drift exceeds G4 budget")

    initial_memory = soak.get("initialProcessMemoryMB")
    final_memory = soak.get("finalProcessMemoryMB")
    peak_memory = soak.get("peakProcessMemoryMB")
    reported_growth = soak.get("residentMemoryGrowthMB")
    memory_values = (initial_memory, final_memory, peak_memory)
    if not all(isinstance(value, (int, float)) and float(value) > 0 for value in memory_values):
        reasons.append(f"{path.name}: process memory measurements must be positive")
    else:
        computed_growth = float(final_memory) - float(initial_memory)
        if not isinstance(reported_growth, (int, float)) or abs(computed_growth - float(reported_growth)) > 2.0:
            reasons.append(f"{path.name}: residentMemoryGrowthMB does not match measured values")
        growth_limit = stability["maxResidentMemoryGrowthMBByProfile"].get(profile)
        if growth_limit is None or float(reported_growth) > float(growth_limit):
            reasons.append(f"{path.name}: resident memory growth exceeds profile budget")
        if isinstance(physical_memory, (int, float)) and float(physical_memory) > 0:
            max_fraction = float(stability["maxProcessMemoryFractionOfPhysical"])
            if float(peak_memory) > float(physical_memory) * max_fraction:
                reasons.append(f"{path.name}: peak process memory exceeds physical-memory fraction budget")

    for key in ("thermalStateStart", "thermalStateEnd"):
        if not isinstance(soak.get(key), str) or not soak.get(key).strip():
            reasons.append(f"{path.name}: soak.{key} is required")
    if soak.get("thermalThrottlingObserved") is not False:
        reasons.append(f"{path.name}: thermal throttling is not allowed")
    if soak.get("criticalThermalStateObserved") is not False:
        reasons.append(f"{path.name}: critical thermal state is not allowed")
    drop = soak.get("sustainedPerformanceDropPercent")
    if not isinstance(drop, (int, float)) or float(drop) < 0 or float(drop) > float(stability["maxSustainedPerformanceDropPercent"]):
        reasons.append(f"{path.name}: sustained performance drop exceeds G4 budget")

    reliability = evidence.get("reliability") or {}
    count_limits = {
        "crashCount": stability["crashesAllowed"],
        "fatalErrorCount": stability["fatalErrorsAllowed"],
        "outOfMemoryEventCount": stability["outOfMemoryEventsAllowed"],
    }
    for key, allowed in count_limits.items():
        value = reliability.get(key)
        if not isinstance(value, int) or value < 0 or value > int(allowed):
            reasons.append(f"{path.name}: reliability.{key} exceeds allowed count")
    cycles = reliability.get("foregroundBackgroundCycles")
    if not isinstance(cycles, int) or cycles < int(stability["minimumForegroundBackgroundCycles"]):
        reasons.append(f"{path.name}: foreground/background cycle count is below minimum")
    if reliability.get("suspendResumePassed") is not True:
        reasons.append(f"{path.name}: suspend/resume must pass")
    if reliability.get("inputRecoveryPassed") is not True:
        reasons.append(f"{path.name}: input recovery must pass")

    return not reasons, reasons, platform, profile, commit


def assess(evidence_root: Path, g3_result: Path, expected_commit: str) -> dict:
    contract = load_json(CONTRACT)
    result = {
        "schema": "worldmakers.g4-device-performance-result.v1",
        "status": "BLOCKED",
        "certified": False,
        "repositoryCommit": expected_commit,
        "g3CertifiedSameCommit": False,
        "v8Status": "BLOCKED",
        "stabilityEvidenceIntegrity": False,
        "requiredPairs": [f"{p}/{t}" for p, t in sorted(REQUIRED_PAIRS)],
        "pairsPresent": [],
        "reasons": [],
    }
    if len(expected_commit) != 40 or any(c not in "0123456789abcdef" for c in expected_commit):
        add_reason(result, "Expected commit must be a full lowercase SHA")
        return result

    g3_ok = validate_g3_result(g3_result, expected_commit, result)

    v8_dir = evidence_root / "v8"
    v8 = load_v8_module()
    v8_result = v8.assess(v8_dir, expected_commit)
    result["v8Status"] = v8_result.get("status", "BLOCKED")
    if result["v8Status"] != "CERTIFIED":
        add_reason(result, "V8 representative-device performance evidence is not CERTIFIED")
        for reason in v8_result.get("reasons", []):
            add_reason(result, "V8: " + reason)

    stability_dir = evidence_root / "stability"
    if not stability_dir.is_dir():
        add_reason(result, "Stability evidence directory does not exist")
        return result
    files = sorted(stability_dir.glob("*.json"))
    pairs: list[tuple[str, str]] = []
    commits: set[str] = set()
    all_stability_valid = True
    for path in files:
        valid, reasons, platform, profile, commit = validate_stability_file(path, stability_dir, expected_commit, contract)
        if platform and profile:
            pairs.append((platform, profile))
        if isinstance(commit, str) and len(commit) == 40:
            commits.add(commit)
        if not valid:
            all_stability_valid = False
            result["reasons"].extend(reasons)

    counts = Counter(pairs)
    missing = REQUIRED_PAIRS - set(counts)
    duplicate = {pair for pair, count in counts.items() if count != 1}
    unexpected = set(counts) - REQUIRED_PAIRS
    if len(files) != len(REQUIRED_PAIRS):
        all_stability_valid = False
        add_reason(result, "G4 requires exactly six stability evidence packages")
    if missing:
        all_stability_valid = False
        add_reason(result, "Missing stability pairs: " + ", ".join(f"{p}/{t}" for p, t in sorted(missing)))
    if duplicate:
        all_stability_valid = False
        add_reason(result, "Duplicate stability pairs: " + ", ".join(f"{p}/{t}" for p, t in sorted(duplicate)))
    if unexpected:
        all_stability_valid = False
        add_reason(result, "Unexpected stability pairs: " + ", ".join(f"{p}/{t}" for p, t in sorted(unexpected)))
    if commits != {expected_commit}:
        all_stability_valid = False
        add_reason(result, "All stability packages must reference the certification commit")

    result["pairsPresent"] = [f"{p}/{t}" for p, t in sorted(set(pairs))]
    result["stabilityEvidenceIntegrity"] = all_stability_valid
    certified = g3_ok and result["v8Status"] == "CERTIFIED" and all_stability_valid
    result["certified"] = certified
    result["status"] = "CERTIFIED" if certified else "BLOCKED"
    return result


def write_synthetic_stability(root: Path, platform: str, profile: str, commit: str) -> None:
    tier = profile.rsplit(".", 1)[-1]
    stem = f"{platform.lower()}-{tier}"
    telemetry_rel = f"telemetry/{stem}.csv"
    telemetry = root / telemetry_rel
    telemetry.parent.mkdir(parents=True, exist_ok=True)
    telemetry.write_text("second,frame_ms,memory_mb,thermal\n0,16,500,nominal\n900,17,520,fair\n", encoding="utf-8")
    physical = 4096 if tier == "low" else 6144 if tier == "medium" else 8192
    payload = {
        "schemaVersion": 1,
        "status": "passed",
        "platform": platform,
        "profileId": profile,
        "buildCommit": commit,
        "unrealVersion": "5.8.2",
        "device": {
            "isPhysicalDevice": True,
            "manufacturer": "Synthetic",
            "model": f"{platform}-{tier}",
            "osVersion": "self-test",
            "soc": "self-test",
            "gpu": "self-test",
            "physicalMemoryMB": physical,
        },
        "telemetryFile": telemetry_rel,
        "telemetrySha256": sha256_file(telemetry),
        "soak": {
            "durationSeconds": 900,
            "initialP95FrameTimeMs": 16.0,
            "finalP95FrameTimeMs": 17.0,
            "frameTimeDriftPercent": 6.25,
            "initialProcessMemoryMB": 500,
            "finalProcessMemoryMB": 520,
            "peakProcessMemoryMB": 540,
            "residentMemoryGrowthMB": 20,
            "thermalStateStart": "nominal",
            "thermalStateEnd": "fair",
            "thermalThrottlingObserved": False,
            "criticalThermalStateObserved": False,
            "sustainedPerformanceDropPercent": 5.0,
        },
        "reliability": {
            "crashCount": 0,
            "fatalErrorCount": 0,
            "outOfMemoryEventCount": 0,
            "foregroundBackgroundCycles": 3,
            "suspendResumePassed": True,
            "inputRecoveryPassed": True,
        },
    }
    (root / f"{stem}.json").write_text(json.dumps(payload, indent=2), encoding="utf-8")


def self_test() -> int:
    commit = "1" * 40
    v8 = load_v8_module()
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        (root / "v8").mkdir()
        stability = root / "stability"
        stability.mkdir()
        for platform, profile in sorted(REQUIRED_PAIRS):
            v8.write_synthetic_evidence(root / "v8", platform, profile)
            write_synthetic_stability(stability, platform, profile, commit)
        for path in (root / "v8").glob("*.json"):
            payload = load_json(path)
            payload["buildCommit"] = commit
            path.write_text(json.dumps(payload, indent=2), encoding="utf-8")
        g3 = root / "g3.json"
        g3.write_text(json.dumps({"status": "CERTIFIED", "certified": True, "repositoryCommit": commit}), encoding="utf-8")
        if assess(root, g3, commit)["status"] != "CERTIFIED":
            raise SystemExit("Complete compliant G4 synthetic evidence should certify")
        bad = stability / "android-low.json"
        payload = load_json(bad)
        payload["soak"]["thermalThrottlingObserved"] = True
        bad.write_text(json.dumps(payload, indent=2), encoding="utf-8")
        if assess(root, g3, commit)["status"] != "BLOCKED":
            raise SystemExit("Thermal throttling must block G4")
    print("G4 assessor self-test passed: G3 dependency, V8 reuse, six device pairs, integrity, memory, thermal and reliability rules verified.")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--evidence-root", type=Path)
    parser.add_argument("--g3-result", type=Path)
    parser.add_argument("--expected-commit")
    parser.add_argument("--output", type=Path)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        return self_test()
    if not args.evidence_root or not args.g3_result or not args.expected_commit:
        parser.error("--evidence-root, --g3-result and --expected-commit are required")
    result = assess(args.evidence_root, args.g3_result, args.expected_commit)
    raw = json.dumps(result, indent=2) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(raw, encoding="utf-8")
    print(raw, end="")
    return 0 if result["status"] == "CERTIFIED" else 1


if __name__ == "__main__":
    raise SystemExit(main())
