#include "Adventure/WMEclipseEngineExperienceSubsystem.h"

#include "Adventure/WMEclipseEngineInteractableActor.h"
#include "Adventure/WMEclipseOpticsRuntime.h"
#include "Adventure/WMEclipseSystemsRuntime.h"
#include "Adventure/WMEpicRuntimeSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Launch/WMLaunchBootstrapSubsystem.h"
#include "Mission/WMMissionRuntimeSubsystem.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Player/WMPlayerCharacter.h"
#include "Thought/WMLanguageThoughtSubsystem.h"
#include "Visual/WMFirstPersonInteractionComponent.h"

void UWMEclipseEngineExperienceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    ReloadExperienceCatalog();
}

void UWMEclipseEngineExperienceSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);
    if (UGameInstance* GameInstance = InWorld.GetGameInstance())
    {
        if (UWMLaunchBootstrapSubsystem* Launch = GameInstance->GetSubsystem<UWMLaunchBootstrapSubsystem>(); Launch && Launch->IsNativeLaunchRequested())
        {
            if (Launch->IsNativeLaunchReady()) Launch->TryApplyEpicResume();
            // Native v2 launches are fail-closed: config-driven auto-start must never race ticket redemption.
            return;
        }
    }

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
    if (!Epic || !Epic->ActivateOrResumeEpic(Catalog.EpicId)) return false;
    if (UWMEclipseSystemsSubsystem* Systems = GetWorld()->GetSubsystem<UWMEclipseSystemsSubsystem>())
    {
        Systems->ResetPrototypeState();
    }
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

bool UWMEclipseEngineExperienceSubsystem::AdvancePrototypeMechanic(const FName ActionId, const int32 InteractionStep)
{
    if (!GetWorld() || InteractionStep < 1 || InteractionStep > 8) return false;
    const FWMEclipseActionDefinition* Action = GetCurrentAction(ActionId);
    if (!Action || !Action->bHasEvidenceRoute) return false;

    UWMEclipseSystemsSubsystem* Systems = GetWorld()->GetSubsystem<UWMEclipseSystemsSubsystem>();
    UWMEclipseOpticsSubsystem* Optics = GetWorld()->GetSubsystem<UWMEclipseOpticsSubsystem>();
    UWMLanguageThoughtSubsystem* Thought = GetWorld()->GetSubsystem<UWMLanguageThoughtSubsystem>();

    if (ActionId == TEXT("eclipse.read-orbit-rhythm"))
    {
        if (!Systems) return false;
        const int32 Candidate = InteractionStep == 1 ? 13 : 14;
        return Systems->SubmitOrbitPattern({2, 5, 8, 11}, Candidate);
    }
    if (ActionId == TEXT("eclipse.balance-orbit-ratio"))
    {
        if (!Systems) return false;
        return InteractionStep == 1 ? Systems->SubmitOrbitRatio(2, 3, 4, 5) : Systems->SubmitOrbitRatio(2, 3, 4, 6);
    }
    if (ActionId == TEXT("eclipse.build-orbit-route"))
    {
        if (!Systems) return false;
        const float CandidateLength = InteractionStep == 1 ? 840.0f : 760.0f;
        return Systems->SubmitOptimizedRoute(CandidateLength, {900.0f, 820.0f});
    }
    if (ActionId == TEXT("eclipse.restore-mirror-symmetry"))
    {
        if (!Optics) return false;
        return InteractionStep == 1 ? Optics->SubmitMirrorSymmetry(30.0f, 20.0f) : Optics->SubmitMirrorSymmetry(30.0f, -30.0f);
    }
    if (ActionId == TEXT("eclipse-angle-light-bridge"))
    {
        if (!Optics) return false;
        return InteractionStep == 1 ? Optics->SubmitReflectionBridge(35.0f, 45.0f, 8.0f) : Optics->SubmitReflectionBridge(35.0f, 35.0f, 1.0f);
    }
    if (ActionId == TEXT("eclipse-prove-light-path"))
    {
        if (!Optics) return false;
        return InteractionStep == 1 ? Optics->SubmitSpatialStability({-1.0f, 0.5f, 6.0f}) : Optics->SubmitSpatialStability({-1.2f, 0.6f, 1.5f, -0.4f});
    }
    if (ActionId == TEXT("eclipse-test-counterweight"))
    {
        if (!Systems) return false;
        const FVector CandidateVelocity = InteractionStep == 1 ? FVector(1.0f, 0.0f, 0.0f) : FVector(2.0f, 0.0f, 0.0f);
        return Systems->SubmitForcePrediction(2.0f, FVector(4.0f, 0.0f, 0.0f), 1.0f, CandidateVelocity, FVector(2.0f, 0.0f, 0.0f));
    }
    if (ActionId == TEXT("eclipse-route-core-current"))
    {
        if (!Systems) return false;
        return InteractionStep == 1 ? Systems->SubmitCircuitModel(12.0f, 6.0f, 1.0f, 12.0f) : Systems->SubmitCircuitModel(12.0f, 6.0f, 2.0f, 24.0f);
    }
    if (ActionId == TEXT("eclipse-restore-command"))
    {
        if (!Thought) return false;
        const FName Choice = InteractionStep == 1 ? TEXT("choice.es.dragon-di-frase") : TEXT("choice.en.dragon-please-speak-phrase");
        return Thought->EvaluateCommunicationAndRecord(TEXT("language.en.dragon-restore-command"), Choice);
    }
    if (ActionId == TEXT("eclipse-decode-context-fragment"))
    {
        if (!Systems) return false;
        if (InteractionStep == 1)
        {
            Systems->ObserveContextClue(TEXT("clue.eclipse.archive-neighbor-symbol"));
            return false;
        }
        if (InteractionStep == 2)
        {
            Systems->ObserveContextClue(TEXT("clue.eclipse.archive-response-pattern"));
            return false;
        }
        const FName Candidate = InteractionStep == 3 ? TEXT("meaning.eclipse.archive-needs-more-power") : TEXT("meaning.eclipse.archive-linked-symbols");
        return Systems->SubmitContextInference(Candidate);
    }
    if (ActionId == TEXT("eclipse-follow-story-thread"))
    {
        if (!Thought || InteractionStep < 2) return false;
        FName NextNode = NAME_None;
        return Thought->TraverseNarrativeAndRecord(
            TEXT("story.labyrinth-minotaur-prototype"), TEXT("node.labyrinth.gate"), TEXT("choice.follow-thread-clue"), NextNode);
    }
    if (ActionId == TEXT("eclipse-read-symbolic-perspective"))
    {
        if (!Thought || InteractionStep < 2) return false;
        FName NextNode = NAME_None;
        return Thought->TraverseNarrativeAndRecord(
            TEXT("story.labyrinth-minotaur-prototype"), TEXT("node.labyrinth.thread"), TEXT("choice.interpret-thread-purpose"), NextNode);
    }
    if (ActionId == TEXT("eclipse-revise-causal-model"))
    {
        if (!Thought) return false;
        const int32 RevisionCount = InteractionStep >= 2 ? 1 : 0;
        return Thought->EvaluatePhilosophicalArgumentAndRecord(
            TEXT("philosophy.ship-theseus-prototype"),
            TEXT("claim.same-ship-despite-replacement"),
            {FName(TEXT("link.function-continuity-supports-same"))},
            {FName(TEXT("assumption.identity-can-persist-through-change"))},
            {FName(TEXT("counterexample.reassembled-original-parts"))},
            RevisionCount);
    }

    return false;
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
