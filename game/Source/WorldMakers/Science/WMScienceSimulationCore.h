#pragma once

#include "CoreMinimal.h"
#include "Environment/WMEnvironmentStateTypes.h"

enum class EWMMatterState : uint8
{
    Solid,
    Liquid,
    Gas
};

enum class EWMPlantStage : uint8
{
    Seed,
    Germinating,
    Seedling,
    Vegetative,
    Flowering,
    Fruiting
};

struct WORLDMAKERS_API FWMSubstanceDefinition
{
    FName SubstanceId;
    float MolarMassGPerMol = 0.0f;
    float MeltingPointC = 0.0f;
    float BoilingPointC = 100.0f;
    float SolubilityGPer100MlWater = 0.0f;

    bool IsSane() const;
    EWMMatterState ResolveMatterState(float TemperatureC) const;
};

struct WORLDMAKERS_API FWMReactionComponent
{
    FName SubstanceId;
    int32 StoichiometricCoefficient = 1;

    bool IsSane() const;
};

struct WORLDMAKERS_API FWMReactionDefinition
{
    FName ReactionId;
    TArray<FWMReactionComponent> Reactants;
    TArray<FWMReactionComponent> Products;
    FName EvidenceEventId;

    bool IsSane() const;
};

struct WORLDMAKERS_API FWMPlantSpeciesDefinition
{
    FName SpeciesId;
    float MinTemperatureC = 10.0f;
    float OptimalTemperatureC = 25.0f;
    float MaxTemperatureC = 40.0f;
    float GerminationHoursAtIdeal = 48.0f;
    float BaseGrowthUnitsPerHour = 0.01f;
    float FloweringBiomass = 1.0f;
    float FruitingBiomass = 1.5f;

    bool IsSane() const;
};

struct WORLDMAKERS_API FWMScienceSimulationCatalog
{
    int32 SchemaVersion = 1;
    bool bPrototypeOnly = true;
    TMap<FName, FWMSubstanceDefinition> Substances;
    TMap<FName, FWMReactionDefinition> Reactions;
    TMap<FName, FWMPlantSpeciesDefinition> PlantSpecies;

    bool IsSane() const;
    const FWMSubstanceDefinition* FindSubstance(FName SubstanceId) const;
    const FWMReactionDefinition* FindReaction(FName ReactionId) const;
    const FWMPlantSpeciesDefinition* FindPlantSpecies(FName SpeciesId) const;
    static bool TryParseJson(const FString& Json, FWMScienceSimulationCatalog& OutCatalog, FString& OutError);
};

struct WORLDMAKERS_API FWMDissolutionResult
{
    bool bAccepted = false;
    bool bSaturated = false;
    float DissolvedMassG = 0.0f;
    float UndissolvedMassG = 0.0f;
    FName EvidenceEventId;
};

struct WORLDMAKERS_API FWMReactionResult
{
    bool bAccepted = false;
    float ReactionExtentMol = 0.0f;
    float ReactantMassConsumedG = 0.0f;
    float ProductMassCreatedG = 0.0f;
    TMap<FName, float> BeforeMoles;
    TMap<FName, float> AfterMoles;
    FName EvidenceEventId;
};

struct WORLDMAKERS_API FWMPhysicsBodyState
{
    float MassKg = 1.0f;
    FVector PositionMeters = FVector::ZeroVector;
    FVector VelocityMetersPerSecond = FVector::ZeroVector;

    bool IsSane() const;
    bool StepConstantForce(const FVector& ForceNewtons, float DeltaSeconds);
    FVector GetMomentumKgMetersPerSecond() const;
    float GetKineticEnergyJoules() const;
};

struct WORLDMAKERS_API FWMDirectCurrentCircuit
{
    float VoltageVolts = 0.0f;
    float ResistanceOhms = 1.0f;

    bool Solve(float& OutCurrentAmps, float& OutPowerWatts) const;
};

struct WORLDMAKERS_API FWMPlantEnvironmentInput
{
    float WaterAvailability = 1.0f;
    float NutrientAvailability = 1.0f;
    float LightAvailability = 1.0f;
    float PollinatorAvailability = 0.0f;
    float TemperatureC = 25.0f;

    bool IsSane() const;
};

struct WORLDMAKERS_API FWMPlantStepResult
{
    bool bAccepted = false;
    float GrowthUnits = 0.0f;
    float WaterDemandUnits = 0.0f;
    float NutrientDemandUnits = 0.0f;
    float PhotosynthesisProxy = 0.0f;
    float RootProtectionProxy = 0.0f;
    EWMPlantStage PreviousStage = EWMPlantStage::Seed;
    EWMPlantStage NewStage = EWMPlantStage::Seed;
    FName EvidenceEventId;
};

struct WORLDMAKERS_API FWMPlantState
{
    EWMPlantStage Stage = EWMPlantStage::Seed;
    float AgeHours = 0.0f;
    float GerminationProgressHours = 0.0f;
    float BiomassUnits = 0.05f;
    float RootDevelopmentUnits = 0.0f;
    float LeafDevelopmentUnits = 0.0f;

    bool IsSane() const;
    bool Step(const FWMPlantSpeciesDefinition& Species, const FWMPlantEnvironmentInput& Environment, float DeltaHours, FWMPlantStepResult& OutResult);
};

/** Deterministic, virtual-only educational science operations. No real-world experiment procedures are stored here. */
struct WORLDMAKERS_API FWMScienceSimulation
{
    static bool DissolveInWater(
        const FWMSubstanceDefinition& Substance,
        float SoluteMassG,
        float WaterVolumeMl,
        FWMDissolutionResult& OutResult);

    static bool ExecuteReaction(
        const FWMScienceSimulationCatalog& Catalog,
        const FWMReactionDefinition& Reaction,
        const TMap<FName, float>& InventoryMoles,
        FWMReactionResult& OutResult);

    static float ResolveTemperatureSuitability(const FWMPlantSpeciesDefinition& Species, float TemperatureC);

    /** Bridges normalized plant simulation output into the existing bounded ecosystem state model. */
    static FWMEnvironmentStateDelta BuildPlantEnvironmentDelta(const FWMPlantStepResult& PlantResult);
};
