#!/usr/bin/env python3
"""Repository mission validation without third-party dependencies.

Implements the JSON-Schema subset currently used by World Makers mission definitions:
type, required, additionalProperties, properties, enum, const, pattern, minItems,
minimum and maximum.
"""

from __future__ import annotations

import json
import re
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[1]


def fail(message: str) -> None:
    raise SystemExit(message)


def validate_node(value: Any, schema: dict[str, Any], path: str) -> None:
    expected_type = schema.get("type")
    if expected_type:
        valid = {
            "object": isinstance(value, dict),
            "array": isinstance(value, list),
            "string": isinstance(value, str),
            "integer": isinstance(value, int) and not isinstance(value, bool),
            "number": isinstance(value, (int, float)) and not isinstance(value, bool),
            "boolean": isinstance(value, bool),
        }.get(expected_type, True)
        if not valid:
            fail(f"{path}: expected {expected_type}, got {type(value).__name__}")

    if "enum" in schema and value not in schema["enum"]:
        fail(f"{path}: value {value!r} not in enum {schema['enum']}")
    if "const" in schema and value != schema["const"]:
        fail(f"{path}: expected constant {schema['const']!r}")
    if isinstance(value, str) and "pattern" in schema and not re.match(schema["pattern"], value):
        fail(f"{path}: value {value!r} does not match {schema['pattern']}")
    if isinstance(value, (int, float)) and not isinstance(value, bool):
        if "minimum" in schema and value < schema["minimum"]:
            fail(f"{path}: value {value} is below minimum {schema['minimum']}")
        if "maximum" in schema and value > schema["maximum"]:
            fail(f"{path}: value {value} is above maximum {schema['maximum']}")

    if isinstance(value, list):
        if len(value) < schema.get("minItems", 0):
            fail(f"{path}: requires at least {schema['minItems']} item(s)")
        item_schema = schema.get("items")
        if item_schema:
            for index, item in enumerate(value):
                validate_node(item, item_schema, f"{path}[{index}]")

    if isinstance(value, dict):
        properties = schema.get("properties", {})
        required = set(schema.get("required", []))
        missing = required.difference(value)
        if missing:
            fail(f"{path}: missing required fields: {sorted(missing)}")
        if schema.get("additionalProperties") is False:
            extras = set(value).difference(properties)
            if extras:
                fail(f"{path}: unexpected fields: {sorted(extras)}")
        for key, child_schema in properties.items():
            if key in value:
                validate_node(value[key], child_schema, f"{path}.{key}")


def main() -> None:
    schema_path = ROOT / "content/missions/schema/mission.schema.json"
    schema = json.loads(schema_path.read_text(encoding="utf-8"))
    mission_files = sorted((ROOT / "content/missions").glob("**/*.json"))
    mission_files = [path for path in mission_files if path != schema_path]
    if not mission_files:
        fail("No mission definitions found")

    seen_ids: set[str] = set()
    for path in mission_files:
        payload = json.loads(path.read_text(encoding="utf-8"))
        validate_node(payload, schema, str(path.relative_to(ROOT)))
        mission_id = payload["id"]
        if mission_id in seen_ids:
            fail(f"Duplicate mission id: {mission_id}")
        seen_ids.add(mission_id)

        review = payload["review"]
        runtime = payload["runtime"]
        if (review["pedagogy"] != "approved" or review["safety"] != "approved") and not runtime["prototypeOnly"]:
            fail(f"{mission_id}: non-approved mission must remain prototypeOnly=true")
        if payload.get("analyticsClassification") == "minimized-learning-evidence" and not payload.get("evidenceEvents"):
            fail(f"{mission_id}: minimized learning evidence requires stable evidence event IDs")

    print(f"Validated {len(mission_files)} mission definition(s) against the mission schema subset.")


if __name__ == "__main__":
    main()
