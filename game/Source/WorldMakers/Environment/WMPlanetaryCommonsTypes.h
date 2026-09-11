#pragma once

#include "CoreMinimal.h"
#include "Environment/WMEnvironmentStateTypes.h"
#include "WMPlanetaryCommonsTypes.generated.h"

USTRUCT(BlueprintType)
struct WORLDMAKERS_API FWMPlanetaryCommonsDelta
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Planetary Commons") float WaterQuality = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Planetary Commons") float AirQuality = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Planetary Commons") float SoilHealth = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Planetary Commons") float Biodiversity = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Planetary Commons") float EnergyReliability = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Planetary Commons") float MaterialDemand = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Planetary Commons") float ComputeEnergyDemand = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Planetary Commons") float HumanWellbeing = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Planetary Commons") float CivicLegitimacy = 0.0f;

    bool IsSane() const;
};

USTRUCT(BlueprintType)
struct WORLDMAKERS_API FWMHabitatLinkDefinition
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Planetary Commons") FName LinkId;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Planetary Commons") float InitialPermeability = 0.0f;

    bool IsSane() const;
};

USTRUCT(BlueprintType)
struct WORLDMAKERS_API FWMSpeciesNeedDefinition
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Living World") FName SpeciesId;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Living World") FName ScientificReviewState;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Living World") float MinWaterQuality = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Living World") float MinAirQuality = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Living World") float MinSoilHealth = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Living World") float MinBiodiversity = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Living World") float MinHabitatConnectivity = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Living World") float DisturbanceSensitivity = 0.0f;

    bool IsSane() const;
};

USTRUCT(BlueprintType)
struct WORLDMAKERS_API FWMSpeciesStateSnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Living World") FName SpeciesId;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Living World") float Stress = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Living World") float Flourishing = 1.0f;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Living World") FName StressBandId;

    bool IsBounded() const;
};

USTRUCT(BlueprintType)
struct WORLDMAKERS_API FWMPlanetaryCommonsSnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Planetary Commons") FName SimulationId;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Planetary Commons") int32 StepIndex = 0;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Planetary Commons") float WaterQuality = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Planetary Commons") float AirQuality = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Planetary Commons") float SoilHealth = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Planetary Commons") float Biodiversity = 0.0f;
    /** Derived from habitat-link permeability. Never authored directly. */
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Planetary Commons") float HabitatConnectivity = 0.0f;
    /** Mean of species-specific stress. Never authored directly. */
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Planetary Commons") float AnimalStress = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Planetary Commons") float EnergyReliability = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Planetary Commons") float MaterialDemand = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Planetary Commons") float ComputeEnergyDemand = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Planetary Commons") float HumanWellbeing = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Planetary Commons") float CivicLegitimacy = 0.0f;

    bool IsBounded() const;
    float GetHookValue(FName HookId) const;
};

USTRUCT(BlueprintType)
struct WORLDMAKERS_API FWMCommonsInterventionDefinition
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Planetary Commons") FName InterventionId;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Planetary Commons") int32 MaxApplications = 1;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Planetary Commons") FWMPlanetaryCommonsDelta Delta;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Planetary Commons") TMap<FName, float> HabitatLinkDeltas;

    bool IsSane() const;
};

USTRUCT(BlueprintType)
struct WORLDMAKERS_API FWMPlanetaryCommonsDefinition
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Planetary Commons") int32 SchemaVersion = 1;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Planetary Commons") FName SimulationId;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Planetary Commons") bool bPrototypeOnly = true;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Planetary Commons") bool bDeterministicStepModel = true;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Planetary Commons") float InitialWaterQuality = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Planetary Commons") float InitialAirQuality = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Planetary Commons") float InitialSoilHealth = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Planetary Commons") float InitialBiodiversity = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Planetary Commons") float InitialEnergyReliability = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Planetary Commons") float InitialMaterialDemand = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Planetary Commons") float InitialComputeEnergyDemand = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Planetary Commons") float InitialHumanWellbeing = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Planetary Commons") float InitialCivicLegitimacy = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Planetary Commons") float ErosionToWaterRate = 0.05f;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Planetary Commons") float EcologyRelaxationRate = 0.10f;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Planetary Commons") float WellbeingRelaxationRate = 0.08f;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Planetary Commons") float MaterialSoilPressureRate = 0.012f;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Planetary Commons") float ComputeReliabilityPressureRate = 0.010f;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Planetary Commons") TArray<FWMHabitatLinkDefinition> HabitatLinks;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Living World") TArray<FWMSpeciesNeedDefinition> SpeciesProfiles;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Planetary Commons") TArray<FWMCommonsInterventionDefinition> Interventions;

    bool IsSane() const;
    const FWMCommonsInterventionDefinition* FindIntervention(FName InterventionId) const;
    static bool TryParseJson(const FString& Json, FWMPlanetaryCommonsDefinition& OutDefinition, FString& OutError);
};

/**
 * Deterministic, framerate-independent simulation for the C1 citizenship hooks.
 * Stable IDs + bounded numeric state only. No child-authored free text and no moral-answer scoring.
 */
struct WORLDMAKERS_API FWMPlanetaryCommonsModel
{
    bool Initialize(const FWMPlanetaryCommonsDefinition& InDefinition);
    bool SeedFromLegacyEcosystem(const FWMEnvironmentStateSnapshot& LegacySnapshot);
    bool ApplyLegacyProjectionDelta(const FWMEnvironmentStateSnapshot& Previous, const FWMEnvironmentStateSnapshot& Current);
    bool CanApplyIntervention(FName InterventionId) const;
    bool ApplyIntervention(FName InterventionId);
    bool AdvanceStep();

    const FWMPlanetaryCommonsSnapshot& GetSnapshot() const { return Snapshot; }
    const TArray<FWMSpeciesStateSnapshot>& GetSpeciesStates() const { return SpeciesStates; }
    float GetHabitatLinkPermeability(FName LinkId) const;
    int32 GetAppliedCount(FName InterventionId) const;

private:
    void ClampPrimaryState();
    void RefreshDerivedState();
    float DeriveHabitatConnectivity() const;
    float DeriveSpeciesStress(const FWMSpeciesNeedDefinition& Species) const;

    bool bInitialized = false;
    FWMPlanetaryCommonsDefinition Definition;
    FWMPlanetaryCommonsSnapshot Snapshot;
    TMap<FName, float> HabitatLinkPermeability;
    TMap<FName, int32> ApplicationCounts;
    TArray<FWMSpeciesStateSnapshot> SpeciesStates;
};
