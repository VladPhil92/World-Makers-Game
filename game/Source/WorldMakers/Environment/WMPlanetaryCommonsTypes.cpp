#include "Environment/WMPlanetaryCommonsTypes.h"

#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
    bool IsUnit(const float Value)
    {
        return FMath::IsFinite(Value) && Value >= 0.0f && Value <= 1.0f;
    }

    bool IsDelta(const float Value)
    {
        return FMath::IsFinite(Value) && FMath::Abs(Value) <= 1.0f;
    }

    float ClampUnit(const float Value)
    {
        return FMath::Clamp(Value, 0.0f, 1.0f);
    }

    bool ReadNumber(const TSharedPtr<FJsonObject>& Object, const TCHAR* Key, float& OutValue)
    {
        double Value = 0.0;
        if (!Object.IsValid() || !Object->TryGetNumberField(Key, Value))
        {
            return false;
        }
        OutValue = static_cast<float>(Value);
        return FMath::IsFinite(OutValue);
    }

    void ReadOptionalDelta(const TSharedPtr<FJsonObject>& Object, const TCHAR* Key, float& OutValue)
    {
        double Value = 0.0;
        if (Object.IsValid() && Object->TryGetNumberField(Key, Value))
        {
            OutValue = static_cast<float>(Value);
        }
    }
}

bool FWMPlanetaryCommonsDelta::IsSane() const
{
    return IsDelta(WaterQuality)
        && IsDelta(AirQuality)
        && IsDelta(SoilHealth)
        && IsDelta(Biodiversity)
        && IsDelta(EnergyReliability)
        && IsDelta(MaterialDemand)
        && IsDelta(ComputeEnergyDemand)
        && IsDelta(HumanWellbeing)
        && IsDelta(CivicLegitimacy);
}

bool FWMHabitatLinkDefinition::IsSane() const
{
    return !LinkId.IsNone() && IsUnit(InitialPermeability);
}

bool FWMSpeciesNeedDefinition::IsSane() const
{
    return !SpeciesId.IsNone()
        && !ScientificReviewState.IsNone()
        && IsUnit(MinWaterQuality)
        && IsUnit(MinAirQuality)
        && IsUnit(MinSoilHealth)
        && IsUnit(MinBiodiversity)
        && IsUnit(MinHabitatConnectivity)
        && IsUnit(DisturbanceSensitivity);
}

bool FWMSpeciesStateSnapshot::IsBounded() const
{
    return !SpeciesId.IsNone() && IsUnit(Stress) && IsUnit(Flourishing) && !StressBandId.IsNone();
}

bool FWMPlanetaryCommonsSnapshot::IsBounded() const
{
    return !SimulationId.IsNone()
        && StepIndex >= 0
        && IsUnit(WaterQuality)
        && IsUnit(AirQuality)
        && IsUnit(SoilHealth)
        && IsUnit(Biodiversity)
        && IsUnit(HabitatConnectivity)
        && IsUnit(AnimalStress)
        && IsUnit(EnergyReliability)
        && IsUnit(MaterialDemand)
        && IsUnit(ComputeEnergyDemand)
        && IsUnit(HumanWellbeing)
        && IsUnit(CivicLegitimacy);
}

float FWMPlanetaryCommonsSnapshot::GetHookValue(const FName HookId) const
{
    if (HookId == TEXT("water-quality")) return WaterQuality;
    if (HookId == TEXT("air-quality")) return AirQuality;
    if (HookId == TEXT("soil-health")) return SoilHealth;
    if (HookId == TEXT("biodiversity")) return Biodiversity;
    if (HookId == TEXT("habitat-connectivity")) return HabitatConnectivity;
    if (HookId == TEXT("animal-stress")) return AnimalStress;
    if (HookId == TEXT("energy-reliability")) return EnergyReliability;
    if (HookId == TEXT("material-demand")) return MaterialDemand;
    if (HookId == TEXT("compute-energy-demand")) return ComputeEnergyDemand;
    if (HookId == TEXT("human-wellbeing")) return HumanWellbeing;
    if (HookId == TEXT("civic-legitimacy")) return CivicLegitimacy;
    return -1.0f;
}

bool FWMCommonsInterventionDefinition::IsSane() const
{
    if (InterventionId.IsNone() || MaxApplications < 1 || !Delta.IsSane())
    {
        return false;
    }

    bool bHasEffect = !FMath::IsNearlyZero(Delta.WaterQuality)
        || !FMath::IsNearlyZero(Delta.AirQuality)
        || !FMath::IsNearlyZero(Delta.SoilHealth)
        || !FMath::IsNearlyZero(Delta.Biodiversity)
        || !FMath::IsNearlyZero(Delta.EnergyReliability)
        || !FMath::IsNearlyZero(Delta.MaterialDemand)
        || !FMath::IsNearlyZero(Delta.ComputeEnergyDemand)
        || !FMath::IsNearlyZero(Delta.HumanWellbeing)
        || !FMath::IsNearlyZero(Delta.CivicLegitimacy);

    for (const TPair<FName, float>& LinkDelta : HabitatLinkDeltas)
    {
        if (LinkDelta.Key.IsNone() || !IsDelta(LinkDelta.Value))
        {
            return false;
        }
        bHasEffect |= !FMath::IsNearlyZero(LinkDelta.Value);
    }
    return bHasEffect;
}

const FWMCommonsInterventionDefinition* FWMPlanetaryCommonsDefinition::FindIntervention(const FName InterventionId) const
{
    return Interventions.FindByPredicate([InterventionId](const FWMCommonsInterventionDefinition& Item)
    {
        return Item.InterventionId == InterventionId;
    });
}

bool FWMPlanetaryCommonsDefinition::IsSane() const
{
    if (SchemaVersion != 1 || SimulationId.IsNone() || !bPrototypeOnly || !bDeterministicStepModel)
    {
        return false;
    }

    const float InitialValues[] = {
        InitialWaterQuality, InitialAirQuality, InitialSoilHealth, InitialBiodiversity,
        InitialEnergyReliability, InitialMaterialDemand, InitialComputeEnergyDemand,
        InitialHumanWellbeing, InitialCivicLegitimacy
    };
    for (const float Value : InitialValues)
    {
        if (!IsUnit(Value)) return false;
    }

    const float Rates[] = {
        ErosionToWaterRate, EcologyRelaxationRate, WellbeingRelaxationRate,
        MaterialSoilPressureRate, ComputeReliabilityPressureRate
    };
    for (const float Rate : Rates)
    {
        if (!IsUnit(Rate)) return false;
    }

    if (HabitatLinks.IsEmpty() || SpeciesProfiles.IsEmpty() || Interventions.IsEmpty())
    {
        return false;
    }

    TSet<FName> SeenLinks;
    for (const FWMHabitatLinkDefinition& Link : HabitatLinks)
    {
        if (!Link.IsSane() || SeenLinks.Contains(Link.LinkId)) return false;
        SeenLinks.Add(Link.LinkId);
    }

    TSet<FName> SeenSpecies;
    for (const FWMSpeciesNeedDefinition& Species : SpeciesProfiles)
    {
        if (!Species.IsSane() || SeenSpecies.Contains(Species.SpeciesId)) return false;
        SeenSpecies.Add(Species.SpeciesId);
    }

    TSet<FName> SeenInterventions;
    for (const FWMCommonsInterventionDefinition& Intervention : Interventions)
    {
        if (!Intervention.IsSane() || SeenInterventions.Contains(Intervention.InterventionId)) return false;
        for (const TPair<FName, float>& LinkDelta : Intervention.HabitatLinkDeltas)
        {
            if (!SeenLinks.Contains(LinkDelta.Key)) return false;
        }
        SeenInterventions.Add(Intervention.InterventionId);
    }
    return true;
}

bool FWMPlanetaryCommonsDefinition::TryParseJson(const FString& Json, FWMPlanetaryCommonsDefinition& OutDefinition, FString& OutError)
{
    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        OutError = TEXT("Invalid planetary commons JSON");
        return false;
    }

    FWMPlanetaryCommonsDefinition Parsed;
    Parsed.SchemaVersion = Root->GetIntegerField(TEXT("schemaVersion"));
    Parsed.SimulationId = FName(*Root->GetStringField(TEXT("simulationId")));
    Parsed.bPrototypeOnly = Root->GetBoolField(TEXT("prototypeOnly"));
    Parsed.bDeterministicStepModel = Root->GetBoolField(TEXT("deterministicStepModel"));

    const TSharedPtr<FJsonObject>* InitialPtr = nullptr;
    if (!Root->TryGetObjectField(TEXT("initialState"), InitialPtr) || !InitialPtr || !InitialPtr->IsValid())
    {
        OutError = TEXT("Missing initialState");
        return false;
    }
    const TSharedPtr<FJsonObject>& Initial = *InitialPtr;
    if (!ReadNumber(Initial, TEXT("waterQuality"), Parsed.InitialWaterQuality)
        || !ReadNumber(Initial, TEXT("airQuality"), Parsed.InitialAirQuality)
        || !ReadNumber(Initial, TEXT("soilHealth"), Parsed.InitialSoilHealth)
        || !ReadNumber(Initial, TEXT("biodiversity"), Parsed.InitialBiodiversity)
        || !ReadNumber(Initial, TEXT("energyReliability"), Parsed.InitialEnergyReliability)
        || !ReadNumber(Initial, TEXT("materialDemand"), Parsed.InitialMaterialDemand)
        || !ReadNumber(Initial, TEXT("computeEnergyDemand"), Parsed.InitialComputeEnergyDemand)
        || !ReadNumber(Initial, TEXT("humanWellbeing"), Parsed.InitialHumanWellbeing)
        || !ReadNumber(Initial, TEXT("civicLegitimacy"), Parsed.InitialCivicLegitimacy))
    {
        OutError = TEXT("initialState is incomplete");
        return false;
    }

    const TSharedPtr<FJsonObject>* CouplingPtr = nullptr;
    if (!Root->TryGetObjectField(TEXT("coupling"), CouplingPtr) || !CouplingPtr || !CouplingPtr->IsValid())
    {
        OutError = TEXT("Missing coupling");
        return false;
    }
    const TSharedPtr<FJsonObject>& Coupling = *CouplingPtr;
    if (!ReadNumber(Coupling, TEXT("erosionToWaterRate"), Parsed.ErosionToWaterRate)
        || !ReadNumber(Coupling, TEXT("ecologyRelaxationRate"), Parsed.EcologyRelaxationRate)
        || !ReadNumber(Coupling, TEXT("wellbeingRelaxationRate"), Parsed.WellbeingRelaxationRate)
        || !ReadNumber(Coupling, TEXT("materialSoilPressureRate"), Parsed.MaterialSoilPressureRate)
        || !ReadNumber(Coupling, TEXT("computeReliabilityPressureRate"), Parsed.ComputeReliabilityPressureRate))
    {
        OutError = TEXT("coupling is incomplete");
        return false;
    }

    const TArray<TSharedPtr<FJsonValue>>* LinkValues = nullptr;
    if (!Root->TryGetArrayField(TEXT("habitatLinks"), LinkValues) || !LinkValues)
    {
        OutError = TEXT("Missing habitatLinks");
        return false;
    }
    for (const TSharedPtr<FJsonValue>& Value : *LinkValues)
    {
        const TSharedPtr<FJsonObject> Object = Value->AsObject();
        FWMHabitatLinkDefinition Link;
        Link.LinkId = FName(*Object->GetStringField(TEXT("linkId")));
        if (!ReadNumber(Object, TEXT("initialPermeability"), Link.InitialPermeability))
        {
            OutError = TEXT("Invalid habitat link");
            return false;
        }
        Parsed.HabitatLinks.Add(Link);
    }

    const TArray<TSharedPtr<FJsonValue>>* SpeciesValues = nullptr;
    if (!Root->TryGetArrayField(TEXT("speciesProfiles"), SpeciesValues) || !SpeciesValues)
    {
        OutError = TEXT("Missing speciesProfiles");
        return false;
    }
    for (const TSharedPtr<FJsonValue>& Value : *SpeciesValues)
    {
        const TSharedPtr<FJsonObject> Object = Value->AsObject();
        FWMSpeciesNeedDefinition Species;
        Species.SpeciesId = FName(*Object->GetStringField(TEXT("speciesId")));
        Species.ScientificReviewState = FName(*Object->GetStringField(TEXT("scientificReviewState")));
        if (!ReadNumber(Object, TEXT("minWaterQuality"), Species.MinWaterQuality)
            || !ReadNumber(Object, TEXT("minAirQuality"), Species.MinAirQuality)
            || !ReadNumber(Object, TEXT("minSoilHealth"), Species.MinSoilHealth)
            || !ReadNumber(Object, TEXT("minBiodiversity"), Species.MinBiodiversity)
            || !ReadNumber(Object, TEXT("minHabitatConnectivity"), Species.MinHabitatConnectivity)
            || !ReadNumber(Object, TEXT("disturbanceSensitivity"), Species.DisturbanceSensitivity))
        {
            OutError = TEXT("Invalid species profile");
            return false;
        }
        Parsed.SpeciesProfiles.Add(Species);
    }

    const TArray<TSharedPtr<FJsonValue>>* InterventionValues = nullptr;
    if (!Root->TryGetArrayField(TEXT("interventions"), InterventionValues) || !InterventionValues)
    {
        OutError = TEXT("Missing interventions");
        return false;
    }
    for (const TSharedPtr<FJsonValue>& Value : *InterventionValues)
    {
        const TSharedPtr<FJsonObject> Object = Value->AsObject();
        FWMCommonsInterventionDefinition Intervention;
        Intervention.InterventionId = FName(*Object->GetStringField(TEXT("interventionId")));
        Intervention.MaxApplications = Object->GetIntegerField(TEXT("maxApplications"));

        const TSharedPtr<FJsonObject>* DeltaPtr = nullptr;
        if (Object->TryGetObjectField(TEXT("delta"), DeltaPtr) && DeltaPtr && DeltaPtr->IsValid())
        {
            const TSharedPtr<FJsonObject>& Delta = *DeltaPtr;
            ReadOptionalDelta(Delta, TEXT("waterQuality"), Intervention.Delta.WaterQuality);
            ReadOptionalDelta(Delta, TEXT("airQuality"), Intervention.Delta.AirQuality);
            ReadOptionalDelta(Delta, TEXT("soilHealth"), Intervention.Delta.SoilHealth);
            ReadOptionalDelta(Delta, TEXT("biodiversity"), Intervention.Delta.Biodiversity);
            ReadOptionalDelta(Delta, TEXT("energyReliability"), Intervention.Delta.EnergyReliability);
            ReadOptionalDelta(Delta, TEXT("materialDemand"), Intervention.Delta.MaterialDemand);
            ReadOptionalDelta(Delta, TEXT("computeEnergyDemand"), Intervention.Delta.ComputeEnergyDemand);
            ReadOptionalDelta(Delta, TEXT("humanWellbeing"), Intervention.Delta.HumanWellbeing);
            ReadOptionalDelta(Delta, TEXT("civicLegitimacy"), Intervention.Delta.CivicLegitimacy);
        }

        const TSharedPtr<FJsonObject>* LinkDeltaPtr = nullptr;
        if (Object->TryGetObjectField(TEXT("habitatLinkDeltas"), LinkDeltaPtr) && LinkDeltaPtr && LinkDeltaPtr->IsValid())
        {
            for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : (*LinkDeltaPtr)->Values)
            {
                Intervention.HabitatLinkDeltas.Add(FName(*Pair.Key), static_cast<float>(Pair.Value->AsNumber()));
            }
        }
        Parsed.Interventions.Add(Intervention);
    }

    if (!Parsed.IsSane())
    {
        OutError = TEXT("Planetary commons definition failed sanity checks");
        return false;
    }

    OutDefinition = MoveTemp(Parsed);
    OutError.Reset();
    return true;
}

bool FWMPlanetaryCommonsModel::Initialize(const FWMPlanetaryCommonsDefinition& InDefinition)
{
    if (!InDefinition.IsSane()) return false;

    Definition = InDefinition;
    Snapshot = FWMPlanetaryCommonsSnapshot();
    Snapshot.SimulationId = Definition.SimulationId;
    Snapshot.WaterQuality = Definition.InitialWaterQuality;
    Snapshot.AirQuality = Definition.InitialAirQuality;
    Snapshot.SoilHealth = Definition.InitialSoilHealth;
    Snapshot.Biodiversity = Definition.InitialBiodiversity;
    Snapshot.EnergyReliability = Definition.InitialEnergyReliability;
    Snapshot.MaterialDemand = Definition.InitialMaterialDemand;
    Snapshot.ComputeEnergyDemand = Definition.InitialComputeEnergyDemand;
    Snapshot.HumanWellbeing = Definition.InitialHumanWellbeing;
    Snapshot.CivicLegitimacy = Definition.InitialCivicLegitimacy;

    HabitatLinkPermeability.Reset();
    for (const FWMHabitatLinkDefinition& Link : Definition.HabitatLinks)
    {
        HabitatLinkPermeability.Add(Link.LinkId, Link.InitialPermeability);
    }
    ApplicationCounts.Reset();
    SpeciesStates.Reset();
    bInitialized = true;
    RefreshDerivedState();
    return Snapshot.IsBounded();
}

bool FWMPlanetaryCommonsModel::SeedFromLegacyEcosystem(const FWMEnvironmentStateSnapshot& LegacySnapshot)
{
    if (!bInitialized || !LegacySnapshot.IsBounded()) return false;

    Snapshot.WaterQuality = LegacySnapshot.WaterFlow;
    Snapshot.SoilHealth = LegacySnapshot.SoilProtection;
    Snapshot.Biodiversity = ClampUnit((LegacySnapshot.VegetationHealth + LegacySnapshot.HabitatQuality) * 0.5f);
    ClampPrimaryState();
    RefreshDerivedState();
    return true;
}

bool FWMPlanetaryCommonsModel::ApplyLegacyProjectionDelta(
    const FWMEnvironmentStateSnapshot& Previous,
    const FWMEnvironmentStateSnapshot& Current)
{
    if (!bInitialized || !Previous.IsBounded() || !Current.IsBounded()) return false;

    Snapshot.WaterQuality += Current.WaterFlow - Previous.WaterFlow;
    Snapshot.SoilHealth += Current.SoilProtection - Previous.SoilProtection;
    Snapshot.Biodiversity += 0.5f * (Current.VegetationHealth - Previous.VegetationHealth)
        + 0.5f * (Current.HabitatQuality - Previous.HabitatQuality);
    ClampPrimaryState();
    RefreshDerivedState();
    return true;
}

bool FWMPlanetaryCommonsModel::CanApplyIntervention(const FName InterventionId) const
{
    if (!bInitialized) return false;
    const FWMCommonsInterventionDefinition* Intervention = Definition.FindIntervention(InterventionId);
    return Intervention && GetAppliedCount(InterventionId) < Intervention->MaxApplications;
}

bool FWMPlanetaryCommonsModel::ApplyIntervention(const FName InterventionId)
{
    if (!CanApplyIntervention(InterventionId)) return false;
    const FWMCommonsInterventionDefinition* Intervention = Definition.FindIntervention(InterventionId);
    if (!Intervention) return false;

    Snapshot.WaterQuality += Intervention->Delta.WaterQuality;
    Snapshot.AirQuality += Intervention->Delta.AirQuality;
    Snapshot.SoilHealth += Intervention->Delta.SoilHealth;
    Snapshot.Biodiversity += Intervention->Delta.Biodiversity;
    Snapshot.EnergyReliability += Intervention->Delta.EnergyReliability;
    Snapshot.MaterialDemand += Intervention->Delta.MaterialDemand;
    Snapshot.ComputeEnergyDemand += Intervention->Delta.ComputeEnergyDemand;
    Snapshot.HumanWellbeing += Intervention->Delta.HumanWellbeing;
    Snapshot.CivicLegitimacy += Intervention->Delta.CivicLegitimacy;

    for (const TPair<FName, float>& LinkDelta : Intervention->HabitatLinkDeltas)
    {
        float* Existing = HabitatLinkPermeability.Find(LinkDelta.Key);
        if (!Existing) return false;
        *Existing = ClampUnit(*Existing + LinkDelta.Value);
    }

    ApplicationCounts.FindOrAdd(InterventionId) += 1;
    ClampPrimaryState();
    RefreshDerivedState();
    return Snapshot.IsBounded();
}

bool FWMPlanetaryCommonsModel::AdvanceStep()
{
    if (!bInitialized) return false;

    Snapshot.HabitatConnectivity = DeriveHabitatConnectivity();

    Snapshot.SoilHealth -= Snapshot.MaterialDemand * Definition.MaterialSoilPressureRate;
    const float ErosionDeficit = FMath::Max(0.0f, 0.60f - Snapshot.SoilHealth);
    Snapshot.WaterQuality -= ErosionDeficit * Definition.ErosionToWaterRate;
    Snapshot.EnergyReliability -= Snapshot.ComputeEnergyDemand * Definition.ComputeReliabilityPressureRate;

    const float EcologicalTarget = (Snapshot.WaterQuality + Snapshot.AirQuality + Snapshot.SoilHealth + Snapshot.HabitatConnectivity) * 0.25f;
    Snapshot.Biodiversity = FMath::Lerp(Snapshot.Biodiversity, EcologicalTarget, Definition.EcologyRelaxationRate);

    const float WellbeingTarget = (Snapshot.WaterQuality + Snapshot.AirQuality + Snapshot.EnergyReliability + Snapshot.Biodiversity) * 0.25f;
    Snapshot.HumanWellbeing = FMath::Lerp(Snapshot.HumanWellbeing, WellbeingTarget, Definition.WellbeingRelaxationRate);

    ++Snapshot.StepIndex;
    ClampPrimaryState();
    RefreshDerivedState();
    return Snapshot.IsBounded();
}

float FWMPlanetaryCommonsModel::GetHabitatLinkPermeability(const FName LinkId) const
{
    if (const float* Value = HabitatLinkPermeability.Find(LinkId)) return *Value;
    return -1.0f;
}

int32 FWMPlanetaryCommonsModel::GetAppliedCount(const FName InterventionId) const
{
    if (const int32* Count = ApplicationCounts.Find(InterventionId)) return *Count;
    return 0;
}

void FWMPlanetaryCommonsModel::ClampPrimaryState()
{
    Snapshot.WaterQuality = ClampUnit(Snapshot.WaterQuality);
    Snapshot.AirQuality = ClampUnit(Snapshot.AirQuality);
    Snapshot.SoilHealth = ClampUnit(Snapshot.SoilHealth);
    Snapshot.Biodiversity = ClampUnit(Snapshot.Biodiversity);
    Snapshot.EnergyReliability = ClampUnit(Snapshot.EnergyReliability);
    Snapshot.MaterialDemand = ClampUnit(Snapshot.MaterialDemand);
    Snapshot.ComputeEnergyDemand = ClampUnit(Snapshot.ComputeEnergyDemand);
    Snapshot.HumanWellbeing = ClampUnit(Snapshot.HumanWellbeing);
    Snapshot.CivicLegitimacy = ClampUnit(Snapshot.CivicLegitimacy);
}

float FWMPlanetaryCommonsModel::DeriveHabitatConnectivity() const
{
    if (HabitatLinkPermeability.IsEmpty()) return 0.0f;
    float Total = 0.0f;
    for (const TPair<FName, float>& Pair : HabitatLinkPermeability)
    {
        Total += ClampUnit(Pair.Value);
    }
    return ClampUnit(Total / static_cast<float>(HabitatLinkPermeability.Num()));
}

float FWMPlanetaryCommonsModel::DeriveSpeciesStress(const FWMSpeciesNeedDefinition& Species) const
{
    const float Deficits[] = {
        FMath::Max(0.0f, Species.MinWaterQuality - Snapshot.WaterQuality),
        FMath::Max(0.0f, Species.MinAirQuality - Snapshot.AirQuality),
        FMath::Max(0.0f, Species.MinSoilHealth - Snapshot.SoilHealth),
        FMath::Max(0.0f, Species.MinBiodiversity - Snapshot.Biodiversity),
        FMath::Max(0.0f, Species.MinHabitatConnectivity - Snapshot.HabitatConnectivity)
    };

    float DeficitMean = 0.0f;
    for (const float Deficit : Deficits) DeficitMean += Deficit;
    DeficitMean /= UE_ARRAY_COUNT(Deficits);

    const float Disturbance = ClampUnit((Snapshot.MaterialDemand + Snapshot.ComputeEnergyDemand) * 0.5f);
    return ClampUnit(DeficitMean * 1.5f + Disturbance * Species.DisturbanceSensitivity * 0.25f);
}

void FWMPlanetaryCommonsModel::RefreshDerivedState()
{
    Snapshot.HabitatConnectivity = DeriveHabitatConnectivity();
    SpeciesStates.Reset();

    float StressTotal = 0.0f;
    for (const FWMSpeciesNeedDefinition& Species : Definition.SpeciesProfiles)
    {
        FWMSpeciesStateSnapshot State;
        State.SpeciesId = Species.SpeciesId;
        State.Stress = DeriveSpeciesStress(Species);
        State.Flourishing = ClampUnit(1.0f - State.Stress);
        State.StressBandId = State.Stress < 0.25f
            ? FName(TEXT("species-state.flourishing"))
            : (State.Stress < 0.55f ? FName(TEXT("species-state.watchful")) : FName(TEXT("species-state.stressed")));
        SpeciesStates.Add(State);
        StressTotal += State.Stress;
    }

    Snapshot.AnimalStress = SpeciesStates.IsEmpty()
        ? 0.0f
        : ClampUnit(StressTotal / static_cast<float>(SpeciesStates.Num()));
}
