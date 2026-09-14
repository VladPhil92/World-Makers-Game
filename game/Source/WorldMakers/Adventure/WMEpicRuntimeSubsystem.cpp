#include "Adventure/WMEpicRuntimeSubsystem.h"

#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Launch/WMLaunchBootstrapSubsystem.h"
#include "Mission/WMMissionRuntimeSubsystem.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
    const FString EpicJourneySaveSlot(TEXT("WM_EpicJourney_v1"));
    constexpr int32 EpicJourneySaveUserIndex = 0;
}

void UWMEpicRuntimeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    ReloadEpicCatalog();
    LoadEpicCheckpoints();
}

bool UWMEpicRuntimeSubsystem::ReloadEpicCatalog()
{
    bCatalogLoaded = false;
    Catalog = FWMEpicCatalog();
    Progress.Reset();

    const FString CatalogPath = FPaths::Combine(
        FPaths::ProjectContentDir(), TEXT("WorldMakers/Epics/cross-disciplinary-epics-v1.json"));
    FString Json;
    if (!FFileHelper::LoadFileToString(Json, *CatalogPath)) return false;

    FWMEpicCatalog Candidate;
    FString Error;
    if (!FWMEpicCatalog::TryParseJson(Json, Candidate, Error)) return false;

    Catalog = MoveTemp(Candidate);
    bCatalogLoaded = true;
    return true;
}

TArray<FName> UWMEpicRuntimeSubsystem::GetEpicIds() const
{
    TArray<FName> Result;
    Result.Reserve(Catalog.Epics.Num());
    for (const FWMEpicDefinition& Epic : Catalog.Epics) Result.Add(Epic.EpicId);
    Result.Sort([](const FName& A, const FName& B) { return A.ToString() < B.ToString(); });
    return Result;
}

int32 UWMEpicRuntimeSubsystem::FindCheckpointIndex(const FName EpicId) const
{
    return Checkpoints.IndexOfByPredicate([EpicId](const FWMEpicCheckpoint& Item)
    {
        return Item.EpicId == EpicId;
    });
}

bool UWMEpicRuntimeSubsystem::IsCheckpointValidForCatalog(const FWMEpicCheckpoint& Checkpoint) const
{
    const FWMEpicDefinition* Epic = Catalog.FindEpic(Checkpoint.EpicId);
    if (!Epic || Checkpoint.ChapterCount != Epic->Chapters.Num()) return false;
    if (Checkpoint.bCompleted)
    {
        return Checkpoint.ChapterIndex == Epic->Chapters.Num() && Checkpoint.ChapterId.IsNone();
    }
    return Epic->Chapters.IsValidIndex(Checkpoint.ChapterIndex) &&
        Epic->Chapters[Checkpoint.ChapterIndex].ChapterId == Checkpoint.ChapterId;
}

bool UWMEpicRuntimeSubsystem::LoadEpicCheckpoints()
{
    Checkpoints.Reset();
    if (!bCatalogLoaded) return false;
    if (!UGameplayStatics::DoesSaveGameExist(EpicJourneySaveSlot, EpicJourneySaveUserIndex)) return true;

    UWMEpicJourneySaveGame* Save = Cast<UWMEpicJourneySaveGame>(
        UGameplayStatics::LoadGameFromSlot(EpicJourneySaveSlot, EpicJourneySaveUserIndex));
    if (!Save || Save->FormatVersion != UWMEpicJourneySaveGame::CurrentFormatVersion) return false;

    TSet<FName> SeenEpicIds;
    for (const FWMEpicCheckpoint& Candidate : Save->Checkpoints)
    {
        if (Candidate.EpicId.IsNone() || SeenEpicIds.Contains(Candidate.EpicId) || !IsCheckpointValidForCatalog(Candidate)) continue;
        SeenEpicIds.Add(Candidate.EpicId);
        Checkpoints.Add(Candidate);
    }
    return true;
}

bool UWMEpicRuntimeSubsystem::SaveEpicCheckpoints() const
{
    UWMEpicJourneySaveGame* Save = Cast<UWMEpicJourneySaveGame>(
        UGameplayStatics::CreateSaveGameObject(UWMEpicJourneySaveGame::StaticClass()));
    if (!Save) return false;
    Save->FormatVersion = UWMEpicJourneySaveGame::CurrentFormatVersion;
    Save->Checkpoints = Checkpoints;
    return UGameplayStatics::SaveGameToSlot(Save, EpicJourneySaveSlot, EpicJourneySaveUserIndex);
}

bool UWMEpicRuntimeSubsystem::SaveCurrentCheckpoint()
{
    const FWMEpicProgressReadModel ReadModel = Progress.BuildReadModel();
    if (ReadModel.EpicId.IsNone() || ReadModel.ChapterCount <= 0) return false;

    FWMEpicCheckpoint Checkpoint;
    Checkpoint.EpicId = ReadModel.EpicId;
    Checkpoint.ChapterId = ReadModel.bCompleted ? NAME_None : ReadModel.CurrentChapterId;
    Checkpoint.ChapterIndex = ReadModel.bCompleted ? ReadModel.ChapterCount : ReadModel.CurrentChapterIndex;
    Checkpoint.ChapterCount = ReadModel.ChapterCount;
    Checkpoint.bCompleted = ReadModel.bCompleted;
    if (!IsCheckpointValidForCatalog(Checkpoint)) return false;

    const TArray<FWMEpicCheckpoint> PreviousCheckpoints = Checkpoints;
    const int32 ExistingIndex = FindCheckpointIndex(Checkpoint.EpicId);
    if (ExistingIndex == INDEX_NONE) Checkpoints.Add(Checkpoint);
    else Checkpoints[ExistingIndex] = Checkpoint;

    if (!SaveEpicCheckpoints())
    {
        Checkpoints = PreviousCheckpoints;
        return false;
    }

    if (GetWorld())
    {
        if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
        {
            if (UWMLaunchBootstrapSubsystem* Launch = GameInstance->GetSubsystem<UWMLaunchBootstrapSubsystem>())
            {
                Launch->SyncEpicCheckpoint(Checkpoint);
            }
        }
    }
    return true;
}

bool UWMEpicRuntimeSubsystem::ActivateEpic(const FName EpicId)
{
    if (!bCatalogLoaded || !GetWorld()) return false;
    const FWMEpicDefinition* Epic = Catalog.FindEpic(EpicId);
    if (!Epic || Epic->Chapters.IsEmpty()) return false;

    UWMMissionRuntimeSubsystem* MissionSubsystem = GetWorld()->GetSubsystem<UWMMissionRuntimeSubsystem>();
    if (!MissionSubsystem || !MissionSubsystem->ActivateMission(Epic->Chapters[0].MissionId)) return false;
    if (!Progress.Begin(*Epic)) return false;
    return SaveCurrentCheckpoint();
}

bool UWMEpicRuntimeSubsystem::ApplyExternalResumeCheckpoint(
    const FName EpicId,
    const FName ChapterId,
    const int32 ChapterIndex,
    const int32 ChapterCount)
{
    if (!bCatalogLoaded || EpicId.IsNone() || ChapterId.IsNone()) return false;

    FWMEpicCheckpoint Incoming;
    Incoming.EpicId = EpicId;
    Incoming.ChapterId = ChapterId;
    Incoming.ChapterIndex = ChapterIndex;
    Incoming.ChapterCount = ChapterCount;
    Incoming.bCompleted = false;
    if (!IsCheckpointValidForCatalog(Incoming)) return false;

    const int32 ExistingIndex = FindCheckpointIndex(EpicId);
    if (Checkpoints.IsValidIndex(ExistingIndex))
    {
        const FWMEpicCheckpoint& Existing = Checkpoints[ExistingIndex];
        // Local completion or an equal/newer local chapter is never regressed by server bootstrap state.
        if (Existing.bCompleted || Existing.ChapterIndex >= Incoming.ChapterIndex) return true;
    }

    const TArray<FWMEpicCheckpoint> PreviousCheckpoints = Checkpoints;
    if (ExistingIndex == INDEX_NONE) Checkpoints.Add(Incoming);
    else Checkpoints[ExistingIndex] = Incoming;
    if (SaveEpicCheckpoints()) return true;
    Checkpoints = PreviousCheckpoints;
    return false;
}

bool UWMEpicRuntimeSubsystem::HasResumableEpicCheckpoint(const FName EpicId) const
{
    const int32 Index = FindCheckpointIndex(EpicId);
    return Checkpoints.IsValidIndex(Index) && !Checkpoints[Index].bCompleted && IsCheckpointValidForCatalog(Checkpoints[Index]);
}

FName UWMEpicRuntimeSubsystem::GetEpicCheckpointChapterId(const FName EpicId) const
{
    const int32 Index = FindCheckpointIndex(EpicId);
    return Checkpoints.IsValidIndex(Index) && IsCheckpointValidForCatalog(Checkpoints[Index]) ? Checkpoints[Index].ChapterId : NAME_None;
}

bool UWMEpicRuntimeSubsystem::ResumeEpic(const FName EpicId)
{
    if (!bCatalogLoaded || !GetWorld() || !HasResumableEpicCheckpoint(EpicId)) return false;
    const int32 CheckpointIndex = FindCheckpointIndex(EpicId);
    const FWMEpicCheckpoint Checkpoint = Checkpoints[CheckpointIndex];
    const FWMEpicDefinition* Epic = Catalog.FindEpic(EpicId);
    if (!Epic || !Epic->Chapters.IsValidIndex(Checkpoint.ChapterIndex)) return false;

    UWMMissionRuntimeSubsystem* MissionSubsystem = GetWorld()->GetSubsystem<UWMMissionRuntimeSubsystem>();
    if (!MissionSubsystem || !MissionSubsystem->ActivateMission(Epic->Chapters[Checkpoint.ChapterIndex].MissionId)) return false;
    if (!Progress.ResumeAtChapter(*Epic, Checkpoint.ChapterIndex)) return false;

    // No partial evidence is restored. The checkpoint itself is unchanged and remains chapter-granular.
    return true;
}

bool UWMEpicRuntimeSubsystem::ActivateOrResumeEpic(const FName EpicId)
{
    return HasResumableEpicCheckpoint(EpicId) ? ResumeEpic(EpicId) : ActivateEpic(EpicId);
}

bool UWMEpicRuntimeSubsystem::ClearEpicCheckpoint(const FName EpicId)
{
    const int32 Index = FindCheckpointIndex(EpicId);
    if (Index == INDEX_NONE) return true;
    const TArray<FWMEpicCheckpoint> PreviousCheckpoints = Checkpoints;
    Checkpoints.RemoveAt(Index);
    if (SaveEpicCheckpoints()) return true;
    Checkpoints = PreviousCheckpoints;
    return false;
}

bool UWMEpicRuntimeSubsystem::RecordEpicEvidence(
    const FName ObjectiveId,
    const FName DisciplineId,
    const FName ProducerKind,
    const FName ProducerRefId,
    const FName PrimitiveId,
    const FName EvidenceEventId,
    const float NumericValue)
{
    if (!GetWorld() || !Progress.CanAcceptEvidence(
        ObjectiveId, DisciplineId, ProducerKind, ProducerRefId, PrimitiveId, EvidenceEventId)) return false;

    UWMMissionRuntimeSubsystem* MissionSubsystem = GetWorld()->GetSubsystem<UWMMissionRuntimeSubsystem>();
    if (!MissionSubsystem || !MissionSubsystem->RecordComposableEvidence(PrimitiveId, EvidenceEventId, NumericValue)) return false;
    if (!Progress.CommitEvidence(ObjectiveId, DisciplineId, ProducerKind, ProducerRefId, PrimitiveId, EvidenceEventId)) return false;
    return AdvanceEpicIfReady();
}

bool UWMEpicRuntimeSubsystem::RecordEpicWorldState(
    const FName ProducerKind, const FName ProducerRefId, const FName WorldStateId)
{
    if (!Progress.CommitWorldState(ProducerKind, ProducerRefId, WorldStateId)) return false;
    return AdvanceEpicIfReady();
}

bool UWMEpicRuntimeSubsystem::AdvanceEpicIfReady()
{
    if (!Progress.IsCurrentChapterReadyToAdvance()) return true;
    const FWMEpicChapterDefinition* NextChapter = Progress.GetNextChapter();
    if (NextChapter)
    {
        if (!GetWorld()) return false;
        UWMMissionRuntimeSubsystem* MissionSubsystem = GetWorld()->GetSubsystem<UWMMissionRuntimeSubsystem>();
        if (!MissionSubsystem || !MissionSubsystem->ActivateMission(NextChapter->MissionId)) return false;
    }
    if (!Progress.AdvanceChapter()) return false;
    return SaveCurrentCheckpoint();
}
