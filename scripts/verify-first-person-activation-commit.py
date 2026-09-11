#!/usr/bin/env python3
"""Verify that a first-person activation commit changes only approved manifests.

The reviewed source commit is the commit that received native evidence. The activation commit
may only flip the authored pack and activation manifest from blocked/all-off to approved/all-on.
No runtime, source art, importer, workflow or gameplay files may change between those commits.
"""
from __future__ import annotations

import argparse
import json
import re
import subprocess
from pathlib import Path

SHA1_RE = re.compile(r"^[0-9a-f]{40}$")
ALLOWED_PATHS = {
    "content/visual/first-person/first-person-authored-pack-v1.json",
    "game/Content/WorldMakers/Visual/first-person-authored-pack-v1.json",
    "content/visual/first-person/first-person-native-activation-v1.json",
    "game/Content/WorldMakers/Visual/first-person-native-activation-v1.json",
}


def fail(message: str) -> None:
    raise SystemExit("First-person activation commit verification failed: " + message)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--reviewed-source", required=True)
    parser.add_argument("--activation-head", default="HEAD")
    parser.add_argument("--repo-root", default=".")
    args = parser.parse_args()

    reviewed = args.reviewed_source.strip().lower()
    if not SHA1_RE.fullmatch(reviewed):
        fail("--reviewed-source must be a lowercase 40-character Git SHA")

    root = Path(args.repo_root).resolve()
    result = subprocess.run(
        ["git", "diff", "--name-only", reviewed, args.activation_head],
        cwd=root,
        capture_output=True,
        text=True,
        check=True,
    )
    changed = {line.strip().replace("\\", "/") for line in result.stdout.splitlines() if line.strip()}
    if not changed:
        fail("activation commit has no changes")
    unexpected = sorted(changed - ALLOWED_PATHS)
    if unexpected:
        fail(f"activation commit changed non-manifest paths: {unexpected}")
    if changed != ALLOWED_PATHS:
        fail(f"activation commit must update exactly four manifests; observed {sorted(changed)}")

    canonical_pack = root / "content/visual/first-person/first-person-authored-pack-v1.json"
    staged_pack = root / "game/Content/WorldMakers/Visual/first-person-authored-pack-v1.json"
    canonical_activation = root / "content/visual/first-person/first-person-native-activation-v1.json"
    staged_activation = root / "game/Content/WorldMakers/Visual/first-person-native-activation-v1.json"

    if canonical_pack.read_bytes() != staged_pack.read_bytes():
        fail("authored pack canonical/staged parity failed")
    if canonical_activation.read_bytes() != staged_activation.read_bytes():
        fail("activation manifest canonical/staged parity failed")

    pack = json.loads(canonical_pack.read_text(encoding="utf-8"))
    activation = json.loads(canonical_activation.read_text(encoding="utf-8"))
    assets = pack.get("assets", [])
    animations = pack.get("animations", [])
    if len(assets) != 5:
        fail(f"activation pack must contain exactly five assets, found {len(assets)}")
    if len(animations) != 9:
        fail(f"activation pack must contain exactly nine animations, found {len(animations)}")
    if any(item.get("authoredPresent") is not True for item in assets):
        fail("all five assets must be authoredPresent=true")
    if any(item.get("authoredPresent") is not True for item in animations):
        fail("all nine animations must be authoredPresent=true")
    if activation.get("status") != "activated" or activation.get("activated") is not True:
        fail("activation manifest is not activated")
    if activation.get("targetCommitSha") != reviewed:
        fail("activation targetCommitSha does not match reviewed source commit")
    for key in ("allFiveAssetsApproved", "allNineAnimationsApproved", "humanReviewApproved", "deviceReviewApproved"):
        if activation.get(key) is not True:
            fail(f"activation approval missing: {key}")

    print(f"First-person activation commit verified: exactly four manifest files changed after reviewed source {reviewed}.")


if __name__ == "__main__":
    main()
