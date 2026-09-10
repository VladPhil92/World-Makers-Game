#include "Adventure/WMAdventureRuntimeSubsystem.h"

#include "Mission/WMMissionRuntimeSubsystem.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

void UWMAdventureRuntimeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    ReloadAdventurePack();
}

bool UWMAdventureRuntimeSubsystem::ReloadAdventurePack()
{
    bPackLoaded = false;
    Pack = FWMFantasticAdventurePack();
    Progress.Reset();

    const FString PackPath = FPaths::Combine(
        FPaths::ProjectContentDir(),
        TEXT("WorldMakers/Adventures/first-fantastic-adventure-pack-v1.json"));

    FString Json;
    if (!FFileHelper::LoadFileToString(Json, *PackPath))
    {
        return false;
    }

    FWMFantasticAdventurePack Candidate;
    FString Error;
    if (!FWMFantasticAdventurePack::TryParseJson(Json, Candidate, Error))
    {
        return false;
    }

    Pack = MoveTemp(Candidate);
    bPackLoaded = true;
    return true;
}

TArray<FName> UWMAdventureRuntimeSubsystem::GetAdventureIds() const
{
    TArray<FName> Result;
    Result.Reserve(Pack.Adventures.Num());
    for (const FWMFantasticAdventureDefinition& Adventure : Pack.Adventures)
    {
        Result.Add(Adventure.AdventureId);
    }
    Result.Sort([](const FName& A, const FName& B)
    {
        return A.ToString() < B.ToString();
    });
    return Result;
}

bool UWMAdventureRuntimeSubsystem::ActivateAdventure(const FName AdventureId)
{
    if (!bPackLoaded || !GetWorld())
    {
        return false;
    }

    const FWMFantasticAdventureDefinition* Adventure = Pack.FindAdventure(AdventureId);
    if (!Adventure)
    {
        return false;
    }

    UWMMissionRuntimeSubsystem* MissionSubsystem = GetWorld()->GetSubsystem<UWMMissionRuntimeSubsystem>();
    if (!MissionSubsystem || !MissionSubsystem->ActivateMission(Adventure->MissionId))
    {
        return false;
    }

    return Progress.Begin(*Adventure);
}

bool UWMAdventureRuntimeSubsystem::RecordAdventureEvidence(
    const FName ProducerKind,
    const FName ProducerRefId,
    const FName PrimitiveId,
    const FName EvidenceEventId,
    const float NumericValue)
{
    if (!GetWorld() || !Progress.CanAcceptEvidence(ProducerKind, ProducerRefId, PrimitiveId, EvidenceEventId))
    {
        return false;
    }

    UWMMissionRuntimeSubsystem* MissionSubsystem = GetWorld()->GetSubsystem<UWMMissionRuntimeSubsystem>();
    if (!MissionSubsystem || !MissionSubsystem->RecordComposableEvidence(PrimitiveId, EvidenceEventId, NumericValue))
    {
        return false;
    }

    return Progress.CommitEvidence(ProducerKind, ProducerRefId, PrimitiveId, EvidenceEventId);
}
