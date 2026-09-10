#include "Mission/WMMissionRuntimeSubsystem.h"

#include "HAL/FileManager.h"
#include "Kismet/GameplayStatics.h"
#include "Mission/WMMissionGeometryActor.h"
#include "Mission/WMMissionJourneySaveGame.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
    const FName DefaultPrototypeMissionId(TEXT("mission.mathematics.measure-and-build-01"));
    const FString JourneySaveSlot(TEXT("WM_MissionJourney_Prototype"));
    constexpr int32 JourneySaveUserIndex = 0;
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

    if (AvailableMissionIds.IsEmpty() || !ValidateCatalogDependencies())
    {
        MissionCatalog.Reset();
        AvailableMissionIds.Reset();
        return false;
    }
    return true;
}

bool UWMMissionRuntimeSubsystem::ValidateCatalogDependencies() const
{
    TMap<FName, uint8> VisitState;
    TFunction<bool(FName)> Visit = [&](const FName MissionId)
    {
        const uint8 ExistingState = VisitState.FindRef(MissionId);
        if (ExistingState == 1) return false;
        if (ExistingState == 2) return true;

        const FWMMissionRuntimeDefinition* Definition = MissionCatalog.Find(MissionId);
        if (!Definition) return false;

        VisitState.Add(MissionId, 1);
        for (const FName PrerequisiteId : Definition->PrerequisiteMissionIds)
        {
            if (!MissionCatalog.Contains(PrerequisiteId) || PrerequisiteId == MissionId || !Visit(PrerequisiteId))
            {
                return false;
            }
        }
        VisitState.Add(MissionId, 2);
        return true;
    };

    for (const FName MissionId : AvailableMissionIds)
    {
        if (!Visit(MissionId)) return false;
    }
    return true;
}

bool UWMMissionRuntimeSubsystem::ActivateMission(const FName MissionId)
{
    const FWMMissionRuntimeDefinition* Definition = MissionCatalog.Find(MissionId);
    if (!Definition || !Journey.CanActivate(*Definition))
    {
        return false;
    }

    Progress = FWMMissionProgressModel();
    LastMeasuredSpanCm = 0.0f;
    if (!Progress.Begin(*Definition))
    {
        return false;
    }

    LastSavedActiveMissionId = MissionId;
    if (ActiveGeometry.IsValid())
    {
        if (Definition->IsMeasureAndBuild())
        {
            ActiveGeometry->ConfigureTargetSpanCm(Definition->TargetSpanCm);
        }
        ActiveGeometry->ClearInteractiveMeasurement();
    }
    SaveJourneyProgress();
    return true;
}

TArray<FName> UWMMissionRuntimeSubsystem::GetActivatableMissionIds() const
{
    TArray<FName> Result;
    for (const FName MissionId : AvailableMissionIds)
    {
        const FWMMissionRuntimeDefinition* Definition = MissionCatalog.Find(MissionId);
        if (Definition && Journey.CanActivate(*Definition)) Result.Add(MissionId);
    }
    return Result;
}

bool UWMMissionRuntimeSubsystem::CycleMission(const int32 Direction)
{
    const TArray<FName> ActivatableMissionIds = GetActivatableMissionIds();
    if (ActivatableMissionIds.IsEmpty())
    {
        return false;
    }
    if (ActivatableMissionIds.Num() == 1 && ActivatableMissionIds[0] == GetActiveMissionId())
    {
        return false;
    }

    int32 Index = ActivatableMissionIds.IndexOfByKey(GetActiveMissionId());
    if (Index == INDEX_NONE)
    {
        Index = 0;
    }
    else
    {
        const int32 Step = Direction >= 0 ? 1 : -1;
        Index = (Index + Step + ActivatableMissionIds.Num()) % ActivatableMissionIds.Num();
    }

    return ActivateMission(ActivatableMissionIds[Index]);
}

bool UWMMissionRuntimeSubsystem::ActivateBestStartupMission()
{
    if (!LastSavedActiveMissionId.IsNone() && !Journey.CompletedMissionIds.Contains(LastSavedActiveMissionId))
    {
        const FWMMissionRuntimeDefinition* SavedDefinition = MissionCatalog.Find(LastSavedActiveMissionId);
        if (SavedDefinition && Journey.CanActivate(*SavedDefinition) && ActivateMission(LastSavedActiveMissionId))
        {
            return true;
        }
    }

    for (const FName MissionId : AvailableMissionIds)
    {
        const FWMMissionRuntimeDefinition* Definition = MissionCatalog.Find(MissionId);
        if (Definition && !Journey.CompletedMissionIds.Contains(MissionId) && Journey.CanActivate(*Definition))
        {
            return ActivateMission(MissionId);
        }
    }

    if (MissionCatalog.Contains(DefaultPrototypeMissionId))
    {
        const FWMMissionRuntimeDefinition& DefaultDefinition = MissionCatalog.FindChecked(DefaultPrototypeMissionId);
        if (Journey.CanActivate(DefaultDefinition)) return ActivateMission(DefaultPrototypeMissionId);
    }

    const TArray<FName> ActivatableMissionIds = GetActivatableMissionIds();
    return !ActivatableMissionIds.IsEmpty() && ActivateMission(ActivatableMissionIds[0]);
}

bool UWMMissionRuntimeSubsystem::ReloadAndActivatePrototypeMission()
{
    if (!ReloadMissionCatalog())
    {
        Progress = FWMMissionProgressModel();
        LastMeasuredSpanCm = 0.0f;
        return false;
    }

    LoadJourneyProgress();
    return ActivateBestStartupMission();
}

bool UWMMissionRuntimeSubsystem::LoadJourneyProgress()
{
    Journey.Restore({}, {});
    LastSavedActiveMissionId = NAME_None;

    if (!UGameplayStatics::DoesSaveGameExist(JourneySaveSlot, JourneySaveUserIndex))
    {
        return true;
    }

    UWMMissionJourneySaveGame* Save = Cast<UWMMissionJourneySaveGame>(UGameplayStatics::LoadGameFromSlot(JourneySaveSlot, JourneySaveUserIndex));
    if (!Save || Save->FormatVersion != UWMMissionJourneySaveGame::CurrentFormatVersion)
    {
        return false;
    }

    TSet<FName> CandidateCompleted;
    for (const FName MissionId : Save->CompletedMissionIds)
    {
        if (MissionCatalog.Contains(MissionId)) CandidateCompleted.Add(MissionId);
    }

    TArray<FName> SanitizedCompleted;
    bool bAddedMission = true;
    while (bAddedMission)
    {
        bAddedMission = false;
        for (const FName MissionId : AvailableMissionIds)
        {
            if (!CandidateCompleted.Contains(MissionId) || SanitizedCompleted.Contains(MissionId)) continue;
            const FWMMissionRuntimeDefinition& Definition = MissionCatalog.FindChecked(MissionId);
            bool bPrerequisitesPresent = true;
            for (const FName PrerequisiteId : Definition.PrerequisiteMissionIds)
            {
                if (!SanitizedCompleted.Contains(PrerequisiteId))
                {
                    bPrerequisitesPresent = false;
                    break;
                }
            }
            if (bPrerequisitesPresent)
            {
                SanitizedCompleted.Add(MissionId);
                bAddedMission = true;
            }
        }
    }

    Journey.Restore(SanitizedCompleted, Save->GrantedRewardIds);
    LastSavedActiveMissionId = MissionCatalog.Contains(Save->LastActiveMissionId) ? Save->LastActiveMissionId : NAME_None;
    return true;
}

bool UWMMissionRuntimeSubsystem::SaveJourneyProgress() const
{
    UWMMissionJourneySaveGame* Save = Cast<UWMMissionJourneySaveGame>(UGameplayStatics::CreateSaveGameObject(UWMMissionJourneySaveGame::StaticClass()));
    if (!Save) return false;

    Save->FormatVersion = UWMMissionJourneySaveGame::CurrentFormatVersion;
    Journey.Export(Save->CompletedMissionIds, Save->GrantedRewardIds);
    Save->LastActiveMissionId = GetActiveMissionId().IsNone() ? LastSavedActiveMissionId : GetActiveMissionId();
    return UGameplayStatics::SaveGameToSlot(Save, JourneySaveSlot, JourneySaveUserIndex);
}

EWMJourneyMissionState UWMMissionRuntimeSubsystem::GetJourneyMissionState(const FName MissionId) const
{
    const FWMMissionRuntimeDefinition* Definition = MissionCatalog.Find(MissionId);
    if (!Definition) return EWMJourneyMissionState::Locked;
    return Journey.ResolveState(*Definition, GetActiveMissionId(), Progress.State);
}

TArray<FWMJourneyMissionReadModel> UWMMissionRuntimeSubsystem::GetJourneyReadModel() const
{
    TArray<FWMJourneyMissionReadModel> Result;
    Result.Reserve(AvailableMissionIds.Num());
    for (const FName MissionId : AvailableMissionIds)
    {
        const FWMMissionRuntimeDefinition* Definition = MissionCatalog.Find(MissionId);
        if (!Definition) continue;

        FWMJourneyMissionReadModel Entry;
        Entry.MissionId = MissionId;
        Entry.State = Journey.ResolveState(*Definition, GetActiveMissionId(), Progress.State);
        Entry.ProgressFraction = Entry.State == EWMJourneyMissionState::Completed
            ? 1.0f
            : (Entry.State == EWMJourneyMissionState::Active ? Progress.GetProgressFraction() : 0.0f);
        Entry.LearningObjectiveIds = Definition->LearningObjectiveIds;
        Entry.bPrototypeOnly = Definition->bPrototypeOnly;
        Result.Add(MoveTemp(Entry));
    }
    return Result;
}

TArray<FName> UWMMissionRuntimeSubsystem::GetGrantedRewardIds() const
{
    TArray<FName> CompletedIds;
    TArray<FName> RewardIds;
    Journey.Export(CompletedIds, RewardIds);
    return RewardIds;
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
    if (!ActiveGeometry.IsValid() || Progress.State != EWMMissionRuntimeState::Active || !Progress.Definition.IsMeasureAndBuild())
    {
        return false;
    }
    return RecordMeasurement(ActiveGeometry->GetMeasuredSpanCm());
}

bool UWMMissionRuntimeSubsystem::RecordStructureSpan(const float StructureSpanCm)
{
    const bool bRecorded = Progress.RecordStructureSpan(StructureSpanCm);
    if (bRecorded && Progress.State == EWMMissionRuntimeState::Completed)
    {
        FinalizeMissionCompletion();
    }
    return bRecorded;
}

bool UWMMissionRuntimeSubsystem::IsObservationRequired(const FName ObservationId) const
{
    return Progress.State == EWMMissionRuntimeState::Active && Progress.Definition.IsObserveEcosystem() &&
        Progress.Definition.FindObservationRequirement(ObservationId) != nullptr;
}

TArray<FName> UWMMissionRuntimeSubsystem::GetRequiredObservationIds() const
{
    TArray<FName> Result;
    if (!Progress.Definition.IsObserveEcosystem()) return Result;
    Result.Reserve(Progress.Definition.ObservationRequirements.Num());
    for (const FWMObservationEvidenceRequirement& Requirement : Progress.Definition.ObservationRequirements)
    {
        Result.Add(Requirement.ObservationId);
    }
    Result.Sort([](const FName& A, const FName& B)
    {
        return A.ToString() < B.ToString();
    });
    return Result;
}

bool UWMMissionRuntimeSubsystem::RecordObservationEvidence(const FName ObservationId)
{
    const bool bRecorded = Progress.RecordObservation(ObservationId);
    if (bRecorded && Progress.State == EWMMissionRuntimeState::Completed)
    {
        FinalizeMissionCompletion();
    }
    return bRecorded;
}

bool UWMMissionRuntimeSubsystem::IsComposableEvidenceRequired(const FName PrimitiveId, const FName EvidenceEventId) const
{
    return Progress.State == EWMMissionRuntimeState::Active && Progress.Definition.IsComposable() &&
        Progress.Definition.FindComposableRequirement(PrimitiveId, EvidenceEventId) != nullptr;
}

TArray<FName> UWMMissionRuntimeSubsystem::GetRequiredEvidencePrimitiveIds() const
{
    TSet<FName> UniquePrimitiveIds;
    if (Progress.Definition.IsComposable())
    {
        for (const FWMComposableEvidenceRequirement& Requirement : Progress.Definition.ComposableRequirements)
        {
            UniquePrimitiveIds.Add(Requirement.PrimitiveId);
        }
    }

    TArray<FName> Result = UniquePrimitiveIds.Array();
    Result.Sort([](const FName& A, const FName& B)
    {
        return A.ToString() < B.ToString();
    });
    return Result;
}

bool UWMMissionRuntimeSubsystem::RecordComposableEvidence(
    const FName PrimitiveId,
    const FName EvidenceEventId,
    const float NumericValue)
{
    const bool bRecorded = Progress.RecordComposableEvidence(PrimitiveId, EvidenceEventId, NumericValue);
    if (bRecorded && Progress.State == EWMMissionRuntimeState::Completed)
    {
        FinalizeMissionCompletion();
    }
    return bRecorded;
}

void UWMMissionRuntimeSubsystem::FinalizeMissionCompletion()
{
    TArray<FName> NewRewardIds;
    Journey.ApplyCompletion(Progress.Definition, NewRewardIds);
    Progress.EarnedRewardIds = MoveTemp(NewRewardIds);
    SaveJourneyProgress();
}

void UWMMissionRuntimeSubsystem::RegisterMissionGeometry(AWMMissionGeometryActor* GeometryActor)
{
    if (IsValid(GeometryActor))
    {
        ActiveGeometry = GeometryActor;
        if (Progress.State != EWMMissionRuntimeState::Inactive && Progress.Definition.IsMeasureAndBuild() && Progress.Definition.TargetSpanCm > 0.0f)
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
