#include "Adventure/WMGardenEndWinterExperienceSubsystem.h"

#include "Adventure/WMGardenEndWinterInteractableActor.h"
#include "Adventure/WMGardenSystemsRuntime.h"
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
#include "Thought/WMLanguageThoughtRuntime.h"
#include "Thought/WMLanguageThoughtSubsystem.h"
#include "Visual/WMFirstPersonInteractionComponent.h"

void UWMGardenEndWinterExperienceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    ReloadExperienceCatalog();
}

void UWMGardenEndWinterExperienceSubsystem::OnWorldBeginPlay(UWorld& InWorld)
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
        GConfig->GetBool(TEXT("/Script/WorldMakers.WMGameMode"), TEXT("bStartGardenEndWinterVerticalSlice"), bAutoStart, GGameIni);
    }
    if (bAutoStart) StartGardenEndWinter();
}

bool UWMGardenEndWinterExperienceSubsystem::ReloadExperienceCatalog()
{
    bCatalogLoaded = false;
    Catalog = FWMGardenExperienceCatalog();
    HintRuntime.Reset();
    SatisfiedMasteryGates.Reset();
    DestroyPrototypeTargets();

    const FString Path = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("WorldMakers/Epics/garden-end-winter-player-experience-v1.json"));
    FString Json;
    if (!FFileHelper::LoadFileToString(Json, *Path)) return false;
    FWMGardenExperienceCatalog Candidate; FString Error;
    if (!FWMGardenExperienceCatalog::TryParseJson(Json, Candidate, Error)) return false;
    Catalog = MoveTemp(Candidate);
    bCatalogLoaded = true;
    return true;
}

bool UWMGardenEndWinterExperienceSubsystem::StartGardenEndWinter()
{
    if (!bCatalogLoaded || !GetWorld()) return false;
    UWMEpicRuntimeSubsystem* Epic = GetWorld()->GetSubsystem<UWMEpicRuntimeSubsystem>();
    if (!Epic || !Epic->ActivateOrResumeEpic(Catalog.EpicId)) return false;
    HintRuntime.Reset();
    SatisfiedMasteryGates.Reset();
    return EnsurePrototypeTargets();
}

bool UWMGardenEndWinterExperienceSubsystem::EnsurePrototypeTargets()
{
    if (!bCatalogLoaded || !GetWorld()) return false;
    DestroyPrototypeTargets();
    FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    for (const FWMGardenChapterExperience& Chapter : Catalog.Chapters)
    {
        for (const FWMGardenActionDefinition& Action : Chapter.Actions)
        {
            AWMGardenEndWinterInteractableActor* Target = GetWorld()->SpawnActor<AWMGardenEndWinterInteractableActor>(
                AWMGardenEndWinterInteractableActor::StaticClass(), Action.PrototypeLocationCm, FRotator::ZeroRotator, Params);
            if (!Target) return false;
            Target->Configure(Chapter.ChapterId, Action);
            PrototypeTargets.Add(Target);
        }
    }
    RefreshPrototypeTargetAvailability();
    return !PrototypeTargets.IsEmpty();
}

void UWMGardenEndWinterExperienceSubsystem::DestroyPrototypeTargets()
{
    for (AWMGardenEndWinterInteractableActor* Target : PrototypeTargets) if (IsValid(Target)) Target->Destroy();
    PrototypeTargets.Reset();
}

const FWMGardenChapterExperience* UWMGardenEndWinterExperienceSubsystem::GetCurrentChapter() const
{
    if (!bCatalogLoaded || !GetWorld()) return nullptr;
    const UWMEpicRuntimeSubsystem* Epic = GetWorld()->GetSubsystem<UWMEpicRuntimeSubsystem>();
    if (!Epic || Epic->GetActiveEpicId() != Catalog.EpicId) return nullptr;
    return Catalog.FindChapter(Epic->GetCurrentEpicChapterId());
}

const FWMGardenActionDefinition* UWMGardenEndWinterExperienceSubsystem::GetCurrentAction(const FName ActionId) const
{
    const FWMGardenChapterExperience* Chapter = GetCurrentChapter();
    return Chapter ? Chapter->FindAction(ActionId) : nullptr;
}

bool UWMGardenEndWinterExperienceSubsystem::AreCurrentMasteryGatesSatisfied() const
{
    const FWMGardenChapterExperience* Chapter = GetCurrentChapter();
    if (!Chapter) return false;
    for (const FName GateId : Chapter->GetRequiredMasteryGateIds()) if (!SatisfiedMasteryGates.Contains(GateId)) return false;
    return true;
}

bool UWMGardenEndWinterExperienceSubsystem::CanBeginAction(const FName ActionId) const
{
    const FWMGardenActionDefinition* Action = GetCurrentAction(ActionId);
    if (!Action || !GetWorld()) return false;
    if (!Action->bHasWorldStateRoute) return true;
    const UWMMissionRuntimeSubsystem* Mission = GetWorld()->GetSubsystem<UWMMissionRuntimeSubsystem>();
    return Mission && Mission->GetMissionState() == EWMMissionRuntimeState::Completed && AreCurrentMasteryGatesSatisfied();
}

void UWMGardenEndWinterExperienceSubsystem::PulseFirstPerson(const FWMGardenActionDefinition& Action) const
{
    if (!GetWorld()) return;
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    AWMPlayerCharacter* Player = PC ? Cast<AWMPlayerCharacter>(PC->GetPawn()) : nullptr;
    if (!Player) return;
    UWMFirstPersonInteractionComponent* FirstPerson = Player->FirstPersonInteractionComponent;
    if (!FirstPerson)
    {
        FirstPerson = NewObject<UWMFirstPersonInteractionComponent>(Player, TEXT("GardenFirstPersonInteractionComponent"));
        if (!FirstPerson) return;
        FirstPerson->RegisterComponent();
        Player->FirstPersonInteractionComponent = FirstPerson;
    }
    FirstPerson->PulseSemanticEvent(Action.FirstPersonEventId, Action.FirstPersonActionId, 0.95f);
}

bool UWMGardenEndWinterExperienceSubsystem::BeginAction(const FName ActionId)
{
    const FWMGardenActionDefinition* Action = GetCurrentAction(ActionId);
    if (!Action || !CanBeginAction(ActionId)) return false;
    HintRuntime.RecordAttempt(ActionId);
    PulseFirstPerson(*Action);
    return true;
}

bool UWMGardenEndWinterExperienceSubsystem::AdvancePrototypeMechanic(const FName ActionId, const int32 InteractionStep)
{
    if (!GetWorld() || InteractionStep < 1 || InteractionStep > 8) return false;
    const FWMGardenActionDefinition* Action = GetCurrentAction(ActionId);
    if (!Action || Action->bHasWorldStateRoute) return false;
    UWMGardenSystemsSubsystem* Systems = GetWorld()->GetSubsystem<UWMGardenSystemsSubsystem>();
    UWMLanguageThoughtSubsystem* Thought = GetWorld()->GetSubsystem<UWMLanguageThoughtSubsystem>();

    if (ActionId == TEXT("garden.balance-cell-energy"))
        return Systems && (InteractionStep == 1 ? Systems->SubmitCellBalance(0.25f,0.90f,0.90f,0.45f) : Systems->SubmitCellBalance(0.90f,0.90f,0.90f,0.90f));
    if (ActionId == TEXT("garden.restore-transport-flow"))
        return Systems && (InteractionStep == 1 ? Systems->SubmitTransportFlow(0.35f,0.55f) : Systems->SubmitTransportFlow(0.95f,0.95f));
    if (ActionId == TEXT("garden.tune-root-water-ratio"))
        return Systems && (InteractionStep == 1 ? Systems->SubmitIrrigationRatio(2,3,4,5) : Systems->SubmitIrrigationRatio(2,3,4,6));
    if (ActionId == TEXT("garden.test-sleeping-water"))
        return Systems && (InteractionStep == 1 ? Systems->SubmitSaturationTrial(20.0f,100.0f) : Systems->SubmitSaturationTrial(40.0f,100.0f));
    if (ActionId == TEXT("garden.balance-mineral-transformation"))
        return Systems && (InteractionStep == 1 ? Systems->SubmitConservationTrial(2.0f,1.0f) : Systems->SubmitConservationTrial(4.0f,3.0f));
    if (ActionId == TEXT("garden.find-growth-bottleneck"))
        return Systems && Systems->SubmitGrowthBottleneck(InteractionStep == 1 ? 0.55f : 0.20f, InteractionStep == 1 ? 0.70f : 0.95f);
    if (ActionId == TEXT("garden.restore-pollinator-route"))
        return Systems && InteractionStep >= 2 && Systems->SubmitPollinationLink();

    if (ActionId == TEXT("garden.choose-shared-restoration"))
    {
        if (!Thought) return false;
        const TArray<FName> Perspectives = InteractionStep >= 2
            ? TArray<FName>{TEXT("perspective.river-village"),TEXT("perspective.hill-village")}
            : TArray<FName>{TEXT("perspective.river-village")};
        return Thought->EvaluateEthicalReasoningAndRecord(
            TEXT("ethics.bridge-two-villages-prototype"), TEXT("option.community-vote"),
            {FName(TEXT("reason.shared-decision"))}, Perspectives, InteractionStep >= 2);
    }

    if (ActionId == TEXT("garden.revise-winter-cause"))
    {
        if (!Thought || InteractionStep < 2) return false;
        const FWMPhilosophyProblemDefinition* Problem = Thought->GetCatalog().FindPhilosophyProblem(TEXT("philosophy.garden-restoration-causality"));
        if (!Problem) return false;
        FWMThoughtEvidenceResult Result;
        const bool bAccepted = FWMLanguageThoughtRuntime::EvaluatePhilosophicalArgument(
            *Problem,
            TEXT("claim.garden-network-causes-explain-recovery"),
            {FName(TEXT("link.network-evidence-supports-network-cause"))},
            {FName(TEXT("assumption.system-effects-can-have-multiple-causes"))},
            {FName(TEXT("counterexample.water-restored-but-fruiting-still-fails"))},
            1,
            Result);
        return bAccepted && ResolveMasteryGate(ActionId, TEXT("mastery.garden.causal-revision"));
    }
    return false;
}

bool UWMGardenEndWinterExperienceSubsystem::ResolveTrustedAction(const FName ActionId, const float NumericValue)
{
    const FWMGardenActionDefinition* Action = GetCurrentAction(ActionId);
    if (!Action || !GetWorld()) return false;
    UWMEpicRuntimeSubsystem* Epic = GetWorld()->GetSubsystem<UWMEpicRuntimeSubsystem>();
    if (!Epic) return false;
    bool bResolved = false;
    if (Action->bHasEvidenceRoute)
    {
        bResolved = Epic->RecordEpicEvidence(Action->Evidence.ObjectiveId,Action->Evidence.DisciplineId,Action->Evidence.ProducerKind,
            Action->Evidence.ProducerRefId,Action->Evidence.PrimitiveId,Action->Evidence.EvidenceEventId,NumericValue);
    }
    else if (Action->bHasWorldStateRoute)
    {
        const UWMMissionRuntimeSubsystem* Mission = GetWorld()->GetSubsystem<UWMMissionRuntimeSubsystem>();
        if (!Mission || Mission->GetMissionState()!=EWMMissionRuntimeState::Completed || !AreCurrentMasteryGatesSatisfied()) return false;
        bResolved = Epic->RecordEpicWorldState(Action->WorldState.ProducerKind,Action->WorldState.ProducerRefId,Action->WorldState.WorldStateId);
    }
    if (!bResolved) return false;
    PulseFirstPerson(*Action);
    for (AWMGardenEndWinterInteractableActor* Target:PrototypeTargets) if(IsValid(Target)&&Target->ActionId==ActionId) Target->MarkResolved();
    RefreshPrototypeTargetAvailability();
    return true;
}

bool UWMGardenEndWinterExperienceSubsystem::ResolveValidatedEvidence(
    const FName ProducerRefId, const FName PrimitiveId, const FName EvidenceEventId, const float NumericValue)
{
    const FWMGardenChapterExperience* Chapter=GetCurrentChapter();
    if(!Chapter) return false;
    const FWMGardenActionDefinition* Match=Chapter->Actions.FindByPredicate([&](const FWMGardenActionDefinition& A)
    {
        return A.bHasEvidenceRoute && A.Evidence.ProducerRefId==ProducerRefId && A.Evidence.PrimitiveId==PrimitiveId && A.Evidence.EvidenceEventId==EvidenceEventId;
    });
    return Match && ResolveTrustedAction(Match->ActionId,NumericValue);
}

bool UWMGardenEndWinterExperienceSubsystem::ResolveMasteryGate(const FName ActionId, const FName GateId)
{
    const FWMGardenActionDefinition* Action=GetCurrentAction(ActionId);
    if(!Action || !Action->bHasMasteryGate || Action->MasteryGate.GateId!=GateId) return false;
    SatisfiedMasteryGates.Add(GateId);
    PulseFirstPerson(*Action);
    for (AWMGardenEndWinterInteractableActor* Target:PrototypeTargets) if(IsValid(Target)&&Target->ActionId==ActionId) Target->MarkResolved();
    RefreshPrototypeTargetAvailability();
    return true;
}

void UWMGardenEndWinterExperienceSubsystem::RefreshPrototypeTargetAvailability()
{
    FName ChapterId=NAME_None; if(const FWMGardenChapterExperience* C=GetCurrentChapter()) ChapterId=C->ChapterId;
    for(AWMGardenEndWinterInteractableActor* Target:PrototypeTargets) if(IsValid(Target)) Target->SetAvailable(!ChapterId.IsNone()&&Target->ChapterId==ChapterId);
}

FName UWMGardenEndWinterExperienceSubsystem::RequestHint(const FName ActionId)
{
    const FWMGardenActionDefinition* Action=GetCurrentAction(ActionId); return Action?HintRuntime.RequestHint(*Action):NAME_None;
}
FName UWMGardenEndWinterExperienceSubsystem::GetCurrentFantasyGoalKey() const{const FWMGardenChapterExperience* C=GetCurrentChapter();return C?C->FantasyGoalKey:NAME_None;}
FName UWMGardenEndWinterExperienceSubsystem::GetCurrentTensionKey() const{const FWMGardenChapterExperience* C=GetCurrentChapter();return C?C->TensionKey:NAME_None;}
bool UWMGardenEndWinterExperienceSubsystem::IsWorldStateAction(const FName ActionId) const{const FWMGardenActionDefinition* A=GetCurrentAction(ActionId);return A&&A->bHasWorldStateRoute;}
