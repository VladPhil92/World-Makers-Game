#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Mission/WMMissionTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMMeasureAndBuildOrderTest,
    "WorldMakers.Missions.MeasureAndBuild.RequiresMeasurementBeforeBuild",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMMeasureAndBuildOrderTest::RunTest(const FString& Parameters)
{
    FWMMissionRuntimeDefinition Definition;
    Definition.MissionId = TEXT("mission.mathematics.measure-and-build-01");
    Definition.LearningObjectiveIds = { TEXT("math.measure.compare-lengths"), TEXT("math.spatial.plan-to-constraint") };
    Definition.RequiredEvidenceEventIds = { TEXT("measurement_used_before_build"), TEXT("structure_fits_target_span") };
    Definition.TargetSpanCm = 300.0f;
    Definition.ToleranceCm = 20.0f;
    Definition.RewardIds = { TEXT("reward.math.measurement-tool-01"), TEXT("reward.math.spatial-builder-badge-01") };

    FWMMissionProgressModel Progress;
    TestTrue(TEXT("Mission begins"), Progress.Begin(Definition));
    TestFalse(TEXT("Build evidence is rejected before measurement"), Progress.RecordStructureSpan(300.0f));
    TestEqual(TEXT("State remains active"), Progress.State, EWMMissionRuntimeState::Active);
    TestTrue(TEXT("Measurement is accepted"), Progress.RecordMeasurement(300.0f));
    TestFalse(TEXT("Out-of-tolerance structure does not complete"), Progress.RecordStructureSpan(260.0f));
    TestTrue(TEXT("In-tolerance structure completes"), Progress.RecordStructureSpan(305.0f));
    TestEqual(TEXT("Mission completes"), Progress.State, EWMMissionRuntimeState::Completed);
    TestEqual(TEXT("Two minimized evidence records"), Progress.Evidence.Num(), 2);
    TestEqual(TEXT("Two deterministic reward IDs"), Progress.EarnedRewardIds.Num(), 2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMMissionJsonParsingTest,
    "WorldMakers.Missions.Definition.ParsesRuntimeContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMMissionJsonParsingTest::RunTest(const FString& Parameters)
{
    const FString Json = TEXT(R"JSON({
        "id":"mission.mathematics.measure-and-build-01",
        "learningObjectives":["math.measure.compare-lengths","math.spatial.plan-to-constraint"],
        "evidenceEvents":["measurement_used_before_build","structure_fits_target_span"],
        "runtime":{"evaluator":"measure-and-build","targetSpanCm":300,"toleranceCm":20,"rewardIds":["reward.math.measurement-tool-01"],"prototypeOnly":true}
    })JSON");

    FWMMissionRuntimeDefinition Definition;
    FString Error;
    TestTrue(TEXT("Runtime contract parses"), FWMMissionRuntimeDefinition::TryParseJson(Json, Definition, Error));
    TestEqual(TEXT("Mission ID preserved"), Definition.MissionId, FName(TEXT("mission.mathematics.measure-and-build-01")));
    TestEqual(TEXT("Target span preserved"), Definition.TargetSpanCm, 300.0f);
    return true;
}

#endif
