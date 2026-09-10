#if WITH_DEV_AUTOMATION_TESTS

#include "Environment/WMEnvironmentStateTypes.h"
#include "Misc/AutomationTest.h"

namespace
{
    const TCHAR* TestEnvironmentJson = LR"JSON({
      "schemaVersion": 1,
      "biomeId": "biome.test",
      "prototypeOnly": true,
      "baseline": {
        "vegetationHealth": 0.30,
        "waterFlow": 0.30,
        "soilProtection": 0.25,
        "shadeCoverage": 0.35
      },
      "actions": [
        {
          "actionId": "action.test.soil",
          "targetId": "target.test.soil",
          "promptKey": "interaction.test.soil",
          "locationCm": [0, 0, 0],
          "interactionRadiusCm": 400,
          "focusRadiusCm": 100,
          "maxApplications": 1,
          "delta": {"vegetationHealth": 0.08, "waterFlow": 0.0, "soilProtection": 0.45, "shadeCoverage": 0.0}
        },
        {
          "actionId": "action.test.shade",
          "targetId": "target.test.shade",
          "promptKey": "interaction.test.shade",
          "locationCm": [300, 0, 0],
          "interactionRadiusCm": 400,
          "focusRadiusCm": 100,
          "maxApplications": 1,
          "delta": {"vegetationHealth": 0.09, "waterFlow": 0.0, "soilProtection": 0.0, "shadeCoverage": 0.45}
        },
        {
          "actionId": "action.test.water",
          "targetId": "target.test.water",
          "promptKey": "interaction.test.water",
          "locationCm": [600, 0, 0],
          "interactionRadiusCm": 400,
          "focusRadiusCm": 100,
          "maxApplications": 1,
          "delta": {"vegetationHealth": 0.08, "waterFlow": 0.45, "soilProtection": 0.0, "shadeCoverage": 0.0}
        }
      ],
      "reactionBands": [
        {"reactionId": "reaction.test.stressed", "maxHabitatQuality": 0.34},
        {"reactionId": "reaction.test.recovering", "maxHabitatQuality": 0.64},
        {"reactionId": "reaction.test.thriving", "maxHabitatQuality": 1.0}
      ]
    })JSON";

    bool ParseTestDefinition(FWMEnvironmentStateDefinition& OutDefinition, FAutomationTestBase& Test)
    {
        FString Error;
        if (!FWMEnvironmentStateDefinition::TryParseJson(TestEnvironmentJson, OutDefinition, Error))
        {
            Test.AddError(Error);
            return false;
        }
        return true;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMEnvironmentDefinitionParseTest,
    "WorldMakers.Environment.Definition.ParsesReactiveEcosystemProfile",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMEnvironmentDefinitionParseTest::RunTest(const FString& Parameters)
{
    FWMEnvironmentStateDefinition Definition;
    TestTrue(TEXT("Reactive ecosystem profile parses"), ParseTestDefinition(Definition, *this));
    TestEqual(TEXT("Stable biome ID preserved"), Definition.BiomeId, FName(TEXT("biome.test")));
    TestEqual(TEXT("Three care actions loaded"), Definition.Actions.Num(), 3);
    TestEqual(TEXT("Three reaction bands loaded"), Definition.ReactionBands.Num(), 3);
    TestTrue(TEXT("Habitat derives from primary dimensions"), FMath::IsNearlyEqual(
        FWMEnvironmentStateDefinition::DeriveHabitatQuality(0.30f, 0.30f, 0.25f, 0.35f), 0.30f));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMEnvironmentDeterministicTransitionTest,
    "WorldMakers.Environment.State.DeterministicBoundedTransitions",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMEnvironmentDeterministicTransitionTest::RunTest(const FString& Parameters)
{
    FWMEnvironmentStateDefinition Definition;
    if (!ParseTestDefinition(Definition, *this)) return false;

    FWMEnvironmentStateModel Model;
    TestTrue(TEXT("State initializes"), Model.Initialize(Definition));
    TestEqual(TEXT("Baseline reaction is stressed"), Model.GetSnapshot().ReactionId, FName(TEXT("reaction.test.stressed")));

    TestTrue(TEXT("Soil care applies once"), Model.ApplyAction(TEXT("action.test.soil")));
    TestFalse(TEXT("Soil care cannot be farmed beyond configured cap"), Model.ApplyAction(TEXT("action.test.soil")));
    TestEqual(TEXT("Soil action count remains one"), Model.GetAppliedCount(TEXT("action.test.soil")), 1);
    TestTrue(TEXT("State remains bounded after action"), Model.GetSnapshot().IsBounded());
    TestEqual(TEXT("First care action moves ecosystem to recovering"), Model.GetSnapshot().ReactionId, FName(TEXT("reaction.test.recovering")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMEnvironmentReactionBandTest,
    "WorldMakers.Environment.State.ReactionBands",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMEnvironmentReactionBandTest::RunTest(const FString& Parameters)
{
    FWMEnvironmentStateDefinition Definition;
    if (!ParseTestDefinition(Definition, *this)) return false;

    FWMEnvironmentStateModel Model;
    if (!Model.Initialize(Definition)) return false;

    TestTrue(TEXT("Soil action accepted"), Model.ApplyAction(TEXT("action.test.soil")));
    TestTrue(TEXT("Shade action accepted"), Model.ApplyAction(TEXT("action.test.shade")));
    TestTrue(TEXT("Water action accepted"), Model.ApplyAction(TEXT("action.test.water")));

    const FWMEnvironmentStateSnapshot Snapshot = Model.GetSnapshot();
    TestTrue(TEXT("Final habitat quality is deterministic"), FMath::IsNearlyEqual(Snapshot.HabitatQuality, 0.70f));
    TestEqual(TEXT("All care actions reach thriving band"), Snapshot.ReactionId, FName(TEXT("reaction.test.thriving")));
    TestTrue(TEXT("Final state remains normalized"), Snapshot.IsBounded());
    return true;
}

#endif
