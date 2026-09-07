#include "Mission/WMMissionRuntimeSubsystem.h"

#include "Mission/WMMissionGeometryActor.h"
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
        LastMeasuredSpanCm = 0.0f;
        return false;
    }

    FWMMissionRuntimeDefinition Definition;
    FString Error;
    if (!FWMMissionRuntimeDefinition::TryParseJson(Json, Definition, Error))
    {
        Progress = FWMMissionProgressModel();
        LastMeasuredSpanCm = 0.0f;
        return false;
    }

    LastMeasuredSpanCm = 0.0f;
    return Progress.Begin(Definition);
}

bool UWMMissionRuntimeSubsystem::RecordMeasurement(const float MeasuredSpanCm)
{
    const bool bRecorded = Progress.RecordMeasurement(MeasuredSpanCm);
    if (bRecorded)
    {
        LastMeasuredSpanCm = MeasuredSpanCm;
    }
    return bRecorded;
}

bool UWMMissionRuntimeSubsystem::RecordActiveGeometryMeasurement()
{
    if (!ActiveGeometry.IsValid() || Progress.State != EWMMissionRuntimeState::Active)
    {
        return false;
    }
    return RecordMeasurement(ActiveGeometry->GetMeasuredSpanCm());
}

bool UWMMissionRuntimeSubsystem::RecordStructureSpan(const float StructureSpanCm)
{
    return Progress.RecordStructureSpan(StructureSpanCm);
}

void UWMMissionRuntimeSubsystem::RegisterMissionGeometry(AWMMissionGeometryActor* GeometryActor)
{
    if (IsValid(GeometryActor))
    {
        ActiveGeometry = GeometryActor;
    }
}

void UWMMissionRuntimeSubsystem::UnregisterMissionGeometry(AWMMissionGeometryActor* GeometryActor)
{
    if (ActiveGeometry.Get() == GeometryActor)
    {
        ActiveGeometry.Reset();
    }
}
