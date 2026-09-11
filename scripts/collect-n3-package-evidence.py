#!/usr/bin/env python3
"""Build one N3 package evidence record from real payloads.

The collector never infers success from filenames. Verification booleans must be
supplied explicitly by the native/device runner after the corresponding command
or human check succeeds. Missing verification produces status=failed.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CONTRACT = ROOT / "content/visual/certification/n3-native-release-certification.json"


def load(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def full_sha(value: str) -> bool:
    return len(value) == 40 and all(c in "0123456789abcdef" for c in value)


def main() -> int:
    contract = load(CONTRACT)
    parser = argparse.ArgumentParser()
    parser.add_argument("--platform", choices=contract["requiredPlatforms"], required=True)
    parser.add_argument("--profile", choices=contract["requiredProfiles"], required=True)
    parser.add_argument("--build-commit", required=True)
    parser.add_argument("--package-kind", choices=["apk", "aab", "ipa"], required=True)
    parser.add_argument("--package", type=Path, required=True)
    parser.add_argument("--install-log", type=Path, required=True)
    parser.add_argument("--runtime-log", type=Path, required=True)
    parser.add_argument("--screenshot", type=Path, required=True)
    parser.add_argument("--v8-evidence", type=Path, required=True)
    parser.add_argument("--evidence-root", type=Path, required=True)
    parser.add_argument("--smoke-session-seconds", type=int, required=True)
    parser.add_argument("--crash-count", type=int, required=True)
    parser.add_argument("--signature-verified", action="store_true")
    parser.add_argument("--install-verified", action="store_true")
    parser.add_argument("--launch-verified", action="store_true")
    parser.add_argument("--automation-passed", action="store_true")
    parser.add_argument("--runtime-takeover-verified", action="store_true")
    for check in contract["smokeTest"]["requiredChecks"]:
        parser.add_argument("--smoke-" + check, action="store_true")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    if not full_sha(args.build_commit):
        parser.error("--build-commit must be a full lowercase SHA")

    allowed = set(contract["packageRules"][args.platform]["allowedKinds"])
    if args.package_kind not in allowed:
        parser.error(f"{args.package_kind} is not valid for {args.platform}")

    files = {
        "package": args.package,
        "installLog": args.install_log,
        "runtimeLog": args.runtime_log,
        "screenshot": args.screenshot,
        "v8Evidence": args.v8_evidence,
    }
    for label, path in files.items():
        if not path.is_file():
            parser.error(f"{label} does not exist: {path}")

    root = args.evidence_root.resolve()
    root.mkdir(parents=True, exist_ok=True)

    def rel(path: Path) -> str:
        resolved = path.resolve()
        if resolved != root and root not in resolved.parents:
            parser.error(f"payload must live inside --evidence-root: {resolved}")
        return resolved.relative_to(root).as_posix()

    smoke = {
        check: bool(getattr(args, "smoke_" + check.replace("-", "_")))
        for check in contract["smokeTest"]["requiredChecks"]
    }
    verifications = {
        "signatureVerified": args.signature_verified,
        "installVerified": args.install_verified,
        "launchVerified": args.launch_verified,
        "automationPassed": args.automation_passed,
        "runtimeTakeoverVerified": args.runtime_takeover_verified,
    }
    passed = (
        all(verifications.values())
        and args.crash_count == 0
        and args.smoke_session_seconds >= int(contract["smokeTest"]["minimumSessionSeconds"])
        and all(smoke.values())
    )

    payload = {
        "schemaVersion": 1,
        "status": "passed" if passed else "failed",
        "platform": args.platform,
        "profileId": args.profile,
        "unrealVersion": contract["unrealVersion"],
        "buildCommit": args.build_commit,
        "packageKind": args.package_kind,
        "packageFile": rel(args.package),
        "packageSha256": sha256_file(args.package),
        "packageSizeBytes": args.package.stat().st_size,
        **verifications,
        "crashCount": args.crash_count,
        "smokeSessionSeconds": args.smoke_session_seconds,
        "smokeChecks": smoke,
        "installLogFile": rel(args.install_log),
        "installLogSha256": sha256_file(args.install_log),
        "runtimeLogFile": rel(args.runtime_log),
        "runtimeLogSha256": sha256_file(args.runtime_log),
        "screenshotFile": rel(args.screenshot),
        "screenshotSha256": sha256_file(args.screenshot),
        "v8EvidenceFile": rel(args.v8_evidence),
        "v8EvidenceSha256": sha256_file(args.v8_evidence),
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(payload, indent=2))
    return 0 if passed else 2


if __name__ == "__main__":
    raise SystemExit(main())
