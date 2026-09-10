#include "Mission/WMMissionObservationAdapterSubsystem.h"

#include "Engine/World.h"
#include "Environment/WMBiomeRuntimeSubsystem.h"
#include "Mission/WMMissionRuntimeSubsystem.h"

void UWMMissionObservationAdapterSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Collection.InitializeDependency<UWMBiomeRuntimeSubsystem>();
    Collection.InitializeDependency<UWMMissionRuntimeSubsystem>();
    Super::Initialize(Collection);

    UWorld* World = GetWorld();
    UWMBiomeRuntimeSubsystem* Biomes = World ? World->GetSubsystem<UWMBiomeRuntimeSubsystem>() : nullptr;
    if (Biomes)
    {
        BoundBiomeRuntime = Biomes;
        Biomes->OnObservationRegistered.AddDynamic(this, &UWMMissionObservationAdapterSubsystem::HandleObservationRegistered);
    }
}

void UWMMissionObservationAdapterSubsystem::Deinitialize()
{
    if (BoundBiomeRuntime.IsValid())
    {
        BoundBiomeRuntime->OnObservationRegistered.RemoveDynamic(this, &UWMMissionObservationAdapterSubsystem::HandleObservationRegistered);
    }
    BoundBiomeRuntime.Reset();
    Super::Deinitialize();
}

void UWMMissionObservationAdapterSubsystem::HandleObservationRegistered(const FName PointId, const FName ObservationId)
{
    (void)PointId;
    UWorld* World = GetWorld();
    UWMMissionRuntimeSubsystem* Missions = World ? World->GetSubsystem<UWMMissionRuntimeSubsystem>() : nullptr;
    if (Missions && Missions->IsObservationRequired(ObservationId))
    {
        Missions->RecordObservationEvidence(ObservationId);
    }
}
