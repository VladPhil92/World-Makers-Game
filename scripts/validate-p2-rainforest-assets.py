#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import json
import re
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "content/visual/authored/rainforest-p2-source-pack.json"
GENERATOR = ROOT / "scripts/generate-p2-rainforest-source.py"
P1 = ROOT / "content/visual/authored/authored-assets-p1.json"
WORKFLOW = ROOT / ".github/workflows/repo-quality.yml"
DOC = ROOT / "docs/p2-authored-rainforest-asset-pack.md"

EXPECTED_IDS = {
    "environment.rainforest.ground.a",
    "environment.rainforest.terrain.a",
    "environment.rainforest.tree.a",
    "environment.rainforest.tree.b",
    "environment.rainforest.tree.c",
    "environment.rainforest.understory.a",
    "environment.rainforest.rock.a",
    "environment.rainforest.water-edge.a",
    "environment.rainforest.hero-ceiba",
}


def fail(message: str) -> None:
    raise SystemExit("P2 validation failed: " + message)


def main() -> None:
    for path in (MANIFEST, GENERATOR, P1, WORKFLOW, DOC):
        if not path.exists():
            fail(f"missing required file {path.relative_to(ROOT)}")

    data = json.loads(MANIFEST.read_text(encoding="utf-8"))
    if data.get("schemaVersion") != 1 or data.get("units") != "centimeters":
        fail("invalid schemaVersion/units")
    if data.get("upAxis") != "Z" or data.get("forwardAxis") != "X":
        fail("P1 axis contract changed")
    assets = data.get("assets", [])
    ids = {a.get("id") for a in assets}
    if ids != EXPECTED_IDS or len(assets) != 9:
        fail("rainforest pack must contain exactly the nine P2 environment families")

    p1 = json.loads(P1.read_text(encoding="utf-8"))
    p1_assets = {a["id"]: a for a in p1["assets"]}
    for asset in assets:
        aid = asset["id"]
        if aid not in p1_assets or p1_assets[aid]["kind"] != "StaticMesh":
            fail(f"{aid} is not a P1 StaticMesh target")
        if asset["lodCount"] < p1_assets[aid]["minLods"]:
            fail(f"{aid} does not meet P1 LOD floor")
        if asset["materialSlots"] > p1_assets[aid]["maxMaterialSlots"]:
            fail(f"{aid} exceeds P1 material-slot ceiling")
        digest = asset.get("sha256", "")
        if not re.fullmatch(r"[0-9a-f]{64}", digest):
            fail(f"{aid} has invalid sha256")

    boundary = data.get("productionBoundary", {})
    if boundary != {
        "sourceGeometryPresent": True,
        "nativeUassetsPresent": False,
        "nativeImportRequired": True,
        "deviceReviewRequired": True,
    }:
        fail("production boundary must remain honest and fail-closed")

    with tempfile.TemporaryDirectory(prefix="wm-p2-") as tmp:
        out = Path(tmp) / "rainforest"
        subprocess.run([sys.executable, str(GENERATOR), "--output-dir", str(out), "--verify"], check=True, cwd=ROOT)
        for asset in assets:
            path = out / asset["filename"]
            if not path.exists():
                fail(f"generator did not produce {asset['filename']}")
            raw = path.read_bytes()
            if hashlib.sha256(raw).hexdigest() != asset["sha256"]:
                fail(f"generated hash mismatch for {asset['filename']}")
            text = raw.decode("utf-8")
            if "\nvt " not in text or "\nvn " not in text or "\nf " not in text:
                fail(f"{asset['filename']} lacks UVs, normals or faces")
            lods = set(re.findall(r"^o .*_LOD(\d+)", text, flags=re.MULTILINE))
            if len(lods) != asset["lodCount"]:
                fail(f"{asset['filename']} expected {asset['lodCount']} LOD groups, found {sorted(lods)}")
            materials = set(re.findall(r"^usemtl (\S+)", text, flags=re.MULTILINE))
            if len(materials) > asset["materialSlots"]:
                fail(f"{asset['filename']} uses too many materials")

    workflow = WORKFLOW.read_text(encoding="utf-8")
    if "Validate P2 Authored Rainforest Asset Pack" not in workflow:
        fail("Repository Quality does not run the P2 gate")
    doc = DOC.read_text(encoding="utf-8")
    for phrase in ("nine environment families", "native .uasset", "procedural fallback", "P3"):
        if phrase.lower() not in doc.lower():
            fail(f"documentation missing required contract phrase: {phrase}")

    print("P2 rainforest source pack validated: 9 families, deterministic geometry, UV0/normals, LOD/material budgets, fail-closed native boundary.")


if __name__ == "__main__":
    main()
