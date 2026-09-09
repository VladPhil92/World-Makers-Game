#pragma once

#include "CoreMinimal.h"
#include "Environment/WMBiomeTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "WMBiomeRuntimeSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWMBiomeZoneChangedSignature, FName, ZoneId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWMDiscoveryRegisteredSignature, FName, DiscoveryId);

UCLASS()
class WORLDMAKERS_API UWMBiomeRuntimeSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

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

    UFUNCTION(BlueprintCallable, Category = "World Makers|Exploration")
    TArray<FName> ObserveLocation(FVector WorldLocation);

    UFUNCTION(BlueprintPure, Category = "World Makers|Exploration")
    TArray<FName> GetNearbyPointOfInterestIds(FVector WorldLocation) const;

    UFUNCTION(BlueprintPure, Category = "World Makers|Exploration")
    TArray<FName> GetDiscoveredIds() const { return ExplorationProgress.GetDiscoveredIds(); }

    UFUNCTION(BlueprintPure, Category = "World Makers|Exploration")
    bool HasDiscovered(FName DiscoveryId) const { return ExplorationProgress.HasDiscovered(DiscoveryId); }

    UFUNCTION(BlueprintCallable, Category = "World Makers|Exploration")
    void ResetSessionDiscoveries();

    UPROPERTY(BlueprintAssignable, Category = "World Makers|Exploration")
    FWMBiomeZoneChangedSignature OnZoneChanged;

    UPROPERTY(BlueprintAssignable, Category = "World Makers|Exploration")
    FWMDiscoveryRegisteredSignature OnDiscoveryRegistered;

private:
    const FWMBiomeRuntimeDefinition* GetActiveDefinition() const;

    TMap<FName, FWMBiomeRuntimeDefinition> BiomeCatalog;
    TArray<FName> AvailableBiomeIds;
    FName ActiveBiomeId;
    FName CurrentZoneId;
    FWMExplorationProgressModel ExplorationProgress;
};
