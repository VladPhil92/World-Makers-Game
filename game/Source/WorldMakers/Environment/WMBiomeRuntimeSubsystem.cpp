#include "Environment/WMBiomeRuntimeSubsystem.h"

#include "Engine/World.h"
#include "Environment/WMEnvironmentalInteractableActor.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
    const FName DefaultPrototypeBiomeId(TEXT("biome.caribbean-rainforest"));
}

void UWMBiomeRuntimeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    ReloadAndActivatePrototypeBiome();
}

void UWMBiomeRuntimeSubsystem::Deinitialize()
{
    ClearInteractionTargets();
    BiomeCatalog.Reset();
    AvailableBiomeIds.Reset();
    ActiveBiomeId = NAME_None;
    CurrentZoneId = NAME_None;
    ExplorationProgress.Reset();
    Super::Deinitialize();
}

bool UWMBiomeRuntimeSubsystem::ReloadAndActivatePrototypeBiome()
{
    return ReloadBiomeCatalog() && ActivateBiome(DefaultPrototypeBiomeId);
}

bool UWMBiomeRuntimeSubsystem::ReloadBiomeCatalog()
{
    ClearInteractionTargets();
    BiomeCatalog.Reset();
    AvailableBiomeIds.Reset();
    ActiveBiomeId = NAME_None;
    CurrentZoneId = NAME_None;

    const FString BiomeDirectory = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("WorldMakers/Biomes"));
    TArray<FString> BiomeFiles;
    IFileManager::Get().FindFiles(BiomeFiles, *FPaths::Combine(BiomeDirectory, TEXT("*.json")), true, false);
    BiomeFiles.Sort();

    for (const FString& BiomeFile : BiomeFiles)
    {
        FString Json;
        if (!FFileHelper::LoadFileToString(Json, *FPaths::Combine(BiomeDirectory, BiomeFile)))
        {
            continue;
        }

        FWMBiomeRuntimeDefinition Definition;
        FString Error;
        if (!FWMBiomeRuntimeDefinition::TryParseJson(Json, Definition, Error) || Definition.BiomeId.IsNone())
        {
            continue;
        }

        if (BiomeCatalog.Contains(Definition.BiomeId))
        {
            BiomeCatalog.Reset();
            AvailableBiomeIds.Reset();
            return false;
        }

        AvailableBiomeIds.Add(Definition.BiomeId);
        BiomeCatalog.Add(Definition.BiomeId, MoveTemp(Definition));
    }

    AvailableBiomeIds.Sort([](const FName& A, const FName& B)
    {
        return A.ToString() < B.ToString();
    });
    return !AvailableBiomeIds.IsEmpty();
}

bool UWMBiomeRuntimeSubsystem::ActivateBiome(const FName BiomeId)
{
    if (!BiomeCatalog.Contains(BiomeId))
    {
        return false;
    }

    ClearInteractionTargets();
    ActiveBiomeId = BiomeId;
    CurrentZoneId = NAME_None;
    return true;
}

const FWMBiomeRuntimeDefinition* UWMBiomeRuntimeSubsystem::GetActiveDefinition() const
{
    return BiomeCatalog.Find(ActiveBiomeId);
}

const FWMPointOfInterestDefinition* UWMBiomeRuntimeSubsystem::FindPointOfInterest(const FName PointId) const
{
    const FWMBiomeRuntimeDefinition* Definition = GetActiveDefinition();
    return Definition ? Definition->FindPointOfInterest(PointId) : nullptr;
}

TArray<FName> UWMBiomeRuntimeSubsystem::GetNearbyPointOfInterestIds(const FVector WorldLocation) const
{
    const FWMBiomeRuntimeDefinition* Definition = GetActiveDefinition();
    return Definition ? Definition->FindNearbyPointOfInterestIds(WorldLocation) : TArray<FName>();
}

TArray<FName> UWMBiomeRuntimeSubsystem::GetKnownObservationIds() const
{
    TArray<FName> ObservationIds;
    const FWMBiomeRuntimeDefinition* Definition = GetActiveDefinition();
    if (!Definition) return ObservationIds;

    for (const FWMPointOfInterestDefinition& Point : Definition->PointsOfInterest)
    {
        if (Point.bRequiresInteraction && !Point.ObservationId.IsNone() && !ObservationIds.Contains(Point.ObservationId))
        {
            ObservationIds.Add(Point.ObservationId);
        }
    }
    ObservationIds.Sort([](const FName& A, const FName& B)
    {
        return A.ToString() < B.ToString();
    });
    return ObservationIds;
}

TArray<FName> UWMBiomeRuntimeSubsystem::ObserveLocation(const FVector WorldLocation)
{
    TArray<FName> NewDiscoveryIds;
    const FWMBiomeRuntimeDefinition* Definition = GetActiveDefinition();
    if (!Definition || WorldLocation.ContainsNaN())
    {
        return NewDiscoveryIds;
    }

    const FWMBiomeZoneDefinition* Zone = Definition->FindZoneAtWorldLocation(WorldLocation);
    const FName NewZoneId = Zone ? Zone->ZoneId : NAME_None;
    if (NewZoneId != CurrentZoneId)
    {
        CurrentZoneId = NewZoneId;
        OnZoneChanged.Broadcast(CurrentZoneId);
    }

    for (const FWMPointOfInterestDefinition& Point : Definition->PointsOfInterest)
    {
        // M3.2: deliberate POIs require explicit player intent; proximity cannot satisfy observation.
        if (Point.bRequiresInteraction)
        {
            continue;
        }
        if (!Point.IsWithinDiscoveryRange(WorldLocation, Definition->OriginCm))
        {
            continue;
        }
        if (ExplorationProgress.RegisterDiscovery(Point.DiscoveryId))
        {
            NewDiscoveryIds.Add(Point.DiscoveryId);
            OnDiscoveryRegistered.Broadcast(Point.DiscoveryId);
        }
    }

    NewDiscoveryIds.Sort([](const FName& A, const FName& B)
    {
        return A.ToString() < B.ToString();
    });
    return NewDiscoveryIds;
}

bool UWMBiomeRuntimeSubsystem::RegisterDeliberateInteraction(
    const FName PointId,
    const FVector InteractorLocation,
    FName& OutObservationId)
{
    OutObservationId = NAME_None;
    const FWMBiomeRuntimeDefinition* Definition = GetActiveDefinition();
    const FWMPointOfInterestDefinition* Point = Definition ? Definition->FindPointOfInterest(PointId) : nullptr;
    if (!Definition || !Point || !Point->bRequiresInteraction ||
        !Point->IsWithinInteractionRange(InteractorLocation, Definition->OriginCm))
    {
        return false;
    }

    OutObservationId = Point->ObservationId;
    const bool bNewObservation = ExplorationProgress.RegisterObservation(Point->ObservationId);
    const bool bNewDiscovery = ExplorationProgress.RegisterDiscovery(Point->DiscoveryId);

    if (bNewObservation)
    {
        OnObservationRegistered.Broadcast(Point->PointId, Point->ObservationId);
    }
    if (bNewDiscovery)
    {
        OnDiscoveryRegistered.Broadcast(Point->DiscoveryId);
    }

    // Re-observation is allowed for learning/play, but stable evidence is never duplicated.
    return true;
}

void UWMBiomeRuntimeSubsystem::EnsureInteractionTargets()
{
    UWorld* World = GetWorld();
    const FWMBiomeRuntimeDefinition* Definition = GetActiveDefinition();
    if (!World || !Definition)
    {
        return;
    }

    InteractionTargets.RemoveAll([](const TWeakObjectPtr<AWMEnvironmentalInteractableActor>& Target)
    {
        return !Target.IsValid();
    });

    for (const FWMPointOfInterestDefinition& Point : Definition->PointsOfInterest)
    {
        if (!Point.bRequiresInteraction)
        {
            continue;
        }

        const bool bAlreadyExists = InteractionTargets.ContainsByPredicate([&Point](const TWeakObjectPtr<AWMEnvironmentalInteractableActor>& Target)
        {
            return Target.IsValid() && Target->GetInteractionPointId() == Point.PointId;
        });
        if (bAlreadyExists)
        {
            continue;
        }

        FActorSpawnParameters SpawnParameters;
        SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AWMEnvironmentalInteractableActor* Target = World->SpawnActor<AWMEnvironmentalInteractableActor>(
            AWMEnvironmentalInteractableActor::StaticClass(),
            Definition->OriginCm + Point.LocationCm,
            FRotator::ZeroRotator,
            SpawnParameters);
        if (Target)
        {
            Target->Configure(Point, Definition->OriginCm);
            InteractionTargets.Add(Target);
        }
    }
}

int32 UWMBiomeRuntimeSubsystem::GetInteractionTargetCount() const
{
    int32 Count = 0;
    for (const TWeakObjectPtr<AWMEnvironmentalInteractableActor>& Target : InteractionTargets)
    {
        if (Target.IsValid())
        {
            ++Count;
        }
    }
    return Count;
}

void UWMBiomeRuntimeSubsystem::ClearInteractionTargets()
{
    for (const TWeakObjectPtr<AWMEnvironmentalInteractableActor>& Target : InteractionTargets)
    {
        if (Target.IsValid())
        {
            Target->Destroy();
        }
    }
    InteractionTargets.Reset();
}

void UWMBiomeRuntimeSubsystem::ResetSessionDiscoveries()
{
    ExplorationProgress.Reset();
}
