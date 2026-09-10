#!/usr/bin/env python3
"""M5.2 Composable Mission Runtime v2 source gate."""

from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

PRIMITIVES = (
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
)


def fail(message: str) -> None:
    raise SystemExit(message)


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def main() -> None:
    required_files = (
        "docs/m5-2-composable-mission-runtime.md",
        "content/missions/schema/mission.schema.json",
        "content/missions/examples/composable-eclipse-engine-proof.json",
        "game/Source/WorldMakers/Mission/WMMissionTypes.h",
        "game/Source/WorldMakers/Mission/WMMissionTypes.cpp",
        "game/Source/WorldMakers/Mission/WMMissionRuntimeSubsystem.h",
        "game/Source/WorldMakers/Mission/WMMissionRuntimeSubsystem.cpp",
        "game/Source/WorldMakers/Private/Tests/WMMissionRuntimeTests.cpp",
    )
    missing = [path for path in required_files if not (ROOT / path).exists()]
    if missing:
        fail(f"Missing M5.2 files: {missing}")

    schema = json.loads(read("content/missions/schema/mission.schema.json"))
    runtime_props = schema["properties"]["runtime"]["properties"]
    evaluators = set(runtime_props["evaluator"]["enum"])
    if not {"measure-and-build", "observe-ecosystem", "composable"}.issubset(evaluators):
        fail("Mission schema must preserve legacy evaluators and add composable")

    primitive_schema = runtime_props.get("evidencePrimitives", {}).get("items", {}).get("properties", {}).get("primitiveId", {})
    schema_primitives = set(primitive_schema.get("enum", []))
    if schema_primitives != set(PRIMITIVES):
        fail(f"Evidence primitive allowlist drifted: {sorted(schema_primitives)}")

    fixture = json.loads(read("content/missions/examples/composable-eclipse-engine-proof.json"))
    if fixture["runtime"]["evaluator"] != "composable":
        fail("M5.2 fixture must use composable evaluator")
    fixture_primitives = {item["primitiveId"] for item in fixture["runtime"]["evidencePrimitives"]}
    if len(fixture_primitives) < 3:
        fail("M5.2 fixture must demonstrate composition across at least three primitives")
    if len(fixture.get("secondaryDisciplines", [])) < 2:
        fail("M5.2 fixture must demonstrate multidisciplinary metadata")
    if fixture.get("runtime", {}).get("prototypeOnly") is not True:
        fail("M5.2 fixture is a source proof and must remain prototypeOnly=true")

    types_h = read("game/Source/WorldMakers/Mission/WMMissionTypes.h")
    for token in (
        "FWMComposableEvidenceRequirement",
        "FName PrimitiveId",
        "int32 RequiredCount = 1",
        "TArray<FWMComposableEvidenceRequirement> ComposableRequirements",
        "bool IsComposable() const",
        "IsSupportedEvidencePrimitive",
        "RecordComposableEvidence",
        "RecordedComposableEvidenceCounts",
    ):
        if token not in types_h:
            fail(f"Mission types missing M5.2 token: {token}")

    # Privacy/minimization boundary: the generic evidence record may carry stable IDs
    # and a numeric value, but no child-authored free text.
    evidence_block = types_h.split("struct WORLDMAKERS_API FWMLearningEvidenceRecord", 1)[1].split("USTRUCT", 1)[0]
    if "FString" in evidence_block or "FText" in evidence_block:
        fail("Composable learning evidence must not retain free-text child responses")

    types_cpp = read("game/Source/WorldMakers/Mission/WMMissionTypes.cpp")
    for primitive in PRIMITIVES:
        if primitive not in types_cpp:
            fail(f"C++ primitive allowlist missing {primitive}")
    for token in (
        'ComposableEvaluator(TEXT("composable"))',
        'TEXT("evidencePrimitives")',
        "FindComposableRequirement",
        "RecordedComposableEvidenceCounts.Add",
        "bAllRequirementsSatisfied",
        "RequiredTotal",
        "RecordedTotal",
    ):
        if token not in types_cpp:
            fail(f"Composable runtime implementation missing token: {token}")

    subsystem_h = read("game/Source/WorldMakers/Mission/WMMissionRuntimeSubsystem.h")
    subsystem_cpp = read("game/Source/WorldMakers/Mission/WMMissionRuntimeSubsystem.cpp")
    for token in (
        "RecordComposableEvidence",
        "IsComposableEvidenceRequired",
        "GetRequiredEvidencePrimitiveIds",
        "GetRecordedComposableEvidenceCount",
    ):
        if token not in subsystem_h or token not in subsystem_cpp:
            fail(f"Mission subsystem missing composable API: {token}")

    tests = read("game/Source/WorldMakers/Private/Tests/WMMissionRuntimeTests.cpp")
    for test_name in (
        "WorldMakers.Missions.MeasureAndBuild.RequiresMeasurementBeforeBuild",
        "WorldMakers.Missions.Definition.ParsesRuntimeContract",
        "WorldMakers.Missions.Composable.CompletesAcrossEvidencePrimitives",
        "WorldMakers.Missions.Composable.ParsesEvidencePrimitiveContract",
    ):
        if test_name not in tests:
            fail(f"Missing M5.2 automation test: {test_name}")
    if "memorize-worksheet" not in tests:
        fail("M5.2 tests must include a negative unsupported-primitive assertion")

    docs = read("docs/m5-2-composable-mission-runtime.md")
    for token in (
        "predict-test-revise",
        "argue-and-revise",
        "requiredCount",
        "Backward compatibility",
        "Privacy boundary",
        "native Unreal",
    ):
        if token not in docs:
            fail(f"M5.2 documentation missing contract token: {token}")

    print(
        "M5.2 Composable Mission Runtime passed: 10 reusable evidence primitives, "
        "multidisciplinary fixture, bounded counts, legacy evaluator compatibility and minimized evidence API are wired."
    )


if __name__ == "__main__":
    main()
