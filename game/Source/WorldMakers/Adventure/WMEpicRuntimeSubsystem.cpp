#include "Adventure/WMEpicRuntimeSubsystem.h"

#include "Mission/WMMissionRuntimeSubsystem.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

void UWMEpicRuntimeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    ReloadEpicCatalog();
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

bool UWMEpicRuntimeSubsystem::ActivateEpic(const FName EpicId)
{
    if (!bCatalogLoaded || !GetWorld()) return false;
    const FWMEpicDefinition* Epic = Catalog.FindEpic(EpicId);
    if (!Epic || Epic->Chapters.IsEmpty()) return false;

    UWMMissionRuntimeSubsystem* MissionSubsystem = GetWorld()->GetSubsystem<UWMMissionRuntimeSubsystem>();
    if (!MissionSubsystem || !MissionSubsystem->ActivateMission(Epic->Chapters[0].MissionId)) return false;
    if (!Progress.Begin(*Epic)) return false;
    return true;
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
    return Progress.AdvanceChapter();
}
