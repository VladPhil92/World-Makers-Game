#pragma once

#include "CoreMinimal.h"
#include "Environment/WMBiomeTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "WMBiomeRuntimeSubsystem.generated.h"

class AWMEnvironmentalInteractableActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWMBiomeZoneChangedSignature, FName, ZoneId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWMDiscoveryRegisteredSignature, FName, DiscoveryId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FWMObservationRegisteredSignature, FName, PointId, FName, ObservationId);

UCLASS()
class WORLDMAKERS_API UWMBiomeRuntimeSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "World Makers|Biome")
    bool ReloadAndActivatePrototypeBiome();

    UFUNCTION(BlueprintCallable, Category = "World Makers|Biome")
    bool ReloadBiomeCatalog();

    UFUNCTION(BlueprintCallable, Category = "World Makers|Biome")
    bool ActivateBiome(FName BiomeId);

    UFUNCTION(BlueprintPure, Category = "World Makers|Biome")
    TArray<FName> GetAvailableBiomeIds() const { return AvailableBiomeIds; }

    UFUNCTION(BlueprintPure, Category = "World Makers|Biome")
    FName GetActiveBiomeId() const { return ActiveBiomeId; }

    UFUNCTION(BlueprintPure, Category = "World Makers|Exploration")
    FName GetCurrentZoneId() const { return CurrentZoneId; }

    /** M3.1 zone observation. M3.2 deliberately-interactive POIs are excluded from passive discovery. */
    UFUNCTION(BlueprintCallable, Category = "World Makers|Exploration")
    TArray<FName> ObserveLocation(FVector WorldLocation);

    UFUNCTION(BlueprintPure, Category = "World Makers|Exploration")
    TArray<FName> GetNearbyPointOfInterestIds(FVector WorldLocation) const;

    UFUNCTION(BlueprintPure, Category = "World Makers|Exploration")
    TArray<FName> GetDiscoveredIds() const { return ExplorationProgress.GetDiscoveredIds(); }

    UFUNCTION(BlueprintPure, Category = "World Makers|Exploration")
    bool HasDiscovered(FName DiscoveryId) const { return ExplorationProgress.HasDiscovered(DiscoveryId); }

    UFUNCTION(BlueprintPure, Category = "World Makers|Interaction")
    TArray<FName> GetObservedIds() const { return ExplorationProgress.GetObservedIds(); }

    /** Stable observation IDs authored by the active biome. Read-only projection for journey UI and pedagogy adapters. */
    UFUNCTION(BlueprintPure, Category = "World Makers|Interaction")
    TArray<FName> GetKnownObservationIds() const;

    UFUNCTION(BlueprintPure, Category = "World Makers|Interaction")
    bool HasObserved(FName ObservationId) const { return ExplorationProgress.HasObserved(ObservationId); }

    /** Spawn semantic interaction anchors for deliberate POIs after gameplay has begun. Idempotent. */
    UFUNCTION(BlueprintCallable, Category = "World Makers|Interaction")
    void EnsureInteractionTargets();

    UFUNCTION(BlueprintPure, Category = "World Makers|Interaction")
    int32 GetInteractionTargetCount() const;

    /** Accepted interactions return true even when already observed; delegates only fire for new stable IDs. */
    bool RegisterDeliberateInteraction(FName PointId, FVector InteractorLocation, FName& OutObservationId);

    const FWMPointOfInterestDefinition* FindPointOfInterest(FName PointId) const;

    UFUNCTION(BlueprintCallable, Category = "World Makers|Exploration")
    void ResetSessionDiscoveries();

    UPROPERTY(BlueprintAssignable, Category = "World Makers|Exploration")
    FWMBiomeZoneChangedSignature OnZoneChanged;

    UPROPERTY(BlueprintAssignable, Category = "World Makers|Exploration")
    FWMDiscoveryRegisteredSignature OnDiscoveryRegistered;

    UPROPERTY(BlueprintAssignable, Category = "World Makers|Interaction")
    FWMObservationRegisteredSignature OnObservationRegistered;

private:
    const FWMBiomeRuntimeDefinition* GetActiveDefinition() const;
    void ClearInteractionTargets();

    TMap<FName, FWMBiomeRuntimeDefinition> BiomeCatalog;
    TArray<FName> AvailableBiomeIds;
    FName ActiveBiomeId;
    FName CurrentZoneId;
    FWMExplorationProgressModel ExplorationProgress;
    TArray<TWeakObjectPtr<AWMEnvironmentalInteractableActor>> InteractionTargets;
};
