#include "Science/WMScienceSimulationCore.h"

namespace
{
    bool IsUnitIntervalValue(const float Value)
    {
        return FMath::IsFinite(Value) && Value >= 0.0f && Value <= 1.0f;
    }
}

bool FWMScienceSimulation::FilterDissolution(const FWMDissolutionResult& Dissolution, FWMFiltrationResult& OutResult)
{
    OutResult = FWMFiltrationResult();
    if (!Dissolution.bAccepted || !FMath::IsFinite(Dissolution.DissolvedMassG) ||
        !FMath::IsFinite(Dissolution.UndissolvedMassG) || Dissolution.DissolvedMassG < 0.0f ||
        Dissolution.UndissolvedMassG < 0.0f)
    {
        return false;
    }

    OutResult.bAccepted = true;
    OutResult.RetainedSolidMassG = Dissolution.UndissolvedMassG;
    OutResult.DissolvedMassRemainingG = Dissolution.DissolvedMassG;
    OutResult.EvidenceEventId = TEXT("science.chemistry.filtration-boundary-demonstrated");
    return true;
}

bool FWMCellEnvironmentInput::IsSane() const
{
    return IsUnitIntervalValue(NutrientAvailability) && IsUnitIntervalValue(OxygenAvailability) &&
        IsUnitIntervalValue(TemperatureSuitability);
}

bool FWMCellSystemState::IsSane() const
{
    return IsUnitIntervalValue(MembraneIntegrity) && IsUnitIntervalValue(EnergyAvailability) &&
        IsUnitIntervalValue(TransportEfficiency) && IsUnitIntervalValue(InformationIntegrity) &&
        IsUnitIntervalValue(WasteLoad);
}

bool FWMCellSystemState::StepMetabolism(
    const FWMCellEnvironmentInput& Environment,
    const float DeltaHours,
    FWMCellStepResult& OutResult)
{
    OutResult = FWMCellStepResult();
    if (!IsSane() || !Environment.IsSane() || !FMath::IsFinite(DeltaHours) || DeltaHours <= 0.0f || DeltaHours > 24.0f)
    {
        return false;
    }

    const float ResourceFactor = FMath::Min(Environment.NutrientAvailability, Environment.OxygenAvailability);
    const float FunctionalFactor = FMath::Min(MembraneIntegrity, FMath::Min(TransportEfficiency, InformationIntegrity));
    const float MetabolicFactor = FMath::Min(ResourceFactor, FMath::Min(Environment.TemperatureSuitability, FunctionalFactor));

    const float EnergyProduced = FMath::Clamp(0.10f * DeltaHours * MetabolicFactor, 0.0f, 0.35f);
    const float TransportWork = FMath::Clamp(0.08f * DeltaHours * EnergyAvailability * MembraneIntegrity, 0.0f, 0.30f);
    const float WasteProduced = FMath::Clamp(0.04f * DeltaHours * MetabolicFactor, 0.0f, 0.20f);
    const float WasteCleared = FMath::Clamp(0.03f * DeltaHours * TransportEfficiency * MembraneIntegrity, 0.0f, 0.20f);

    EnergyAvailability = FMath::Clamp(EnergyAvailability + EnergyProduced - (TransportWork * 0.5f), 0.0f, 1.0f);
    WasteLoad = FMath::Clamp(WasteLoad + WasteProduced - WasteCleared, 0.0f, 1.0f);

    OutResult.bAccepted = IsSane();
    OutResult.EnergyProducedUnits = EnergyProduced;
    OutResult.WasteProducedUnits = WasteProduced;
    OutResult.TransportWorkUnits = TransportWork;
    OutResult.EvidenceEventId = MetabolicFactor > 0.5f
        ? FName(TEXT("science.biology.cell-system-interdependence-demonstrated"))
        : FName(TEXT("science.biology.cell-limiting-factor-demonstrated"));
    return OutResult.bAccepted;
}
