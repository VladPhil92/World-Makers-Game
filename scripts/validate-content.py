#!/usr/bin/env python3
"""Repository mission validation without third-party dependencies.

Implements the JSON-Schema subset currently used by World Makers mission definitions:
type, required, additionalProperties, properties, enum, const, pattern, minItems,
minimum and maximum. It also enforces evaluator-specific semantic contracts that
are awkward to express in the intentionally small local schema validator.
"""

from __future__ import annotations

import json
import re
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[1]

SUPPORTED_EVIDENCE_PRIMITIVES = {
    "construct-to-constraint",
    "observe-and-classify",
    "predict-test-revise",
    "sequence-and-infer",
    "communicate-in-language",
    "model-system",
    "solve-spatial-system",
    "interpret-text-world",
    "reason-through-dilemma",
    "argue-and-revise",
}


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


def validate_runtime_semantics(payload: dict[str, Any], source: str) -> None:
    mission_id = payload["id"]
    runtime = payload["runtime"]
    evaluator = runtime["evaluator"]
    objective_ids = payload["learningObjectives"]
    evidence_event_ids = payload["evidenceEvents"]

    if len(set(objective_ids)) != len(objective_ids):
        fail(f"{source}: {mission_id} has duplicate learning objective IDs")
    if len(set(evidence_event_ids)) != len(evidence_event_ids):
        fail(f"{source}: {mission_id} has duplicate evidence event IDs")

    if evaluator == "measure-and-build":
        required = {"targetSpanCm", "toleranceCm"}
        forbidden = {"observationRequirements", "evidencePrimitives"}
        missing = required.difference(runtime)
        present_forbidden = forbidden.intersection(runtime)
        if missing or present_forbidden:
            fail(
                f"{source}: {mission_id} measure-and-build fields invalid; "
                f"missing={sorted(missing)} forbidden={sorted(present_forbidden)}"
            )
        if runtime["toleranceCm"] >= runtime["targetSpanCm"]:
            fail(f"{source}: {mission_id} tolerance must be smaller than target span")
        return

    if evaluator == "observe-ecosystem":
        forbidden = {"targetSpanCm", "toleranceCm", "evidencePrimitives"}
        present_forbidden = forbidden.intersection(runtime)
        requirements = runtime.get("observationRequirements", [])
        if present_forbidden or len(requirements) < 2:
            fail(
                f"{source}: {mission_id} observe-ecosystem fields invalid; "
                f"forbidden={sorted(present_forbidden)} requirements={len(requirements)}"
            )

        observation_ids: set[str] = set()
        mapped_events: set[str] = set()
        for requirement in requirements:
            observation_id = requirement["observationId"]
            event_id = requirement["evidenceEventId"]
            objective_id = requirement["objectiveId"]
            if observation_id in observation_ids or event_id in mapped_events:
                fail(f"{source}: {mission_id} observation requirements must have unique observation/event IDs")
            if event_id not in evidence_event_ids or objective_id not in objective_ids:
                fail(f"{source}: {mission_id} observation requirement references undeclared evidence/objective ID")
            observation_ids.add(observation_id)
            mapped_events.add(event_id)
        if mapped_events != set(evidence_event_ids):
            fail(f"{source}: {mission_id} observation requirements must map every declared evidence event exactly once")
        return

    if evaluator == "composable":
        forbidden = {"targetSpanCm", "toleranceCm", "observationRequirements"}
        present_forbidden = forbidden.intersection(runtime)
        requirements = runtime.get("evidencePrimitives", [])
        if present_forbidden or not requirements:
            fail(
                f"{source}: {mission_id} composable fields invalid; "
                f"forbidden={sorted(present_forbidden)} requirements={len(requirements)}"
            )

        mapped_events: set[str] = set()
        for requirement in requirements:
            primitive_id = requirement["primitiveId"]
            event_id = requirement["evidenceEventId"]
            objective_id = requirement["objectiveId"]
            required_count = requirement.get("requiredCount", 1)
            if primitive_id not in SUPPORTED_EVIDENCE_PRIMITIVES:
                fail(f"{source}: {mission_id} unsupported evidence primitive {primitive_id!r}")
            if event_id in mapped_events:
                fail(f"{source}: {mission_id} composable evidence event IDs must be unique")
            if event_id not in evidence_event_ids or objective_id not in objective_ids:
                fail(f"{source}: {mission_id} composable requirement references undeclared evidence/objective ID")
            if not isinstance(required_count, int) or isinstance(required_count, bool) or not 1 <= required_count <= 20:
                fail(f"{source}: {mission_id} requiredCount must be an integer from 1 to 20")
            mapped_events.add(event_id)
        if mapped_events != set(evidence_event_ids):
            fail(f"{source}: {mission_id} composable requirements must map every declared evidence event exactly once")
        return

    fail(f"{source}: {mission_id} unsupported evaluator {evaluator!r}")


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
        source = str(path.relative_to(ROOT))
        validate_node(payload, schema, source)
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

        validate_runtime_semantics(payload, source)

    print(f"Validated {len(mission_files)} mission definition(s) against schema and evaluator semantics.")


if __name__ == "__main__":
    main()
