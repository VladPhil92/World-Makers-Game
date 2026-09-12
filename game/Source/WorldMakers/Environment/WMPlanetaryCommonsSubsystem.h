#pragma once

#include "CoreMinimal.h"
#include "Environment/WMPlanetaryCommonsTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "WMPlanetaryCommonsSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FWMPlanetaryCommonsChangedSignature,
    FName, CausalId,
    FWMPlanetaryCommonsSnapshot, Snapshot);

/**
 * World-level authority for the C1 simulation hooks. M3.4 remains the local care-action
 * authority; its accepted state changes are projected into this broader model as deltas.
 */
UCLASS()
class WORLDMAKERS_API UWMPlanetaryCommonsSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "World Makers|Planetary Commons")
    bool ReloadPrototypeProfile();

    UFUNCTION(BlueprintCallable, Category = "World Makers|Planetary Commons")
    bool SyncFromLegacyEcosystem();

    UFUNCTION(BlueprintPure, Category = "World Makers|Planetary Commons")
    FWMPlanetaryCommonsSnapshot GetSnapshot() const { return Model.GetSnapshot(); }

    UFUNCTION(BlueprintPure, Category = "World Makers|Planetary Commons")
    TArray<FWMSpeciesStateSnapshot> GetSpeciesStates() const { return Model.GetSpeciesStates(); }

    UFUNCTION(BlueprintPure, Category = "World Makers|Planetary Commons")
    bool CanApplyIntervention(FName InterventionId) const { return Model.CanApplyIntervention(InterventionId); }

    UFUNCTION(BlueprintCallable, Category = "World Makers|Planetary Commons")
    bool ApplyIntervention(FName InterventionId);

    UFUNCTION(BlueprintCallable, Category = "World Makers|Planetary Commons")
    bool AdvanceSimulationStep();

    UPROPERTY(BlueprintAssignable, Category = "World Makers|Planetary Commons")
    FWMPlanetaryCommonsChangedSignature OnPlanetaryCommonsChanged;

private:
    UFUNCTION()
    void HandleLegacyEnvironmentChanged(FWMEnvironmentStateSnapshot Snapshot);

    void Broadcast(FName CausalId);

    FWMPlanetaryCommonsDefinition Definition;
    FWMPlanetaryCommonsModel Model;
    FWMEnvironmentStateSnapshot LastLegacySnapshot;
    bool bHasLegacySnapshot = false;
};
