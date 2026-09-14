#pragma once

#include "CoreMinimal.h"
#include "Science/WMScienceSimulationCore.h"
#include "Subsystems/WorldSubsystem.h"
#include "WMGardenSystemsRuntime.generated.h"

/** Deterministic living-system mechanics for The Garden at the End of Winter. */
struct WORLDMAKERS_API FWMGardenSystemsRuntime
{
    static bool IsCellBalanceRestored(float Nutrients, float Oxygen, float TemperatureSuitability, float TransportEfficiency);
    static bool IsTransportFlowRestored(float TransportEfficiency, float MembraneIntegrity);
    static bool IsIrrigationRatioBalanced(int32 RootA, int32 WaterA, int32 RootB, int32 WaterB);
    static bool IsSaturationThresholdFound(const FWMSubstanceDefinition& Substance, float SoluteMassG, float WaterVolumeMl);
    static bool IsConservationModelClosed(const FWMScienceSimulationCatalog& Catalog, const FWMReactionDefinition& Reaction, const TMap<FName,float>& InventoryMoles);
    static bool IsGrowthLimitingFactorDemonstrated(const FWMPlantSpeciesDefinition& Species, float ConstrainedWater, float RestoredWater);
    static bool IsPollinationFruitingLinkDemonstrated(const FWMPlantSpeciesDefinition& Species);
};

/** Trusted C++ bridge from simulated living-system outcomes into Garden epic authority. */
UCLASS()
class WORLDMAKERS_API UWMGardenSystemsSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    bool SubmitCellBalance(float Nutrients, float Oxygen, float TemperatureSuitability, float TransportEfficiency);
    bool SubmitTransportFlow(float TransportEfficiency, float MembraneIntegrity);
    bool SubmitIrrigationRatio(int32 RootA, int32 WaterA, int32 RootB, int32 WaterB);
    bool SubmitSaturationTrial(float SoluteMassG, float WaterVolumeMl);
    bool SubmitConservationTrial(float IronMoles, float OxygenMoles);
    bool SubmitGrowthBottleneck(float ConstrainedWater, float RestoredWater);
    bool SubmitPollinationLink();
};
