#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Mission/WMMissionTypes.h"

namespace
{
    const TCHAR* ScienceMissionJson = LR"JSON({
        "id":"mission.science.test-ecosystem",
        "axis":"science",
        "ageBand":"7-8",
        "learningObjectives":["science.ecosystems.observe-interdependence","science.ecosystems.compare-factors"],
        "playerGoalKey":"mission.science.test.goal",
        "mechanics":["exploration","deliberate-observation"],
        "evidenceEvents":["science.test.tree","science.test.plant","science.test.water"],
        "difficulty":2,
        "review":{"pedagogy":"draft","safety":"draft","cultural":"draft"},
        "runtime":{
            "evaluator":"observe-ecosystem",
            "observationRequirements":[
                {"observationId":"observation.test.tree","evidenceEventId":"science.test.tree","objectiveId":"science.ecosystems.observe-interdependence"},
                {"observationId":"observation.test.plant","evidenceEventId":"science.test.plant","objectiveId":"science.ecosystems.observe-interdependence"},
                {"observationId":"observation.test.water","evidenceEventId":"science.test.water","objectiveId":"science.ecosystems.compare-factors"}
            ],
            "rewardIds":["reward.science.test-badge"],
            "prerequisiteMissionIds":[],
            "prototypeOnly":true
        }
    })JSON";

    const TCHAR* MathMissionJson = LR"JSON({
        "id":"mission.mathematics.test-measure",
        "axis":"mathematics",
        "ageBand":"7-8",
        "learningObjectives":["math.measure.compare-lengths","math.spatial.plan-to-constraint"],
        "playerGoalKey":"mission.math.test.goal",
        "mechanics":["measure","build"],
        "evidenceEvents":["measurement_used_before_build","structure_fits_target_span"],
        "difficulty":1,
        "review":{"pedagogy":"draft","safety":"draft"},
        "runtime":{
            "evaluator":"measure-and-build",
            "targetSpanCm":300,
            "toleranceCm":20,
            "rewardIds":["reward.math.test"],
            "prerequisiteMissionIds":[],
            "prototypeOnly":true
        }
    })JSON";
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMScienceMissionDefinitionTest,
    "WorldMakers.Missions.Science.Definition.ParsesObservationEvaluator",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMScienceMissionDefinitionTest::RunTest(const FString& Parameters)
{
    FWMMissionRuntimeDefinition Definition;
    FString Error;
    TestTrue(TEXT("Science mission parses"), FWMMissionRuntimeDefinition::TryParseJson(ScienceMissionJson, Definition, Error));
    TestTrue(TEXT("Science evaluator is explicit"), Definition.IsObserveEcosystem());
    TestEqual(TEXT("Science mission has three observation requirements"), Definition.ObservationRequirements.Num(), 3);
    TestEqual(TEXT("Science mission does not carry a target span"), Definition.TargetSpanCm, 0.0f);
    TestEqual(TEXT("Stable first observation preserved"), Definition.ObservationRequirements[0].ObservationId, FName(TEXT("observation.test.tree")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMScienceMissionProgressTest,
    "WorldMakers.Missions.Science.Progress.OrderIndependentIdempotentObservations",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMScienceMissionProgressTest::RunTest(const FString& Parameters)
{
    FWMMissionRuntimeDefinition Definition;
    FString Error;
    if (!FWMMissionRuntimeDefinition::TryParseJson(ScienceMissionJson, Definition, Error))
    {
        AddError(Error);
        return false;
    }

    FWMMissionProgressModel Progress;
    TestTrue(TEXT("Science mission begins"), Progress.Begin(Definition));
    TestFalse(TEXT("Unrelated observation is rejected"), Progress.RecordObservation(FName(TEXT("observation.test.unknown"))));
    TestTrue(TEXT("Water can be observed first"), Progress.RecordObservation(FName(TEXT("observation.test.water"))));
    TestEqual(TEXT("One third progress after first unique observation"), Progress.GetProgressFraction(), 1.0f / 3.0f);
    TestFalse(TEXT("Duplicate water observation is ignored"), Progress.RecordObservation(FName(TEXT("observation.test.water"))));
    TestEqual(TEXT("Duplicate does not create evidence"), Progress.Evidence.Num(), 1);
    TestTrue(TEXT("Tree can be observed second"), Progress.RecordObservation(FName(TEXT("observation.test.tree"))));
    TestEqual(TEXT("Two observations recorded"), Progress.GetRecordedObservationCount(), 2);
    TestTrue(TEXT("Plant completes mission regardless of order"), Progress.RecordObservation(FName(TEXT("observation.test.plant"))));
    TestEqual(TEXT("Science mission completes after all required observations"), Progress.State, EWMMissionRuntimeState::Completed);
    TestEqual(TEXT("Completion emits exactly three evidence records"), Progress.Evidence.Num(), 3);
    TestEqual(TEXT("Science reward is earned"), Progress.EarnedRewardIds.Num(), 1);
    TestEqual(TEXT("Science progress reaches one"), Progress.GetProgressFraction(), 1.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMScienceMissionMathRegressionTest,
    "WorldMakers.Missions.Science.Regression.MeasureAndBuildStillWorks",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMScienceMissionMathRegressionTest::RunTest(const FString& Parameters)
{
    FWMMissionRuntimeDefinition Definition;
    FString Error;
    TestTrue(TEXT("Mathematics mission still parses"), FWMMissionRuntimeDefinition::TryParseJson(MathMissionJson, Definition, Error));
    TestTrue(TEXT("Mathematics evaluator remains measure-and-build"), Definition.IsMeasureAndBuild());

    FWMMissionProgressModel Progress;
    TestTrue(TEXT("Mathematics mission begins"), Progress.Begin(Definition));
    TestFalse(TEXT("Science observation cannot advance mathematics"), Progress.RecordObservation(FName(TEXT("observation.test.tree"))));
    TestTrue(TEXT("Measurement still records"), Progress.RecordMeasurement(300.0f));
    TestTrue(TEXT("Matching structure still completes"), Progress.RecordStructureSpan(310.0f));
    TestEqual(TEXT("Mathematics mission completes"), Progress.State, EWMMissionRuntimeState::Completed);
    return true;
}

#endif
