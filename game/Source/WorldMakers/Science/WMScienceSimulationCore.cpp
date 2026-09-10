#include "Science/WMScienceSimulationCore.h"

#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
    constexpr float MassBalanceToleranceGPerStoichUnit = 0.05f;

    bool IsUnitInterval(const float Value)
    {
        return FMath::IsFinite(Value) && Value >= 0.0f && Value <= 1.0f;
    }

    bool ReadReactionComponents(
        const TSharedPtr<FJsonObject>& Object,
        const TCHAR* FieldName,
        TArray<FWMReactionComponent>& OutComponents)
    {
        const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
        if (!Object.IsValid() || !Object->TryGetArrayField(FieldName, Values) || !Values || Values->IsEmpty())
        {
            return false;
        }

        OutComponents.Reset();
        for (const TSharedPtr<FJsonValue>& Value : *Values)
        {
            const TSharedPtr<FJsonObject>* ComponentObject = nullptr;
            if (!Value.IsValid() || !Value->TryGetObject(ComponentObject) || !ComponentObject || !ComponentObject->IsValid())
            {
                return false;
            }

            FString SubstanceId;
            int32 Coefficient = 0;
            if (!(*ComponentObject)->TryGetStringField(TEXT("substanceId"), SubstanceId) || SubstanceId.IsEmpty() ||
                !(*ComponentObject)->TryGetNumberField(TEXT("coefficient"), Coefficient))
            {
                return false;
            }

            FWMReactionComponent Component;
            Component.SubstanceId = FName(*SubstanceId);
            Component.StoichiometricCoefficient = Coefficient;
            if (!Component.IsSane())
            {
                return false;
            }
            OutComponents.Add(MoveTemp(Component));
        }
        return true;
    }

    float CalculateStoichiometricMass(
        const FWMScienceSimulationCatalog& Catalog,
        const TArray<FWMReactionComponent>& Components)
    {
        float TotalMass = 0.0f;
        for (const FWMReactionComponent& Component : Components)
        {
            const FWMSubstanceDefinition* Substance = Catalog.FindSubstance(Component.SubstanceId);
            if (!Substance)
            {
                return -1.0f;
            }
            TotalMass += Substance->MolarMassGPerMol * static_cast<float>(Component.StoichiometricCoefficient);
        }
        return TotalMass;
    }
}

bool FWMSubstanceDefinition::IsSane() const
{
    return !SubstanceId.IsNone() && FMath::IsFinite(MolarMassGPerMol) && MolarMassGPerMol > 0.0f &&
        FMath::IsFinite(MeltingPointC) && FMath::IsFinite(BoilingPointC) && MeltingPointC < BoilingPointC &&
        FMath::IsFinite(SolubilityGPer100MlWater) && SolubilityGPer100MlWater >= 0.0f;
}

EWMMatterState FWMSubstanceDefinition::ResolveMatterState(const float TemperatureC) const
{
    if (TemperatureC < MeltingPointC) return EWMMatterState::Solid;
    if (TemperatureC < BoilingPointC) return EWMMatterState::Liquid;
    return EWMMatterState::Gas;
}

bool FWMReactionComponent::IsSane() const
{
    return !SubstanceId.IsNone() && StoichiometricCoefficient > 0 && StoichiometricCoefficient <= 64;
}

bool FWMReactionDefinition::IsSane() const
{
    if (ReactionId.IsNone() || EvidenceEventId.IsNone() || Reactants.IsEmpty() || Products.IsEmpty())
    {
        return false;
    }

    TSet<FName> SeenReactants;
    for (const FWMReactionComponent& Component : Reactants)
    {
        if (!Component.IsSane() || SeenReactants.Contains(Component.SubstanceId)) return false;
        SeenReactants.Add(Component.SubstanceId);
    }

    TSet<FName> SeenProducts;
    for (const FWMReactionComponent& Component : Products)
    {
        if (!Component.IsSane() || SeenProducts.Contains(Component.SubstanceId)) return false;
        SeenProducts.Add(Component.SubstanceId);
    }
    return true;
}

bool FWMPlantSpeciesDefinition::IsSane() const
{
    return !SpeciesId.IsNone() && FMath::IsFinite(MinTemperatureC) && FMath::IsFinite(OptimalTemperatureC) &&
        FMath::IsFinite(MaxTemperatureC) && MinTemperatureC < OptimalTemperatureC && OptimalTemperatureC < MaxTemperatureC &&
        FMath::IsFinite(GerminationHoursAtIdeal) && GerminationHoursAtIdeal > 0.0f &&
        FMath::IsFinite(BaseGrowthUnitsPerHour) && BaseGrowthUnitsPerHour > 0.0f &&
        FMath::IsFinite(FloweringBiomass) && FMath::IsFinite(FruitingBiomass) &&
        FloweringBiomass > 0.05f && FruitingBiomass > FloweringBiomass;
}

const FWMSubstanceDefinition* FWMScienceSimulationCatalog::FindSubstance(const FName SubstanceId) const
{
    return Substances.Find(SubstanceId);
}

const FWMReactionDefinition* FWMScienceSimulationCatalog::FindReaction(const FName ReactionId) const
{
    return Reactions.Find(ReactionId);
}

const FWMPlantSpeciesDefinition* FWMScienceSimulationCatalog::FindPlantSpecies(const FName SpeciesId) const
{
    return PlantSpecies.Find(SpeciesId);
}

bool FWMScienceSimulationCatalog::IsSane() const
{
    if (SchemaVersion != 1 || Substances.IsEmpty() || Reactions.IsEmpty() || PlantSpecies.IsEmpty())
    {
        return false;
    }

    for (const TPair<FName, FWMSubstanceDefinition>& Pair : Substances)
    {
        if (Pair.Key != Pair.Value.SubstanceId || !Pair.Value.IsSane()) return false;
    }

    for (const TPair<FName, FWMReactionDefinition>& Pair : Reactions)
    {
        if (Pair.Key != Pair.Value.ReactionId || !Pair.Value.IsSane()) return false;
        const float ReactantMass = CalculateStoichiometricMass(*this, Pair.Value.Reactants);
        const float ProductMass = CalculateStoichiometricMass(*this, Pair.Value.Products);
        if (ReactantMass <= 0.0f || ProductMass <= 0.0f || !FMath::IsNearlyEqual(ReactantMass, ProductMass, MassBalanceToleranceGPerStoichUnit))
        {
            return false;
        }
    }

    for (const TPair<FName, FWMPlantSpeciesDefinition>& Pair : PlantSpecies)
    {
        if (Pair.Key != Pair.Value.SpeciesId || !Pair.Value.IsSane()) return false;
    }
    return true;
}

bool FWMScienceSimulationCatalog::TryParseJson(const FString& Json, FWMScienceSimulationCatalog& OutCatalog, FString& OutError)
{
    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        OutError = TEXT("Science simulation JSON is not a valid object.");
        return false;
    }

    int32 SchemaVersion = 0;
    bool bPrototypeOnly = true;
    const TArray<TSharedPtr<FJsonValue>>* SubstanceValues = nullptr;
    const TArray<TSharedPtr<FJsonValue>>* ReactionValues = nullptr;
    const TArray<TSharedPtr<FJsonValue>>* PlantValues = nullptr;
    if (!Root->TryGetNumberField(TEXT("schemaVersion"), SchemaVersion) ||
        !Root->TryGetBoolField(TEXT("prototypeOnly"), bPrototypeOnly) ||
        !Root->TryGetArrayField(TEXT("substances"), SubstanceValues) || !SubstanceValues ||
        !Root->TryGetArrayField(TEXT("reactions"), ReactionValues) || !ReactionValues ||
        !Root->TryGetArrayField(TEXT("plantSpecies"), PlantValues) || !PlantValues)
    {
        OutError = TEXT("Science simulation catalog header is invalid.");
        return false;
    }

    FWMScienceSimulationCatalog Candidate;
    Candidate.SchemaVersion = SchemaVersion;
    Candidate.bPrototypeOnly = bPrototypeOnly;

    for (const TSharedPtr<FJsonValue>& Value : *SubstanceValues)
    {
        const TSharedPtr<FJsonObject>* Object = nullptr;
        if (!Value.IsValid() || !Value->TryGetObject(Object) || !Object || !Object->IsValid())
        {
            OutError = TEXT("Science substance entry is invalid.");
            return false;
        }

        FString Id;
        double MolarMass = 0.0;
        double MeltingPoint = 0.0;
        double BoilingPoint = 0.0;
        double Solubility = 0.0;
        if (!(*Object)->TryGetStringField(TEXT("substanceId"), Id) || Id.IsEmpty() ||
            !(*Object)->TryGetNumberField(TEXT("molarMassGPerMol"), MolarMass) ||
            !(*Object)->TryGetNumberField(TEXT("meltingPointC"), MeltingPoint) ||
            !(*Object)->TryGetNumberField(TEXT("boilingPointC"), BoilingPoint) ||
            !(*Object)->TryGetNumberField(TEXT("solubilityGPer100MlWater"), Solubility))
        {
            OutError = TEXT("Science substance fields are invalid.");
            return false;
        }

        FWMSubstanceDefinition Substance;
        Substance.SubstanceId = FName(*Id);
        Substance.MolarMassGPerMol = static_cast<float>(MolarMass);
        Substance.MeltingPointC = static_cast<float>(MeltingPoint);
        Substance.BoilingPointC = static_cast<float>(BoilingPoint);
        Substance.SolubilityGPer100MlWater = static_cast<float>(Solubility);
        if (!Substance.IsSane() || Candidate.Substances.Contains(Substance.SubstanceId))
        {
            OutError = TEXT("Science substance definition failed semantic validation.");
            return false;
        }
        Candidate.Substances.Add(Substance.SubstanceId, MoveTemp(Substance));
    }

    for (const TSharedPtr<FJsonValue>& Value : *ReactionValues)
    {
        const TSharedPtr<FJsonObject>* Object = nullptr;
        if (!Value.IsValid() || !Value->TryGetObject(Object) || !Object || !Object->IsValid())
        {
            OutError = TEXT("Science reaction entry is invalid.");
            return false;
        }

        FString ReactionId;
        FString EvidenceEventId;
        FWMReactionDefinition Reaction;
        if (!(*Object)->TryGetStringField(TEXT("reactionId"), ReactionId) || ReactionId.IsEmpty() ||
            !(*Object)->TryGetStringField(TEXT("evidenceEventId"), EvidenceEventId) || EvidenceEventId.IsEmpty() ||
            !ReadReactionComponents(*Object, TEXT("reactants"), Reaction.Reactants) ||
            !ReadReactionComponents(*Object, TEXT("products"), Reaction.Products))
        {
            OutError = TEXT("Science reaction fields are invalid.");
            return false;
        }
        Reaction.ReactionId = FName(*ReactionId);
        Reaction.EvidenceEventId = FName(*EvidenceEventId);
        if (!Reaction.IsSane() || Candidate.Reactions.Contains(Reaction.ReactionId))
        {
            OutError = TEXT("Science reaction definition failed semantic validation.");
            return false;
        }
        Candidate.Reactions.Add(Reaction.ReactionId, MoveTemp(Reaction));
    }

    for (const TSharedPtr<FJsonValue>& Value : *PlantValues)
    {
        const TSharedPtr<FJsonObject>* Object = nullptr;
        if (!Value.IsValid() || !Value->TryGetObject(Object) || !Object || !Object->IsValid())
        {
            OutError = TEXT("Plant species entry is invalid.");
            return false;
        }

        FString SpeciesId;
        double MinTemp = 0.0;
        double OptimalTemp = 0.0;
        double MaxTemp = 0.0;
        double GerminationHours = 0.0;
        double GrowthRate = 0.0;
        double FloweringBiomass = 0.0;
        double FruitingBiomass = 0.0;
        if (!(*Object)->TryGetStringField(TEXT("speciesId"), SpeciesId) || SpeciesId.IsEmpty() ||
            !(*Object)->TryGetNumberField(TEXT("minTemperatureC"), MinTemp) ||
            !(*Object)->TryGetNumberField(TEXT("optimalTemperatureC"), OptimalTemp) ||
            !(*Object)->TryGetNumberField(TEXT("maxTemperatureC"), MaxTemp) ||
            !(*Object)->TryGetNumberField(TEXT("germinationHoursAtIdeal"), GerminationHours) ||
            !(*Object)->TryGetNumberField(TEXT("baseGrowthUnitsPerHour"), GrowthRate) ||
            !(*Object)->TryGetNumberField(TEXT("floweringBiomass"), FloweringBiomass) ||
            !(*Object)->TryGetNumberField(TEXT("fruitingBiomass"), FruitingBiomass))
        {
            OutError = TEXT("Plant species fields are invalid.");
            return false;
        }

        FWMPlantSpeciesDefinition Species;
        Species.SpeciesId = FName(*SpeciesId);
        Species.MinTemperatureC = static_cast<float>(MinTemp);
        Species.OptimalTemperatureC = static_cast<float>(OptimalTemp);
        Species.MaxTemperatureC = static_cast<float>(MaxTemp);
        Species.GerminationHoursAtIdeal = static_cast<float>(GerminationHours);
        Species.BaseGrowthUnitsPerHour = static_cast<float>(GrowthRate);
        Species.FloweringBiomass = static_cast<float>(FloweringBiomass);
        Species.FruitingBiomass = static_cast<float>(FruitingBiomass);
        if (!Species.IsSane() || Candidate.PlantSpecies.Contains(Species.SpeciesId))
        {
            OutError = TEXT("Plant species definition failed semantic validation.");
            return false;
        }
        Candidate.PlantSpecies.Add(Species.SpeciesId, MoveTemp(Species));
    }

    if (!Candidate.IsSane())
    {
        OutError = TEXT("Science simulation catalog failed semantic validation.");
        return false;
    }

    OutCatalog = MoveTemp(Candidate);
    OutError.Reset();
    return true;
}

bool FWMPhysicsBodyState::IsSane() const
{
    return FMath::IsFinite(MassKg) && MassKg > 0.0f &&
        PositionMeters.ContainsNaN() == false && VelocityMetersPerSecond.ContainsNaN() == false;
}

bool FWMPhysicsBodyState::StepConstantForce(const FVector& ForceNewtons, const float DeltaSeconds)
{
    if (!IsSane() || ForceNewtons.ContainsNaN() || !FMath::IsFinite(DeltaSeconds) || DeltaSeconds <= 0.0f || DeltaSeconds > 60.0f)
    {
        return false;
    }

    const FVector Acceleration = ForceNewtons / MassKg;
    PositionMeters += VelocityMetersPerSecond * DeltaSeconds + 0.5 * Acceleration * DeltaSeconds * DeltaSeconds;
    VelocityMetersPerSecond += Acceleration * DeltaSeconds;
    return !PositionMeters.ContainsNaN() && !VelocityMetersPerSecond.ContainsNaN();
}

FVector FWMPhysicsBodyState::GetMomentumKgMetersPerSecond() const
{
    return VelocityMetersPerSecond * MassKg;
}

float FWMPhysicsBodyState::GetKineticEnergyJoules() const
{
    return 0.5f * MassKg * VelocityMetersPerSecond.SizeSquared();
}

bool FWMDirectCurrentCircuit::Solve(float& OutCurrentAmps, float& OutPowerWatts) const
{
    OutCurrentAmps = 0.0f;
    OutPowerWatts = 0.0f;
    if (!FMath::IsFinite(VoltageVolts) || !FMath::IsFinite(ResistanceOhms) || ResistanceOhms <= 0.0f || FMath::Abs(VoltageVolts) > 10000.0f)
    {
        return false;
    }
    OutCurrentAmps = VoltageVolts / ResistanceOhms;
    OutPowerWatts = VoltageVolts * OutCurrentAmps;
    return FMath::IsFinite(OutCurrentAmps) && FMath::IsFinite(OutPowerWatts);
}

bool FWMPlantEnvironmentInput::IsSane() const
{
    return IsUnitInterval(WaterAvailability) && IsUnitInterval(NutrientAvailability) &&
        IsUnitInterval(LightAvailability) && IsUnitInterval(PollinatorAvailability) && FMath::IsFinite(TemperatureC);
}

bool FWMPlantState::IsSane() const
{
    return FMath::IsFinite(AgeHours) && AgeHours >= 0.0f &&
        FMath::IsFinite(GerminationProgressHours) && GerminationProgressHours >= 0.0f &&
        FMath::IsFinite(BiomassUnits) && BiomassUnits >= 0.0f &&
        FMath::IsFinite(RootDevelopmentUnits) && RootDevelopmentUnits >= 0.0f &&
        FMath::IsFinite(LeafDevelopmentUnits) && LeafDevelopmentUnits >= 0.0f;
}

bool FWMPlantState::Step(
    const FWMPlantSpeciesDefinition& Species,
    const FWMPlantEnvironmentInput& Environment,
    const float DeltaHours,
    FWMPlantStepResult& OutResult)
{
    OutResult = FWMPlantStepResult();
    if (!IsSane() || !Species.IsSane() || !Environment.IsSane() || !FMath::IsFinite(DeltaHours) || DeltaHours <= 0.0f || DeltaHours > 168.0f)
    {
        return false;
    }

    OutResult.PreviousStage = Stage;
    AgeHours += DeltaHours;
    const float TemperatureSuitability = FWMScienceSimulation::ResolveTemperatureSuitability(Species, Environment.TemperatureC);

    if (Stage == EWMPlantStage::Seed || Stage == EWMPlantStage::Germinating)
    {
        const float GerminationFactor = FMath::Min(Environment.WaterAvailability, TemperatureSuitability);
        GerminationProgressHours += DeltaHours * GerminationFactor;
        Stage = GerminationProgressHours > 0.0f ? EWMPlantStage::Germinating : EWMPlantStage::Seed;
        if (GerminationProgressHours >= Species.GerminationHoursAtIdeal)
        {
            Stage = EWMPlantStage::Seedling;
            RootDevelopmentUnits = FMath::Max(RootDevelopmentUnits, 0.05f);
            LeafDevelopmentUnits = FMath::Max(LeafDevelopmentUnits, 0.05f);
            OutResult.EvidenceEventId = TEXT("science.botany.germination-demonstrated");
        }
    }
    else
    {
        const float LimitingFactor = FMath::Min(
            FMath::Min(Environment.WaterAvailability, Environment.NutrientAvailability),
            FMath::Min(Environment.LightAvailability, TemperatureSuitability));
        const float Growth = Species.BaseGrowthUnitsPerHour * DeltaHours * LimitingFactor;
        BiomassUnits += Growth;
        RootDevelopmentUnits += Growth * 0.45f;
        LeafDevelopmentUnits += Growth * 0.55f;

        OutResult.GrowthUnits = Growth;
        OutResult.WaterDemandUnits = Growth * 0.40f;
        OutResult.NutrientDemandUnits = Growth * 0.25f;
        OutResult.PhotosynthesisProxy = Growth * Environment.LightAvailability;
        OutResult.RootProtectionProxy = Growth * 0.45f;

        if (BiomassUnits >= Species.FruitingBiomass && Environment.PollinatorAvailability >= 0.5f)
        {
            Stage = EWMPlantStage::Fruiting;
            OutResult.EvidenceEventId = TEXT("science.botany.pollination-fruiting-link-demonstrated");
        }
        else if (BiomassUnits >= Species.FloweringBiomass)
        {
            Stage = EWMPlantStage::Flowering;
            OutResult.EvidenceEventId = TEXT("science.botany.flowering-demonstrated");
        }
        else if (BiomassUnits >= 0.25f)
        {
            Stage = EWMPlantStage::Vegetative;
            OutResult.EvidenceEventId = TEXT("science.botany.growth-limiting-factor-demonstrated");
        }
        else
        {
            Stage = EWMPlantStage::Seedling;
            OutResult.EvidenceEventId = TEXT("science.botany.seedling-growth-demonstrated");
        }
    }

    OutResult.NewStage = Stage;
    OutResult.bAccepted = IsSane();
    return OutResult.bAccepted;
}

bool FWMScienceSimulation::DissolveInWater(
    const FWMSubstanceDefinition& Substance,
    const float SoluteMassG,
    const float WaterVolumeMl,
    FWMDissolutionResult& OutResult)
{
    OutResult = FWMDissolutionResult();
    if (!Substance.IsSane() || !FMath::IsFinite(SoluteMassG) || SoluteMassG < 0.0f ||
        !FMath::IsFinite(WaterVolumeMl) || WaterVolumeMl <= 0.0f || WaterVolumeMl > 100000.0f)
    {
        return false;
    }

    const float CapacityG = Substance.SolubilityGPer100MlWater * (WaterVolumeMl / 100.0f);
    OutResult.DissolvedMassG = FMath::Min(SoluteMassG, CapacityG);
    OutResult.UndissolvedMassG = FMath::Max(0.0f, SoluteMassG - OutResult.DissolvedMassG);
    OutResult.bSaturated = SoluteMassG > CapacityG || FMath::IsNearlyEqual(SoluteMassG, CapacityG, 0.001f);
    OutResult.bAccepted = true;
    OutResult.EvidenceEventId = OutResult.bSaturated
        ? FName(TEXT("science.chemistry.solution-saturation-demonstrated"))
        : FName(TEXT("science.chemistry.solubility-demonstrated"));
    return true;
}

bool FWMScienceSimulation::ExecuteReaction(
    const FWMScienceSimulationCatalog& Catalog,
    const FWMReactionDefinition& Reaction,
    const TMap<FName, float>& InventoryMoles,
    FWMReactionResult& OutResult)
{
    OutResult = FWMReactionResult();
    if (!Catalog.IsSane() || !Reaction.IsSane() || InventoryMoles.IsEmpty())
    {
        return false;
    }

    const float ReactantStoichMass = CalculateStoichiometricMass(Catalog, Reaction.Reactants);
    const float ProductStoichMass = CalculateStoichiometricMass(Catalog, Reaction.Products);
    if (ReactantStoichMass <= 0.0f || ProductStoichMass <= 0.0f ||
        !FMath::IsNearlyEqual(ReactantStoichMass, ProductStoichMass, MassBalanceToleranceGPerStoichUnit))
    {
        return false;
    }

    float LimitingExtent = TNumericLimits<float>::Max();
    for (const FWMReactionComponent& Reactant : Reaction.Reactants)
    {
        const float* Available = InventoryMoles.Find(Reactant.SubstanceId);
        if (!Available || !FMath::IsFinite(*Available) || *Available < 0.0f)
        {
            return false;
        }
        LimitingExtent = FMath::Min(LimitingExtent, *Available / static_cast<float>(Reactant.StoichiometricCoefficient));
    }
    if (!FMath::IsFinite(LimitingExtent) || LimitingExtent <= KINDA_SMALL_NUMBER)
    {
        return false;
    }

    OutResult.BeforeMoles = InventoryMoles;
    OutResult.AfterMoles = InventoryMoles;
    OutResult.ReactionExtentMol = LimitingExtent;

    for (const FWMReactionComponent& Reactant : Reaction.Reactants)
    {
        float& Remaining = OutResult.AfterMoles.FindOrAdd(Reactant.SubstanceId);
        Remaining = FMath::Max(0.0f, Remaining - LimitingExtent * static_cast<float>(Reactant.StoichiometricCoefficient));
    }
    for (const FWMReactionComponent& Product : Reaction.Products)
    {
        float& Produced = OutResult.AfterMoles.FindOrAdd(Product.SubstanceId);
        Produced += LimitingExtent * static_cast<float>(Product.StoichiometricCoefficient);
    }

    OutResult.ReactantMassConsumedG = ReactantStoichMass * LimitingExtent;
    OutResult.ProductMassCreatedG = ProductStoichMass * LimitingExtent;
    OutResult.EvidenceEventId = Reaction.EvidenceEventId;
    OutResult.bAccepted = FMath::IsNearlyEqual(OutResult.ReactantMassConsumedG, OutResult.ProductMassCreatedG, MassBalanceToleranceGPerStoichUnit);
    return OutResult.bAccepted;
}

float FWMScienceSimulation::ResolveTemperatureSuitability(const FWMPlantSpeciesDefinition& Species, const float TemperatureC)
{
    if (!Species.IsSane() || !FMath::IsFinite(TemperatureC) || TemperatureC <= Species.MinTemperatureC || TemperatureC >= Species.MaxTemperatureC)
    {
        return 0.0f;
    }
    if (FMath::IsNearlyEqual(TemperatureC, Species.OptimalTemperatureC)) return 1.0f;
    if (TemperatureC < Species.OptimalTemperatureC)
    {
        return FMath::Clamp((TemperatureC - Species.MinTemperatureC) / (Species.OptimalTemperatureC - Species.MinTemperatureC), 0.0f, 1.0f);
    }
    return FMath::Clamp((Species.MaxTemperatureC - TemperatureC) / (Species.MaxTemperatureC - Species.OptimalTemperatureC), 0.0f, 1.0f);
}

FWMEnvironmentStateDelta FWMScienceSimulation::BuildPlantEnvironmentDelta(const FWMPlantStepResult& PlantResult)
{
    FWMEnvironmentStateDelta Delta;
    if (!PlantResult.bAccepted) return Delta;

    Delta.VegetationHealth = FMath::Clamp(PlantResult.GrowthUnits * 0.05f, 0.0f, 0.08f);
    Delta.SoilProtection = FMath::Clamp(PlantResult.RootProtectionProxy * 0.04f, 0.0f, 0.06f);
    Delta.ShadeCoverage = FMath::Clamp(PlantResult.PhotosynthesisProxy * 0.02f, 0.0f, 0.04f);
    Delta.WaterFlow = 0.0f;
    return Delta;
}
