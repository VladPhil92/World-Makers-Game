#include "Mission/WMMissionRuntimeSubsystem.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

void UWMMissionRuntimeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    ReloadAndActivatePrototypeMission();
}

bool UWMMissionRuntimeSubsystem::ReloadAndActivatePrototypeMission()
{
    const FString MissionPath = FPaths::Combine(
        FPaths::ProjectContentDir(),
        TEXT("WorldMakers/Missions/mission.mathematics.measure-and-build-01.json"));

    FString Json;
    if (!FFileHelper::LoadFileToString(Json, *MissionPath))
    {
        Progress = FWMMissionProgressModel();
        return false;
    }

    FWMMissionRuntimeDefinition Definition;
    FString Error;
    if (!FWMMissionRuntimeDefinition::TryParseJson(Json, Definition, Error))
    {
        Progress = FWMMissionProgressModel();
        return false;
    }

    return Progress.Begin(Definition);
}

bool UWMMissionRuntimeSubsystem::RecordMeasurement(const float MeasuredSpanCm)
{
    return Progress.RecordMeasurement(MeasuredSpanCm);
}

bool UWMMissionRuntimeSubsystem::RecordStructureSpan(const float StructureSpanCm)
{
    return Progress.RecordStructureSpan(StructureSpanCm);
}
