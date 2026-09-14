#include "Adventure/WMGardenSystemsRuntime.h"

#include "Adventure/WMGardenEndWinterExperienceSubsystem.h"
#include "Science/WMScienceSimulationSubsystem.h"

bool FWMGardenSystemsRuntime::IsCellBalanceRestored(const float Nutrients,const float Oxygen,const float TemperatureSuitability,const float TransportEfficiency)
{
    FWMCellSystemState State; State.MembraneIntegrity=0.90f; State.EnergyAvailability=0.20f; State.TransportEfficiency=TransportEfficiency; State.InformationIntegrity=0.90f; State.WasteLoad=0.35f;
    FWMCellEnvironmentInput Environment; Environment.NutrientAvailability=Nutrients; Environment.OxygenAvailability=Oxygen; Environment.TemperatureSuitability=TemperatureSuitability;
    FWMCellStepResult Result;
    if(!State.StepMetabolism(Environment,3.0f,Result)||!Result.bAccepted) return false;
    return Result.EnergyProducedUnits>=0.15f&&State.EnergyAvailability>=0.30f&&State.WasteLoad<=0.42f;
}

bool FWMGardenSystemsRuntime::IsTransportFlowRestored(const float TransportEfficiency,const float MembraneIntegrity)
{
    FWMCellSystemState State; State.MembraneIntegrity=MembraneIntegrity; State.EnergyAvailability=0.65f; State.TransportEfficiency=TransportEfficiency; State.InformationIntegrity=0.90f; State.WasteLoad=0.55f;
    FWMCellEnvironmentInput Environment; Environment.NutrientAvailability=0.90f; Environment.OxygenAvailability=0.90f; Environment.TemperatureSuitability=0.90f;
    FWMCellStepResult Result;
    if(!State.StepMetabolism(Environment,3.0f,Result)||!Result.bAccepted) return false;
    return Result.TransportWorkUnits>=0.10f&&State.WasteLoad<=0.60f&&State.EnergyAvailability>=0.70f;
}

bool FWMGardenSystemsRuntime::IsIrrigationRatioBalanced(const int32 RootA,const int32 WaterA,const int32 RootB,const int32 WaterB)
{
    if(RootA<=0||WaterA<=0||RootB<=0||WaterB<=0||RootA>1000||WaterA>1000||RootB>1000||WaterB>1000) return false;
    if(RootA==RootB&&WaterA==WaterB) return false;
    return static_cast<int64>(RootA)*static_cast<int64>(WaterB)==static_cast<int64>(RootB)*static_cast<int64>(WaterA);
}

bool FWMGardenSystemsRuntime::IsSaturationThresholdFound(const FWMSubstanceDefinition& Substance,const float SoluteMassG,const float WaterVolumeMl)
{
    FWMDissolutionResult Result;
    return FWMScienceSimulation::DissolveInWater(Substance,SoluteMassG,WaterVolumeMl,Result)&&Result.bAccepted&&Result.bSaturated&&Result.UndissolvedMassG>=0.0f;
}

bool FWMGardenSystemsRuntime::IsConservationModelClosed(const FWMScienceSimulationCatalog& Catalog,const FWMReactionDefinition& Reaction,const TMap<FName,float>& InventoryMoles)
{
    FWMReactionResult Result;
    if(!FWMScienceSimulation::ExecuteReaction(Catalog,Reaction,InventoryMoles,Result)||!Result.bAccepted||Result.ReactantMassConsumedG<=0.0f||
        !FMath::IsNearlyEqual(Result.ReactantMassConsumedG,Result.ProductMassCreatedG,0.05f)) return false;
    // The gameplay gate asks the player to close the whole transformation, not merely trigger a valid partial reaction.
    for(const FWMReactionComponent& Reactant:Reaction.Reactants)
    {
        const float Remaining=Result.AfterMoles.FindRef(Reactant.SubstanceId);
        if(Remaining>0.01f) return false;
    }
    return true;
}

bool FWMGardenSystemsRuntime::IsGrowthLimitingFactorDemonstrated(const FWMPlantSpeciesDefinition& Species,const float ConstrainedWater,const float RestoredWater)
{
    if(!FMath::IsFinite(ConstrainedWater)||!FMath::IsFinite(RestoredWater)||ConstrainedWater<0.0f||RestoredWater>1.0f||RestoredWater<=ConstrainedWater) return false;
    FWMPlantState Constrained; Constrained.Stage=EWMPlantStage::Seedling; Constrained.AgeHours=72.0f; Constrained.GerminationProgressHours=Species.GerminationHoursAtIdeal; Constrained.BiomassUnits=0.18f; Constrained.RootDevelopmentUnits=0.08f; Constrained.LeafDevelopmentUnits=0.08f;
    FWMPlantState Restored=Constrained;
    FWMPlantEnvironmentInput Low; Low.WaterAvailability=ConstrainedWater; Low.NutrientAvailability=0.95f; Low.LightAvailability=0.95f; Low.PollinatorAvailability=0.0f; Low.TemperatureC=Species.OptimalTemperatureC;
    FWMPlantEnvironmentInput High=Low; High.WaterAvailability=RestoredWater;
    FWMPlantStepResult LowResult,HighResult;
    if(!Constrained.Step(Species,Low,12.0f,LowResult)||!Restored.Step(Species,High,12.0f,HighResult)) return false;
    return HighResult.GrowthUnits>=LowResult.GrowthUnits*1.75f&&HighResult.GrowthUnits>0.0f;
}

bool FWMGardenSystemsRuntime::IsPollinationFruitingLinkDemonstrated(const FWMPlantSpeciesDefinition& Species)
{
    FWMPlantState Without; Without.Stage=EWMPlantStage::Flowering; Without.AgeHours=180.0f; Without.GerminationProgressHours=Species.GerminationHoursAtIdeal; Without.BiomassUnits=FMath::Max(Species.FruitingBiomass-0.02f,Species.FloweringBiomass); Without.RootDevelopmentUnits=0.7f; Without.LeafDevelopmentUnits=0.8f;
    FWMPlantState With=Without;
    FWMPlantEnvironmentInput Environment; Environment.WaterAvailability=1.0f; Environment.NutrientAvailability=1.0f; Environment.LightAvailability=1.0f; Environment.TemperatureC=Species.OptimalTemperatureC; Environment.PollinatorAvailability=0.0f;
    FWMPlantStepResult WithoutResult; if(!Without.Step(Species,Environment,4.0f,WithoutResult)) return false;
    Environment.PollinatorAvailability=1.0f;
    FWMPlantStepResult WithResult; if(!With.Step(Species,Environment,4.0f,WithResult)) return false;
    return WithoutResult.NewStage!=EWMPlantStage::Fruiting&&WithResult.NewStage==EWMPlantStage::Fruiting&&WithResult.EvidenceEventId==TEXT("science.botany.pollination-fruiting-link-demonstrated");
}

bool UWMGardenSystemsSubsystem::SubmitCellBalance(const float Nutrients,const float Oxygen,const float TemperatureSuitability,const float TransportEfficiency)
{
    if(!FWMGardenSystemsRuntime::IsCellBalanceRestored(Nutrients,Oxygen,TemperatureSuitability,TransportEfficiency)||!GetWorld()) return false;
    UWMGardenEndWinterExperienceSubsystem* Garden=GetWorld()->GetSubsystem<UWMGardenEndWinterExperienceSubsystem>(); return Garden&&Garden->ResolveTrustedAction(TEXT("garden.balance-cell-energy"));
}
bool UWMGardenSystemsSubsystem::SubmitTransportFlow(const float TransportEfficiency,const float MembraneIntegrity)
{
    if(!FWMGardenSystemsRuntime::IsTransportFlowRestored(TransportEfficiency,MembraneIntegrity)||!GetWorld()) return false;
    UWMGardenEndWinterExperienceSubsystem* Garden=GetWorld()->GetSubsystem<UWMGardenEndWinterExperienceSubsystem>(); return Garden&&Garden->ResolveTrustedAction(TEXT("garden.restore-transport-flow"));
}
bool UWMGardenSystemsSubsystem::SubmitIrrigationRatio(const int32 RootA,const int32 WaterA,const int32 RootB,const int32 WaterB)
{
    if(!FWMGardenSystemsRuntime::IsIrrigationRatioBalanced(RootA,WaterA,RootB,WaterB)||!GetWorld()) return false;
    UWMGardenEndWinterExperienceSubsystem* Garden=GetWorld()->GetSubsystem<UWMGardenEndWinterExperienceSubsystem>(); return Garden&&Garden->ResolveMasteryGate(TEXT("garden.tune-root-water-ratio"),TEXT("mastery.garden.irrigation-ratio"));
}
bool UWMGardenSystemsSubsystem::SubmitSaturationTrial(const float SoluteMassG,const float WaterVolumeMl)
{
    if(!GetWorld()) return false; UWMScienceSimulationSubsystem* Science=GetWorld()->GetSubsystem<UWMScienceSimulationSubsystem>();
    if(!Science||!Science->IsScienceCatalogLoaded()) return false; const FWMSubstanceDefinition* Substance=Science->GetCatalog().FindSubstance(TEXT("substance.sodium-chloride"));
    if(!Substance||!FWMGardenSystemsRuntime::IsSaturationThresholdFound(*Substance,SoluteMassG,WaterVolumeMl)) return false;
    UWMGardenEndWinterExperienceSubsystem* Garden=GetWorld()->GetSubsystem<UWMGardenEndWinterExperienceSubsystem>(); return Garden&&Garden->ResolveTrustedAction(TEXT("garden.test-sleeping-water"));
}
bool UWMGardenSystemsSubsystem::SubmitConservationTrial(const float IronMoles,const float OxygenMoles)
{
    if(!GetWorld()) return false; UWMScienceSimulationSubsystem* Science=GetWorld()->GetSubsystem<UWMScienceSimulationSubsystem>();
    if(!Science||!Science->IsScienceCatalogLoaded()) return false; const FWMReactionDefinition* Reaction=Science->GetCatalog().FindReaction(TEXT("reaction.iron-oxidation"));
    TMap<FName,float> Inventory; Inventory.Add(TEXT("substance.iron"),IronMoles); Inventory.Add(TEXT("substance.oxygen"),OxygenMoles);
    if(!Reaction||!FWMGardenSystemsRuntime::IsConservationModelClosed(Science->GetCatalog(),*Reaction,Inventory)) return false;
    UWMGardenEndWinterExperienceSubsystem* Garden=GetWorld()->GetSubsystem<UWMGardenEndWinterExperienceSubsystem>(); return Garden&&Garden->ResolveTrustedAction(TEXT("garden.balance-mineral-transformation"));
}
bool UWMGardenSystemsSubsystem::SubmitGrowthBottleneck(const float ConstrainedWater,const float RestoredWater)
{
    if(!GetWorld()) return false; UWMScienceSimulationSubsystem* Science=GetWorld()->GetSubsystem<UWMScienceSimulationSubsystem>();
    if(!Science||!Science->IsScienceCatalogLoaded()) return false; const FWMPlantSpeciesDefinition* Species=Science->GetCatalog().FindPlantSpecies(TEXT("plant.rainforest-learning-prototype"));
    if(!Species||!FWMGardenSystemsRuntime::IsGrowthLimitingFactorDemonstrated(*Species,ConstrainedWater,RestoredWater)) return false;
    UWMGardenEndWinterExperienceSubsystem* Garden=GetWorld()->GetSubsystem<UWMGardenEndWinterExperienceSubsystem>(); return Garden&&Garden->ResolveTrustedAction(TEXT("garden.find-growth-bottleneck"));
}
bool UWMGardenSystemsSubsystem::SubmitPollinationLink()
{
    if(!GetWorld()) return false; UWMScienceSimulationSubsystem* Science=GetWorld()->GetSubsystem<UWMScienceSimulationSubsystem>();
    if(!Science||!Science->IsScienceCatalogLoaded()) return false; const FWMPlantSpeciesDefinition* Species=Science->GetCatalog().FindPlantSpecies(TEXT("plant.rainforest-learning-prototype"));
    if(!Species||!FWMGardenSystemsRuntime::IsPollinationFruitingLinkDemonstrated(*Species)) return false;
    UWMGardenEndWinterExperienceSubsystem* Garden=GetWorld()->GetSubsystem<UWMGardenEndWinterExperienceSubsystem>(); return Garden&&Garden->ResolveTrustedAction(TEXT("garden.restore-pollinator-route"));
}
