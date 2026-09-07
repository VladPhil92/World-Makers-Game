#!/usr/bin/env python3
"""Lightweight repository content validation with no third-party dependencies."""

from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main() -> None:
    schema_path = ROOT / "content/missions/schema/mission.schema.json"
    schema = json.loads(schema_path.read_text(encoding="utf-8"))
    required = set(schema["required"])

    mission_files = sorted((ROOT / "content/missions").glob("**/*.json"))
    mission_files = [p for p in mission_files if p != schema_path]
    if not mission_files:
        raise SystemExit("No mission examples found")

    for path in mission_files:
        payload = json.loads(path.read_text(encoding="utf-8"))
        missing = required.difference(payload)
        if missing:
            raise SystemExit(f"{path}: missing required fields: {sorted(missing)}")
        if not str(payload["id"]).startswith("mission."):
            raise SystemExit(f"{path}: invalid mission id")

    print(f"Validated {len(mission_files)} mission definition(s).")


if __name__ == "__main__":
    main()
