#include "Adventure/WMEclipseEngineExperienceSubsystem.h"

#include "Adventure/WMEclipseEngineInteractableActor.h"
#include "Adventure/WMEpicRuntimeSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Mission/WMMissionRuntimeSubsystem.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Player/WMPlayerCharacter.h"
#include "Visual/WMFirstPersonInteractionComponent.h"

void UWMEclipseEngineExperienceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    ReloadExperienceCatalog();
}

void UWMEclipseEngineExperienceSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);
    bool bAutoStart = false;
    if (GConfig)
    {
        GConfig->GetBool(TEXT("/Script/WorldMakers.WMGameMode"), TEXT("bStartEclipseEngineVerticalSlice"), bAutoStart, GGameIni);
    }
    if (bAutoStart)
    {
        StartEclipseEngine();
    }
}

bool UWMEclipseEngineExperienceSubsystem::ReloadExperienceCatalog()
{
    bCatalogLoaded = false;
    Catalog = FWMEclipseExperienceCatalog();
    HintRuntime.Reset();
    DestroyPrototypeTargets();

    const FString Path = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("WorldMakers/Epics/eclipse-engine-player-experience-v1.json"));
    FString Json;
    if (!FFileHelper::LoadFileToString(Json, *Path)) return false;

    FWMEclipseExperienceCatalog Candidate;
    FString Error;
    if (!FWMEclipseExperienceCatalog::TryParseJson(Json, Candidate, Error)) return false;
    Catalog = MoveTemp(Candidate);
    bCatalogLoaded = true;
    return true;
}

bool UWMEclipseEngineExperienceSubsystem::StartEclipseEngine()
{
    if (!bCatalogLoaded || !GetWorld()) return false;
    UWMEpicRuntimeSubsystem* Epic = GetWorld()->GetSubsystem<UWMEpicRuntimeSubsystem>();
    if (!Epic || !Epic->ActivateEpic(Catalog.EpicId)) return false;
    HintRuntime.Reset();
    return EnsurePrototypeTargets();
}

bool UWMEclipseEngineExperienceSubsystem::EnsurePrototypeTargets()
{
    if (!bCatalogLoaded || !GetWorld()) return false;
    DestroyPrototypeTargets();

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    for (const FWMEclipseChapterExperience& Chapter : Catalog.Chapters)
    {
        for (const FWMEclipseActionDefinition& Action : Chapter.Actions)
        {
            AWMEclipseEngineInteractableActor* Target = GetWorld()->SpawnActor<AWMEclipseEngineInteractableActor>(
                AWMEclipseEngineInteractableActor::StaticClass(), Action.PrototypeLocationCm, FRotator::ZeroRotator, SpawnParams);
            if (!Target) return false;
            Target->Configure(Chapter.ChapterId, Action);
            PrototypeTargets.Add(Target);
        }
    }
    RefreshPrototypeTargetAvailability();
    return !PrototypeTargets.IsEmpty();
}

void UWMEclipseEngineExperienceSubsystem::DestroyPrototypeTargets()
{
    for (AWMEclipseEngineInteractableActor* Target : PrototypeTargets)
    {
        if (IsValid(Target)) Target->Destroy();
    }
    PrototypeTargets.Reset();
}

void UWMEclipseEngineExperienceSubsystem::RefreshPrototypeTargetAvailability()
{
    FName CurrentChapterId = NAME_None;
    if (const FWMEclipseChapterExperience* Chapter = GetCurrentChapter()) CurrentChapterId = Chapter->ChapterId;
    for (AWMEclipseEngineInteractableActor* Target : PrototypeTargets)
    {
        if (IsValid(Target)) Target->SetAvailable(!CurrentChapterId.IsNone() && Target->ChapterId == CurrentChapterId);
    }
}

const FWMEclipseChapterExperience* UWMEclipseEngineExperienceSubsystem::GetCurrentChapter() const
{
    if (!bCatalogLoaded || !GetWorld()) return nullptr;
    const UWMEpicRuntimeSubsystem* Epic = GetWorld()->GetSubsystem<UWMEpicRuntimeSubsystem>();
    if (!Epic || Epic->GetActiveEpicId() != Catalog.EpicId) return nullptr;
    return Catalog.FindChapter(Epic->GetCurrentEpicChapterId());
}

const FWMEclipseActionDefinition* UWMEclipseEngineExperienceSubsystem::GetCurrentAction(const FName ActionId) const
{
    const FWMEclipseChapterExperience* Chapter = GetCurrentChapter();
    return Chapter ? Chapter->FindAction(ActionId) : nullptr;
}

bool UWMEclipseEngineExperienceSubsystem::CanBeginAction(const FName ActionId) const
{
    const FWMEclipseActionDefinition* Action = GetCurrentAction(ActionId);
    if (!Action || !GetWorld()) return false;
    if (!Action->bHasWorldStateRoute) return true;

    const UWMMissionRuntimeSubsystem* Mission = GetWorld()->GetSubsystem<UWMMissionRuntimeSubsystem>();
    return Mission && Mission->GetMissionState() == EWMMissionRuntimeState::Completed;
}

void UWMEclipseEngineExperienceSubsystem::PulseFirstPerson(const FWMEclipseActionDefinition& Action) const
{
    if (!GetWorld()) return;
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    AWMPlayerCharacter* Player = PC ? Cast<AWMPlayerCharacter>(PC->GetPawn()) : nullptr;
    if (!Player) return;

    UWMFirstPersonInteractionComponent* FirstPerson = Player->FirstPersonInteractionComponent;
    if (!FirstPerson)
    {
        FirstPerson = NewObject<UWMFirstPersonInteractionComponent>(Player, TEXT("FirstPersonInteractionComponent"));
        if (!FirstPerson) return;
        FirstPerson->RegisterComponent();
        Player->FirstPersonInteractionComponent = FirstPerson;
    }
    FirstPerson->PulseSemanticEvent(Action.FirstPersonEventId, Action.FirstPersonActionId, 0.95f);
}

bool UWMEclipseEngineExperienceSubsystem::BeginAction(const FName ActionId)
{
    const FWMEclipseActionDefinition* Action = GetCurrentAction(ActionId);
    if (!Action || !CanBeginAction(ActionId)) return false;
    HintRuntime.RecordAttempt(ActionId);
    PulseFirstPerson(*Action);
    return true;
}

bool UWMEclipseEngineExperienceSubsystem::ResolveTrustedAction(const FName ActionId, const float NumericValue)
{
    const FWMEclipseActionDefinition* Action = GetCurrentAction(ActionId);
    if (!Action || !GetWorld()) return false;

    UWMEpicRuntimeSubsystem* Epic = GetWorld()->GetSubsystem<UWMEpicRuntimeSubsystem>();
    if (!Epic) return false;

    bool bResolved = false;
    if (Action->bHasEvidenceRoute)
    {
        bResolved = Epic->RecordEpicEvidence(
            Action->Evidence.ObjectiveId,
            Action->Evidence.DisciplineId,
            Action->Evidence.ProducerKind,
            Action->Evidence.ProducerRefId,
            Action->Evidence.PrimitiveId,
            Action->Evidence.EvidenceEventId,
            NumericValue);
    }
    else if (Action->bHasWorldStateRoute)
    {
        UWMMissionRuntimeSubsystem* Mission = GetWorld()->GetSubsystem<UWMMissionRuntimeSubsystem>();
        if (!Mission || Mission->GetMissionState() != EWMMissionRuntimeState::Completed) return false;
        bResolved = Epic->RecordEpicWorldState(Action->WorldState.ProducerKind, Action->WorldState.ProducerRefId, Action->WorldState.WorldStateId);
    }

    if (!bResolved) return false;
    PulseFirstPerson(*Action);
    for (AWMEclipseEngineInteractableActor* Target : PrototypeTargets)
    {
        if (IsValid(Target) && Target->ActionId == ActionId) Target->MarkResolved();
    }
    RefreshPrototypeTargetAvailability();
    return true;
}

bool UWMEclipseEngineExperienceSubsystem::ResolveValidatedEvidence(
    const FName ProducerRefId,
    const FName PrimitiveId,
    const FName EvidenceEventId,
    const float NumericValue)
{
    const FWMEclipseChapterExperience* Chapter = GetCurrentChapter();
    if (!Chapter || ProducerRefId.IsNone() || PrimitiveId.IsNone() || EvidenceEventId.IsNone()) return false;
    const FWMEclipseActionDefinition* Match = Chapter->Actions.FindByPredicate(
        [ProducerRefId, PrimitiveId, EvidenceEventId](const FWMEclipseActionDefinition& Action)
        {
            return Action.bHasEvidenceRoute && Action.Evidence.ProducerRefId == ProducerRefId &&
                Action.Evidence.PrimitiveId == PrimitiveId && Action.Evidence.EvidenceEventId == EvidenceEventId;
        });
    return Match && ResolveTrustedAction(Match->ActionId, NumericValue);
}

FName UWMEclipseEngineExperienceSubsystem::RequestHint(const FName ActionId)
{
    const FWMEclipseActionDefinition* Action = GetCurrentAction(ActionId);
    return Action ? HintRuntime.RequestHint(*Action) : NAME_None;
}

FName UWMEclipseEngineExperienceSubsystem::GetCurrentFantasyGoalKey() const
{
    const FWMEclipseChapterExperience* Chapter = GetCurrentChapter();
    return Chapter ? Chapter->FantasyGoalKey : NAME_None;
}

FName UWMEclipseEngineExperienceSubsystem::GetCurrentTensionKey() const
{
    const FWMEclipseChapterExperience* Chapter = GetCurrentChapter();
    return Chapter ? Chapter->TensionKey : NAME_None;
}

TArray<FName> UWMEclipseEngineExperienceSubsystem::GetCurrentActionIds() const
{
    TArray<FName> Result;
    if (const FWMEclipseChapterExperience* Chapter = GetCurrentChapter())
    {
        for (const FWMEclipseActionDefinition& Action : Chapter->Actions) Result.Add(Action.ActionId);
    }
    return Result;
}

bool UWMEclipseEngineExperienceSubsystem::IsWorldStateAction(const FName ActionId) const
{
    const FWMEclipseActionDefinition* Action = GetCurrentAction(ActionId);
    return Action && Action->bHasWorldStateRoute;
}
