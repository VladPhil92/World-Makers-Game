#include "Environment/WMInteractionComponent.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "Environment/WMBiomeRuntimeSubsystem.h"
#include "Environment/WMEnvironmentalInteractableActor.h"
#include "Environment/WMEnvironmentActionActor.h"
#include "Environment/WMEnvironmentStateSubsystem.h"
#include "Environment/WMInteractable.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

UWMInteractionComponent::UWMInteractionComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
    SetIsReplicatedByDefault(false);
}

void UWMInteractionComponent::BeginPlay()
{
    Super::BeginPlay();
    PrimaryComponentTick.TickInterval = FocusRefreshIntervalSeconds;

    if (UWorld* World = GetWorld())
    {
        if (UWMBiomeRuntimeSubsystem* Biomes = World->GetSubsystem<UWMBiomeRuntimeSubsystem>())
        {
            Biomes->EnsureInteractionTargets();
        }
        if (UWMEnvironmentStateSubsystem* EnvironmentState = World->GetSubsystem<UWMEnvironmentStateSubsystem>())
        {
            EnvironmentState->EnsureActionTargets();
        }
    }
    RefreshFocus();
}

void UWMInteractionComponent::TickComponent(
    const float DeltaTime,
    const ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    const APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (OwnerPawn && !OwnerPawn->IsLocallyControlled())
    {
        return;
    }
    RefreshFocus();
}

bool UWMInteractionComponent::IsFocusCandidate(
    const FVector& ViewLocation,
    const FVector& ViewForward,
    const FVector& TargetLocation,
    const float MaxDistanceCm,
    const float MinDot)
{
    if (ViewLocation.ContainsNaN() || ViewForward.ContainsNaN() || TargetLocation.ContainsNaN() ||
        !FMath::IsFinite(MaxDistanceCm) || MaxDistanceCm <= 0.0f ||
        !FMath::IsFinite(MinDot) || MinDot < -1.0f || MinDot > 1.0f)
    {
        return false;
    }

    const FVector Delta = TargetLocation - ViewLocation;
    const float DistanceSquared = Delta.SizeSquared();
    if (DistanceSquared <= KINDA_SMALL_NUMBER || DistanceSquared > FMath::Square(MaxDistanceCm))
    {
        return false;
    }

    const FVector Forward = ViewForward.GetSafeNormal();
    if (Forward.IsNearlyZero())
    {
        return false;
    }

    const float Dot = FVector::DotProduct(Forward, Delta.GetSafeNormal());
    return Dot >= MinDot;
}

void UWMInteractionComponent::ClearFocus()
{
    FocusedActor.Reset();
    FocusedPointId = NAME_None;
    FocusedPromptKey = NAME_None;
    FocusedInteractionMode = NAME_None;
    FocusedObservationId = NAME_None;
    FocusedActionId = NAME_None;
    bCanInteract = false;
}

void UWMInteractionComponent::RefreshFocus()
{
    ClearFocus();

    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    UWorld* World = GetWorld();
    APlayerController* PlayerController = OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;
    if (!OwnerPawn || !World || !PlayerController || !OwnerPawn->IsLocallyControlled())
    {
        return;
    }

    if (UWMBiomeRuntimeSubsystem* Biomes = World->GetSubsystem<UWMBiomeRuntimeSubsystem>())
    {
        Biomes->EnsureInteractionTargets();
    }
    if (UWMEnvironmentStateSubsystem* EnvironmentState = World->GetSubsystem<UWMEnvironmentStateSubsystem>())
    {
        EnvironmentState->EnsureActionTargets();
    }

    FVector ViewLocation;
    FRotator ViewRotation;
    PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);
    const FVector ViewForward = ViewRotation.Vector();

    AActor* BestTarget = nullptr;
    float BestDot = -1.0f;
    float BestDistanceSquared = TNumericLimits<float>::Max();

    auto ConsiderTarget = [&](AActor* Target)
    {
        IWMInteractable* Interactable = Target ? Cast<IWMInteractable>(Target) : nullptr;
        if (!Interactable || !Interactable->CanInteract(OwnerPawn))
        {
            return;
        }

        const FVector TargetLocation = Interactable->GetInteractionAnchorLocation();
        if (!IsFocusCandidate(ViewLocation, ViewForward, TargetLocation, FocusMaxDistanceCm, MinimumFocusDot))
        {
            return;
        }

        const FVector Delta = TargetLocation - ViewLocation;
        const float Dot = FVector::DotProduct(ViewForward.GetSafeNormal(), Delta.GetSafeNormal());
        const float DistanceSquared = Delta.SizeSquared();
        if (!BestTarget || Dot > BestDot + KINDA_SMALL_NUMBER ||
            (FMath::IsNearlyEqual(Dot, BestDot) && DistanceSquared < BestDistanceSquared))
        {
            BestTarget = Target;
            BestDot = Dot;
            BestDistanceSquared = DistanceSquared;
        }
    };

    // Preserve the M3.2 observation target path.
    for (TActorIterator<AWMEnvironmentalInteractableActor> It(World); It; ++It)
    {
        ConsiderTarget(*It);
    }

    // M3.4 adds explicit ecosystem-care targets through the same focus contract.
    for (TActorIterator<AWMEnvironmentActionActor> It(World); It; ++It)
    {
        ConsiderTarget(*It);
    }

    IWMInteractable* BestInteractable = BestTarget ? Cast<IWMInteractable>(BestTarget) : nullptr;
    if (!BestInteractable)
    {
        return;
    }

    FocusedActor = BestTarget;
    FocusedPointId = BestInteractable->GetInteractionPointId();
    FocusedPromptKey = BestInteractable->GetInteractionPromptKey();
    FocusedInteractionMode = BestInteractable->GetInteractionMode();
    FocusedObservationId = BestInteractable->GetInteractionObservationId();
    FocusedActionId = BestInteractable->GetInteractionActionId();
    bCanInteract = BestInteractable->CanInteract(OwnerPawn);
}

bool UWMInteractionComponent::TryInteractFocused()
{
    RefreshFocus();
    if (!CanInteractNow() || !GetOwner())
    {
        return false;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }

    const double CurrentTimeSeconds = World->GetTimeSeconds();
    if ((CurrentTimeSeconds - LastInteractionTimeSeconds) < MinimumInteractionIntervalSeconds)
    {
        return false;
    }

    AActor* Target = FocusedActor.Get();
    IWMInteractable* Interactable = Target ? Cast<IWMInteractable>(Target) : nullptr;
    if (!Interactable || !Interactable->Interact(GetOwner()))
    {
        return false;
    }

    LastInteractionTimeSeconds = CurrentTimeSeconds;
    LastObservationId = Interactable->GetInteractionObservationId();
    LastActionId = Interactable->GetInteractionActionId();
    return true;
}
