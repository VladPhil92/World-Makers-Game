#if WITH_DEV_AUTOMATION_TESTS

#include "Building/WMBuildCatalogSettings.h"
#include "Building/WMBuildUnlockSubsystem.h"
#include "Environment/WMEcologicalBuildTypes.h"
#include "Misc/AutomationTest.h"

namespace
{
    const TCHAR* TestEcologicalBuildJson = LR"JSON({
      "schemaVersion": 1,
      "biomeId": "biome.test",
      "prototypeOnly": true,
      "interventions": [
        {
          "interventionId": "intervention.test.soil",
          "anchorCm": [0, 0, 0],
          "radiusCm": 300,
          "prerequisiteInterventionIds": [],
          "requirements": [{"pieceId": "prototype.wall", "minCount": 2}],
          "effectDelta": {"vegetationHealth": 0.02, "waterFlow": 0.0, "soilProtection": 0.15, "shadeCoverage": 0.0},
          "rewardId": "reward.test.roof"
        },
        {
          "interventionId": "intervention.test.shade",
          "anchorCm": [800, 0, 0],
          "radiusCm": 300,
          "prerequisiteInterventionIds": ["intervention.test.soil"],
          "requirements": [{"pieceId": "eco.leaf-roof", "minCount": 1}],
          "effectDelta": {"vegetationHealth": 0.02, "waterFlow": 0.0, "soilProtection": 0.0, "shadeCoverage": 0.15},
          "rewardId": "reward.test.planter"
        }
      ]
    })JSON";

    FWMPlacedBuildPieceSnapshot MakePiece(const TCHAR* PieceId, const FVector& Location)
    {
        FWMPlacedBuildPieceSnapshot Piece;
        Piece.PieceId = FName(PieceId);
        Piece.LocationCm = Location;
        return Piece;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMEcologicalBuildDefinitionTest,
    "WorldMakers.Environment.Building.Definition.ParsesInterventionSequence",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMEcologicalBuildDefinitionTest::RunTest(const FString& Parameters)
{
    FWMEcologicalBuildDefinition Definition;
    FString Error;
    TestTrue(TEXT("Ecological build definition parses"), FWMEcologicalBuildDefinition::TryParseJson(TestEcologicalBuildJson, Definition, Error));
    TestEqual(TEXT("Two interventions parsed"), Definition.Interventions.Num(), 2);
    TestEqual(TEXT("Second intervention depends on first"), Definition.Interventions[1].PrerequisiteInterventionIds[0], FName(TEXT("intervention.test.soil")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMEcologicalBuildSpatialEvaluationTest,
    "WorldMakers.Environment.Building.Evaluation.SpatialAndSequential",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMEcologicalBuildSpatialEvaluationTest::RunTest(const FString& Parameters)
{
    FWMEcologicalBuildDefinition Definition;
    FString Error;
    if (!FWMEcologicalBuildDefinition::TryParseJson(TestEcologicalBuildJson, Definition, Error))
    {
        AddError(Error);
        return false;
    }

    TArray<FWMPlacedBuildPieceSnapshot> Pieces;
    Pieces.Add(MakePiece(TEXT("prototype.wall"), FVector(100.0f, 0.0f, 100.0f)));
    Pieces.Add(MakePiece(TEXT("prototype.wall"), FVector(-100.0f, 0.0f, 100.0f)));
    Pieces.Add(MakePiece(TEXT("eco.leaf-roof"), FVector(800.0f, 0.0f, 200.0f)));

    FWMEcologicalBuildProgressModel Progress;
    TestTrue(TEXT("Soil intervention is spatially satisfied"), Progress.CanComplete(Definition.Interventions[0], Pieces));
    TestFalse(TEXT("Shade remains locked before soil completion"), Progress.CanComplete(Definition.Interventions[1], Pieces));
    TestTrue(TEXT("Soil completion is idempotently recorded"), Progress.MarkCompleted(TEXT("intervention.test.soil")));
    TestFalse(TEXT("Soil completion cannot be duplicated"), Progress.MarkCompleted(TEXT("intervention.test.soil")));
    TestTrue(TEXT("Shade unlocks immediately from structure state after prerequisite"), Progress.CanComplete(Definition.Interventions[1], Pieces));

    Pieces[0].LocationCm = FVector(5000.0f, 0.0f, 100.0f);
    Progress.Reset();
    TestFalse(TEXT("Global piece count cannot satisfy an intervention outside its area"), Progress.CanComplete(Definition.Interventions[0], Pieces));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMBuildUnlockModelTest,
    "WorldMakers.Building.Unlocks.RewardGateIsIdempotent",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMBuildUnlockModelTest::RunTest(const FString& Parameters)
{
    FWMBuildPieceSpec BasePiece;
    BasePiece.PieceId = TEXT("prototype.base");
    BasePiece.DimensionsCm = FVector(100.0f);

    FWMBuildPieceSpec RewardPiece = BasePiece;
    RewardPiece.PieceId = TEXT("eco.reward-piece");
    RewardPiece.RequiredRewardId = TEXT("reward.eco.test-unlock");

    FWMBuildUnlockModel Unlocks;
    TestTrue(TEXT("Base piece requires no reward"), Unlocks.IsPieceUnlocked(BasePiece));
    TestFalse(TEXT("Creative piece begins locked"), Unlocks.IsPieceUnlocked(RewardPiece));
    TestTrue(TEXT("First reward grant changes state"), Unlocks.Grant(TEXT("reward.eco.test-unlock")));
    TestFalse(TEXT("Duplicate reward grant is idempotent"), Unlocks.Grant(TEXT("reward.eco.test-unlock")));
    TestTrue(TEXT("Creative piece unlocks after reward"), Unlocks.IsPieceUnlocked(RewardPiece));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMEnvironmentTrustedBuildEffectTest,
    "WorldMakers.Environment.Building.Effect.BoundedOneShot",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMEnvironmentTrustedBuildEffectTest::RunTest(const FString& Parameters)
{
    const TCHAR* EnvironmentJson = LR"JSON({
      "schemaVersion": 1,
      "biomeId": "biome.test",
      "prototypeOnly": true,
      "baseline": {"vegetationHealth": 0.3, "waterFlow": 0.3, "soilProtection": 0.25, "shadeCoverage": 0.35},
      "actions": [
        {"actionId":"action.test.a","targetId":"target.test.a","promptKey":"interaction.test.a","locationCm":[0,0,0],"interactionRadiusCm":400,"focusRadiusCm":100,"maxApplications":1,"delta":{"vegetationHealth":0.1,"waterFlow":0,"soilProtection":0,"shadeCoverage":0}},
        {"actionId":"action.test.b","targetId":"target.test.b","promptKey":"interaction.test.b","locationCm":[100,0,0],"interactionRadiusCm":400,"focusRadiusCm":100,"maxApplications":1,"delta":{"vegetationHealth":0,"waterFlow":0.1,"soilProtection":0,"shadeCoverage":0}},
        {"actionId":"action.test.c","targetId":"target.test.c","promptKey":"interaction.test.c","locationCm":[200,0,0],"interactionRadiusCm":400,"focusRadiusCm":100,"maxApplications":1,"delta":{"vegetationHealth":0,"waterFlow":0,"soilProtection":0.1,"shadeCoverage":0}}
      ],
      "reactionBands": [
        {"reactionId":"reaction.test.low","maxHabitatQuality":0.5},
        {"reactionId":"reaction.test.high","maxHabitatQuality":1.0}
      ]
    })JSON";

    FWMEnvironmentStateDefinition Definition;
    FString Error;
    if (!FWMEnvironmentStateDefinition::TryParseJson(EnvironmentJson, Definition, Error))
    {
        AddError(Error);
        return false;
    }

    FWMEnvironmentStateModel Model;
    TestTrue(TEXT("Environment state initializes"), Model.Initialize(Definition));
    FWMEnvironmentStateDelta Delta;
    Delta.SoilProtection = 0.25f;
    TestTrue(TEXT("Trusted build effect applies once"), Model.ApplyTrustedEffect(TEXT("intervention.test.effect"), Delta, 1));
    TestFalse(TEXT("Trusted build effect cannot farm state"), Model.ApplyTrustedEffect(TEXT("intervention.test.effect"), Delta, 1));
    TestTrue(TEXT("State remains bounded"), Model.GetSnapshot().IsBounded());
    return true;
}

#endif
