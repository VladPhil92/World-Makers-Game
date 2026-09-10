#!/usr/bin/env python3
"""Report the World Makers P1 authored-art production queue without mutating the repository."""

from __future__ import annotations

import argparse
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "content/visual/authored/authored-assets-p1.json"


def uasset_path(object_path: str) -> Path:
    package = object_path.split(".", 1)[0].removeprefix("/Game/")
    return ROOT / "game/Content" / f"{package}.uasset"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--json", action="store_true", help="Emit machine-readable JSON instead of Markdown.")
    args = parser.parse_args()

    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    rows = []
    for asset in manifest["assets"]:
        file_exists = uasset_path(asset["objectPath"]).is_file()
        rows.append({
            "id": asset["id"],
            "kind": asset["kind"],
            "declaredPresent": asset["authoredPresent"],
            "uassetExists": file_exists,
            "minLods": asset["minLods"],
            "maxMaterialSlots": asset["maxMaterialSlots"],
            "collisionPolicy": asset["collisionPolicy"],
            "fallbackMode": asset["fallbackMode"],
            "objectPath": asset["objectPath"],
            "sourcePath": asset["sourcePath"],
        })

    if args.json:
        print(json.dumps({"schemaVersion": 1, "assets": rows}, indent=2))
        return 0

    complete = sum(1 for row in rows if row["declaredPresent"] and row["uassetExists"])
    print(f"# P1 Authored Asset Queue\n\nProduction-ready declarations: **{complete}/{len(rows)}**\n")
    print("| Asset | Kind | Present | LOD floor | Material slots | Collision | Fallback |")
    print("|---|---|---:|---:|---:|---|---|")
    for row in rows:
        present = "yes" if row["declaredPresent"] and row["uassetExists"] else "no"
        print(f"| `{row['id']}` | {row['kind']} | {present} | {row['minLods']} | {row['maxMaterialSlots']} | {row['collisionPolicy']} | {row['fallbackMode']} |")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
