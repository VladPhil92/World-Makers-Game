#include "Environment/WMExplorationComponent.h"

#include "Engine/World.h"
#include "Environment/WMBiomeRuntimeSubsystem.h"
#include "GameFramework/Pawn.h"

UWMExplorationComponent::UWMExplorationComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
    SetIsReplicatedByDefault(false);
}

void UWMExplorationComponent::BeginPlay()
{
    Super::BeginPlay();
    PrimaryComponentTick.TickInterval = ObservationIntervalSeconds;
    ObserveNow();
}

void UWMExplorationComponent::TickComponent(
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
    ObserveNow();
}

TArray<FName> UWMExplorationComponent::ObserveNow()
{
    LastNewDiscoveryIds.Reset();
    if (!GetOwner())
    {
        return LastNewDiscoveryIds;
    }

    UWorld* World = GetWorld();
    UWMBiomeRuntimeSubsystem* Biomes = World ? World->GetSubsystem<UWMBiomeRuntimeSubsystem>() : nullptr;
    if (!Biomes)
    {
        ActiveBiomeId = NAME_None;
        CurrentZoneId = NAME_None;
        return LastNewDiscoveryIds;
    }

    LastNewDiscoveryIds = Biomes->ObserveLocation(GetOwner()->GetActorLocation());
    ActiveBiomeId = Biomes->GetActiveBiomeId();
    CurrentZoneId = Biomes->GetCurrentZoneId();
    return LastNewDiscoveryIds;
}
