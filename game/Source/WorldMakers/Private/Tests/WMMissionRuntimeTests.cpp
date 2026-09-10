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
    Definition.Evaluator = TEXT("measure-and-build");
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMComposableMissionProgressTest,
    "WorldMakers.Missions.Composable.CompletesAcrossEvidencePrimitives",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMComposableMissionProgressTest::RunTest(const FString& Parameters)
{
    FWMMissionRuntimeDefinition Definition;
    Definition.MissionId = TEXT("mission.prototype.eclipse-engine-thinking-loop");
    Definition.Evaluator = TEXT("composable");
    Definition.LearningObjectiveIds = {
        TEXT("chemistry.inquiry.predict-and-revise"),
        TEXT("philosophy.argument.revise-with-reasons")
    };
    Definition.RequiredEvidenceEventIds = {
        TEXT("eclipse.prediction-revised"),
        TEXT("eclipse.argument-revised")
    };
    Definition.RewardIds = { TEXT("reward.science.rainforest-observer-badge-01") };

    FWMComposableEvidenceRequirement Prediction;
    Prediction.PrimitiveId = TEXT("predict-test-revise");
    Prediction.EvidenceEventId = TEXT("eclipse.prediction-revised");
    Prediction.ObjectiveId = TEXT("chemistry.inquiry.predict-and-revise");
    Prediction.RequiredCount = 2;

    FWMComposableEvidenceRequirement Argument;
    Argument.PrimitiveId = TEXT("argue-and-revise");
    Argument.EvidenceEventId = TEXT("eclipse.argument-revised");
    Argument.ObjectiveId = TEXT("philosophy.argument.revise-with-reasons");
    Argument.RequiredCount = 1;

    Definition.ComposableRequirements = { Prediction, Argument };

    FWMMissionProgressModel Progress;
    TestTrue(TEXT("Composable mission begins"), Progress.Begin(Definition));
    TestFalse(TEXT("Wrong primitive cannot satisfy an event"), Progress.RecordComposableEvidence(TEXT("model-system"), TEXT("eclipse.prediction-revised")));
    TestTrue(TEXT("First prediction/revision is accepted"), Progress.RecordComposableEvidence(TEXT("predict-test-revise"), TEXT("eclipse.prediction-revised"), 0.5f));
    TestEqual(TEXT("Progress is one third"), Progress.GetProgressFraction(), 1.0f / 3.0f);
    TestTrue(TEXT("Second required prediction/revision is accepted"), Progress.RecordComposableEvidence(TEXT("predict-test-revise"), TEXT("eclipse.prediction-revised"), 1.0f));
    TestFalse(TEXT("Evidence beyond required count is rejected"), Progress.RecordComposableEvidence(TEXT("predict-test-revise"), TEXT("eclipse.prediction-revised"), 1.0f));
    TestEqual(TEXT("Mission still active before final primitive"), Progress.State, EWMMissionRuntimeState::Active);
    TestTrue(TEXT("Argument revision is accepted"), Progress.RecordComposableEvidence(TEXT("argue-and-revise"), TEXT("eclipse.argument-revised"), 1.0f));
    TestEqual(TEXT("Composable mission completes"), Progress.State, EWMMissionRuntimeState::Completed);
    TestEqual(TEXT("Three minimized evidence records retained"), Progress.Evidence.Num(), 3);
    TestEqual(TEXT("Primitive attribution is retained"), Progress.Evidence[0].PrimitiveId, FName(TEXT("predict-test-revise")));
    TestEqual(TEXT("Completed progress is one"), Progress.GetProgressFraction(), 1.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMComposableMissionJsonParsingTest,
    "WorldMakers.Missions.Composable.ParsesEvidencePrimitiveContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMComposableMissionJsonParsingTest::RunTest(const FString& Parameters)
{
    const FString Json = TEXT(R"JSON({
        "id":"mission.prototype.composable-runtime-proof",
        "learningObjectives":["physics.systems.model-cause-effect","ethics.reasoning.compare-reasons"],
        "evidenceEvents":["proof.model-completed","proof.reason-revised"],
        "runtime":{
            "evaluator":"composable",
            "evidencePrimitives":[
                {"primitiveId":"model-system","evidenceEventId":"proof.model-completed","objectiveId":"physics.systems.model-cause-effect"},
                {"primitiveId":"reason-through-dilemma","evidenceEventId":"proof.reason-revised","objectiveId":"ethics.reasoning.compare-reasons","requiredCount":2}
            ],
            "rewardIds":["reward.science.rainforest-observer-badge-01"],
            "prototypeOnly":true
        }
    })JSON");

    FWMMissionRuntimeDefinition Definition;
    FString Error;
    TestTrue(TEXT("Composable runtime contract parses"), FWMMissionRuntimeDefinition::TryParseJson(Json, Definition, Error));
    TestTrue(TEXT("Definition is composable"), Definition.IsComposable());
    TestEqual(TEXT("Two primitives parsed"), Definition.ComposableRequirements.Num(), 2);
    TestEqual(TEXT("Required count preserved"), Definition.ComposableRequirements[1].RequiredCount, 2);
    TestTrue(TEXT("Primitive allowlist accepts philosophy reasoning"), FWMMissionRuntimeDefinition::IsSupportedEvidencePrimitive(TEXT("argue-and-revise")));
    TestFalse(TEXT("Unknown primitive is rejected"), FWMMissionRuntimeDefinition::IsSupportedEvidencePrimitive(TEXT("memorize-worksheet")));
    return true;
}

#endif
