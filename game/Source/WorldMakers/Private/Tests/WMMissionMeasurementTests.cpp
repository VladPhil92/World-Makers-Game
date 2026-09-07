#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Mission/WMMissionMeasurementComponent.h"
#include "Mission/WMMissionTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMMissionMeasurementStateTest,
    "WorldMakers.Missions.Measurement.CapturesTwoValidPoints",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMMissionMeasurementStateTest::RunTest(const FString& Parameters)
{
    FWMMissionMeasurementModel Model;
    Model.Begin(TEXT("mission.mathematics.measure-and-build-01"));

    float DistanceCm = 0.0f;
    TestEqual(TEXT("New session awaits point A"), Model.State, EWMMissionMeasurementState::AwaitingFirstPoint);
    TestFalse(TEXT("Outside-zone point is rejected"), Model.CapturePoint(FVector::ZeroVector, false, 5.0f, DistanceCm));
    TestEqual(TEXT("Rejected point does not advance state"), Model.State, EWMMissionMeasurementState::AwaitingFirstPoint);

    TestTrue(TEXT("Point A is accepted"), Model.CapturePoint(FVector(0.0f, 0.0f, 0.0f), true, 5.0f, DistanceCm));
    TestEqual(TEXT("Session now awaits point B"), Model.State, EWMMissionMeasurementState::AwaitingSecondPoint);

    TestFalse(TEXT("Point B below minimum separation is rejected"), Model.CapturePoint(FVector(2.0f, 0.0f, 0.0f), true, 5.0f, DistanceCm));
    TestEqual(TEXT("Near point does not complete session"), Model.State, EWMMissionMeasurementState::AwaitingSecondPoint);

    TestTrue(TEXT("Valid point B completes measurement"), Model.CapturePoint(FVector(300.0f, 0.0f, 0.0f), true, 5.0f, DistanceCm));
    TestEqual(TEXT("Session completes"), Model.State, EWMMissionMeasurementState::Complete);
    TestEqual(TEXT("Distance is measured from selected points"), DistanceCm, 300.0f);
    TestEqual(TEXT("Model stores completed distance"), Model.LastDistanceCm, 300.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMMissionCatalogMultipleDefinitionsTest,
    "WorldMakers.Missions.Catalog.MultipleDefinitions",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMMissionCatalogMultipleDefinitionsTest::RunTest(const FString& Parameters)
{
    const FString MissionDirectory = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("WorldMakers/Missions"));
    TArray<FString> MissionFiles;
    IFileManager::Get().FindFiles(MissionFiles, *FPaths::Combine(MissionDirectory, TEXT("*.json")), true, false);
    TestTrue(TEXT("At least two packaged mission definitions exist"), MissionFiles.Num() >= 2);

    TSet<FName> MissionIds;
    for (const FString& MissionFile : MissionFiles)
    {
        FString Json;
        if (!FFileHelper::LoadFileToString(Json, *FPaths::Combine(MissionDirectory, MissionFile)))
        {
            AddError(FString::Printf(TEXT("Unable to read mission file: %s"), *MissionFile));
            continue;
        }

        FWMMissionRuntimeDefinition Definition;
        FString Error;
        if (!FWMMissionRuntimeDefinition::TryParseJson(Json, Definition, Error))
        {
            AddError(FString::Printf(TEXT("Unable to parse mission file %s: %s"), *MissionFile, *Error));
            continue;
        }
        MissionIds.Add(Definition.MissionId);
    }

    TestTrue(TEXT("Catalog includes original 300 cm mission"), MissionIds.Contains(TEXT("mission.mathematics.measure-and-build-01")));
    TestTrue(TEXT("Catalog includes second 200 cm mission"), MissionIds.Contains(TEXT("mission.mathematics.measure-and-build-02")));
    TestEqual(TEXT("Packaged mission IDs are unique"), MissionIds.Num(), MissionFiles.Num());
    return true;
}
