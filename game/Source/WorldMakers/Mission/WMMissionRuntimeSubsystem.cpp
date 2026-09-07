#include "Mission/WMMissionRuntimeSubsystem.h"

#include "HAL/FileManager.h"
#include "Mission/WMMissionGeometryActor.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
    const FName DefaultPrototypeMissionId(TEXT("mission.mathematics.measure-and-build-01"));
}

void UWMMissionRuntimeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    ReloadAndActivatePrototypeMission();
}

bool UWMMissionRuntimeSubsystem::ReloadMissionCatalog()
{
    MissionCatalog.Reset();
    AvailableMissionIds.Reset();

    const FString MissionDirectory = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("WorldMakers/Missions"));
    TArray<FString> MissionFiles;
    IFileManager::Get().FindFiles(MissionFiles, *FPaths::Combine(MissionDirectory, TEXT("*.json")), true, false);
    MissionFiles.Sort();

    for (const FString& MissionFile : MissionFiles)
    {
        FString Json;
        if (!FFileHelper::LoadFileToString(Json, *FPaths::Combine(MissionDirectory, MissionFile)))
        {
            continue;
        }

        FWMMissionRuntimeDefinition Definition;
        FString Error;
        if (!FWMMissionRuntimeDefinition::TryParseJson(Json, Definition, Error) || Definition.MissionId.IsNone())
        {
            continue;
        }

        if (MissionCatalog.Contains(Definition.MissionId))
        {
            MissionCatalog.Reset();
            AvailableMissionIds.Reset();
            return false;
        }

        AvailableMissionIds.Add(Definition.MissionId);
        MissionCatalog.Add(Definition.MissionId, Definition);
    }

    AvailableMissionIds.Sort([](const FName& A, const FName& B)
    {
        return A.ToString() < B.ToString();
    });

    return !AvailableMissionIds.IsEmpty();
}

bool UWMMissionRuntimeSubsystem::ActivateMission(const FName MissionId)
{
    const FWMMissionRuntimeDefinition* Definition = MissionCatalog.Find(MissionId);
    if (!Definition)
    {
        return false;
    }

    Progress = FWMMissionProgressModel();
    LastMeasuredSpanCm = 0.0f;
    if (!Progress.Begin(*Definition))
    {
        return false;
    }

    if (ActiveGeometry.IsValid())
    {
        ActiveGeometry->ConfigureTargetSpanCm(Definition->TargetSpanCm);
    }
    return true;
}

bool UWMMissionRuntimeSubsystem::CycleMission(const int32 Direction)
{
    if (AvailableMissionIds.IsEmpty())
    {
        return false;
    }

    int32 Index = AvailableMissionIds.IndexOfByKey(GetActiveMissionId());
    if (Index == INDEX_NONE)
    {
        Index = 0;
    }
    else
    {
        const int32 Step = Direction >= 0 ? 1 : -1;
        Index = (Index + Step + AvailableMissionIds.Num()) % AvailableMissionIds.Num();
    }

    return ActivateMission(AvailableMissionIds[Index]);
}

bool UWMMissionRuntimeSubsystem::ReloadAndActivatePrototypeMission()
{
    if (!ReloadMissionCatalog())
    {
        Progress = FWMMissionProgressModel();
        LastMeasuredSpanCm = 0.0f;
        return false;
    }

    if (MissionCatalog.Contains(DefaultPrototypeMissionId))
    {
        return ActivateMission(DefaultPrototypeMissionId);
    }
    return ActivateMission(AvailableMissionIds[0]);
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
        if (Progress.State != EWMMissionRuntimeState::Inactive && Progress.Definition.TargetSpanCm > 0.0f)
        {
            GeometryActor->ConfigureTargetSpanCm(Progress.Definition.TargetSpanCm);
        }
    }
}

void UWMMissionRuntimeSubsystem::UnregisterMissionGeometry(AWMMissionGeometryActor* GeometryActor)
{
    if (ActiveGeometry.Get() == GeometryActor)
    {
        ActiveGeometry.Reset();
    }
}
