#if WITH_DEV_AUTOMATION_TESTS

#include "Environment/WMBiomeTypes.h"
#include "Environment/WMInteractionComponent.h"
#include "Misc/AutomationTest.h"

namespace
{
    const TCHAR* InteractionBiomeJson = LR"JSON({
        "schemaVersion": 1,
        "id": "biome.interaction-test",
        "prototypeOnly": true,
        "originCm": [0, 0, 0],
        "zones": [
            {"id":"zone.test.center","kind":"build-clearing","centerCm":[0,0,0],"extentCm":[100,100,100],"buildAllowed":true,"missionIds":[]},
            {"id":"zone.test.east","kind":"exploration","centerCm":[500,0,0],"extentCm":[150,150,150],"buildAllowed":false,"missionIds":[]}
        ],
        "pointsOfInterest": [
            {
                "id":"poi.test.tree",
                "zoneId":"zone.test.east",
                "category":"tree",
                "locationCm":[500,0,0],
                "discoveryRadiusCm":180,
                "discoveryId":"discovery.test.tree",
                "requiresInteraction":true,
                "interactionMode":"observe",
                "interactionRadiusCm":600,
                "focusRadiusCm":100,
                "promptKey":"interaction.test.tree.observe",
                "observationId":"observation.test.tree"
            }
        ]
    })JSON";
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMInteractionDefinitionMetadataTest,
    "WorldMakers.Interaction.Definition.DeliberateMetadata",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMInteractionDefinitionMetadataTest::RunTest(const FString& Parameters)
{
    FWMBiomeRuntimeDefinition Definition;
    FString Error;
    TestTrue(TEXT("Interaction biome JSON parses"), FWMBiomeRuntimeDefinition::TryParseJson(InteractionBiomeJson, Definition, Error));
    if (Definition.PointsOfInterest.IsEmpty())
    {
        AddError(Error.IsEmpty() ? TEXT("Interaction POI missing") : Error);
        return false;
    }

    const FWMPointOfInterestDefinition& Point = Definition.PointsOfInterest[0];
    TestTrue(TEXT("POI explicitly requires interaction"), Point.bRequiresInteraction);
    TestEqual(TEXT("Interaction mode stable"), Point.InteractionMode, FName(TEXT("observe")));
    TestEqual(TEXT("Observation ID stable"), Point.ObservationId, FName(TEXT("observation.test.tree")));
    TestEqual(TEXT("Prompt localization key stable"), Point.PromptKey, FName(TEXT("interaction.test.tree.observe")));
    TestTrue(TEXT("Player inside interaction radius is eligible"), Point.IsWithinInteractionRange(FVector::ZeroVector, Definition.OriginCm));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMInteractionFocusRulesTest,
    "WorldMakers.Interaction.Focus.RequiresAimAndRange",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMInteractionFocusRulesTest::RunTest(const FString& Parameters)
{
    const FVector ViewLocation = FVector::ZeroVector;
    const FVector ViewForward = FVector::ForwardVector;
    TestTrue(TEXT("Target in front and inside range is focusable"),
        UWMInteractionComponent::IsFocusCandidate(ViewLocation, ViewForward, FVector(500.0f, 0.0f, 0.0f), 700.0f, 0.90f));
    TestFalse(TEXT("Target to the side is not focusable"),
        UWMInteractionComponent::IsFocusCandidate(ViewLocation, ViewForward, FVector(0.0f, 500.0f, 0.0f), 700.0f, 0.90f));
    TestFalse(TEXT("Target beyond max focus distance is not focusable"),
        UWMInteractionComponent::IsFocusCandidate(ViewLocation, ViewForward, FVector(900.0f, 0.0f, 0.0f), 700.0f, 0.90f));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMInteractionObservationIdempotencyTest,
    "WorldMakers.Interaction.Observation.IdempotentStableIds",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMInteractionObservationIdempotencyTest::RunTest(const FString& Parameters)
{
    FWMExplorationProgressModel Progress;
    const FName ObservationId(TEXT("observation.test.tree"));
    const FName DiscoveryId(TEXT("discovery.test.tree"));

    TestTrue(TEXT("First observation registers"), Progress.RegisterObservation(ObservationId));
    TestFalse(TEXT("Duplicate observation does not register twice"), Progress.RegisterObservation(ObservationId));
    TestTrue(TEXT("Observation can be queried by stable ID"), Progress.HasObserved(ObservationId));
    TestEqual(TEXT("Only one observation stored"), Progress.GetObservedIds().Num(), 1);

    TestTrue(TEXT("First discovery registers independently"), Progress.RegisterDiscovery(DiscoveryId));
    TestFalse(TEXT("Duplicate discovery remains idempotent"), Progress.RegisterDiscovery(DiscoveryId));
    return true;
}

#endif
