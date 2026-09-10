#include "Science/WMScienceVFXAdapter.h"

namespace
{
    FWMVFXEvent BaseEvent(const FName EventId, const FVector& LocationCm, const float Intensity, const float DurationSeconds)
    {
        FWMVFXEvent Event;
        Event.EventId = EventId;
        Event.LocationCm = LocationCm;
        Event.Direction = FVector::ForwardVector;
        Event.Intensity = FMath::Clamp(Intensity, 0.0f, 1.0f);
        Event.DurationSeconds = FMath::Clamp(DurationSeconds, 0.08f, 2.5f);
        Event.MotionScale = 1.0f;
        return Event;
    }
}

bool FWMScienceVFXAdapter::FromDissolution(const FWMDissolutionResult& Result, const FVector& LocationCm, FWMVFXEvent& OutEvent)
{
    if (!Result.bAccepted || !FMath::IsFinite(Result.DissolvedMassG) || !FMath::IsFinite(Result.UndissolvedMassG)) return false;
    const float TotalMass = FMath::Max(Result.DissolvedMassG + Result.UndissolvedMassG, KINDA_SMALL_NUMBER);
    const float FractionDissolved = FMath::Clamp(Result.DissolvedMassG / TotalMass, 0.0f, 1.0f);
    OutEvent = BaseEvent(Result.bSaturated ? TEXT("science.chemistry.saturation") : TEXT("science.chemistry.dissolution"), LocationCm, 0.25f + 0.75f * FractionDissolved, 0.85f);
    return OutEvent.IsSane();
}

bool FWMScienceVFXAdapter::FromFiltration(const FWMFiltrationResult& Result, const FVector& LocationCm, FWMVFXEvent& OutEvent)
{
    if (!Result.bAccepted || !FMath::IsFinite(Result.RetainedSolidMassG) || !FMath::IsFinite(Result.DissolvedMassRemainingG)) return false;
    const float Total = FMath::Max(Result.RetainedSolidMassG + Result.DissolvedMassRemainingG, KINDA_SMALL_NUMBER);
    const float RetainedFraction = FMath::Clamp(Result.RetainedSolidMassG / Total, 0.0f, 1.0f);
    OutEvent = BaseEvent(TEXT("science.chemistry.filtration"), LocationCm, 0.30f + 0.70f * RetainedFraction, 0.80f);
    OutEvent.Direction = FVector(0.0f, 0.0f, -1.0f);
    return OutEvent.IsSane();
}

bool FWMScienceVFXAdapter::FromReaction(const FWMReactionResult& Result, const FVector& LocationCm, FWMVFXEvent& OutEvent)
{
    if (!Result.bAccepted || !FMath::IsFinite(Result.ReactionExtentMol) || Result.ReactionExtentMol <= 0.0f) return false;
    const float Intensity = FMath::Clamp(0.25f + Result.ReactionExtentMol * 0.35f, 0.25f, 1.0f);
    OutEvent = BaseEvent(TEXT("science.chemistry.reaction"), LocationCm, Intensity, 0.95f);
    return OutEvent.IsSane();
}

bool FWMScienceVFXAdapter::FromForce(const FVector& ForceNewtons, const FVector& LocationCm, const float ReferenceForceNewtons, FWMVFXEvent& OutEvent)
{
    if (ForceNewtons.ContainsNaN() || !FMath::IsFinite(ReferenceForceNewtons) || ReferenceForceNewtons <= KINDA_SMALL_NUMBER) return false;
    const float ForceMagnitude = ForceNewtons.Size();
    if (!FMath::IsFinite(ForceMagnitude) || ForceMagnitude <= KINDA_SMALL_NUMBER) return false;
    OutEvent = BaseEvent(TEXT("science.physics.force"), LocationCm, FMath::Clamp(ForceMagnitude / ReferenceForceNewtons, 0.15f, 1.0f), 0.70f);
    OutEvent.Direction = ForceNewtons.GetSafeNormal();
    return OutEvent.IsSane();
}

bool FWMScienceVFXAdapter::FromCircuit(const float CurrentAmps, const float PowerWatts, const FVector& LocationCm, FWMVFXEvent& OutEvent)
{
    if (!FMath::IsFinite(CurrentAmps) || !FMath::IsFinite(PowerWatts) || CurrentAmps < 0.0f || PowerWatts < 0.0f) return false;
    const float Intensity = FMath::Clamp(0.15f + PowerWatts / 50.0f, 0.15f, 1.0f);
    OutEvent = BaseEvent(TEXT("science.physics.circuit-flow"), LocationCm, Intensity, 0.72f);
    return OutEvent.IsSane();
}

bool FWMScienceVFXAdapter::FromCell(const FWMCellStepResult& Result, const FVector& LocationCm, FWMVFXEvent& OutEvent)
{
    if (!Result.bAccepted || !FMath::IsFinite(Result.EnergyProducedUnits)) return false;
    OutEvent = BaseEvent(TEXT("science.biology.cell-energy"), LocationCm, FMath::Clamp(0.20f + Result.EnergyProducedUnits, 0.20f, 1.0f), 0.82f);
    return OutEvent.IsSane();
}

bool FWMScienceVFXAdapter::FromPlant(const FWMPlantStepResult& Result, const FVector& LocationCm, FWMVFXEvent& OutEvent)
{
    if (!Result.bAccepted || !FMath::IsFinite(Result.GrowthUnits) || Result.GrowthUnits <= 0.0f) return false;
    OutEvent = BaseEvent(TEXT("science.biology.plant-growth"), LocationCm, FMath::Clamp(0.20f + Result.GrowthUnits * 12.0f, 0.20f, 1.0f), 0.90f);
    OutEvent.Direction = FVector::UpVector;
    return OutEvent.IsSane();
}

bool FWMScienceVFXAdapter::FromEnvironmentDelta(const FWMEnvironmentStateDelta& Delta, const FVector& LocationCm, FWMVFXEvent& OutEvent)
{
    if (!Delta.IsSane()) return false;
    const float SignedChange = Delta.VegetationHealth + Delta.WaterFlow + Delta.SoilProtection + Delta.ShadeCoverage;
    if (FMath::IsNearlyZero(SignedChange)) return false;
    const float Magnitude = FMath::Clamp(FMath::Abs(SignedChange) * 0.75f, 0.20f, 1.0f);
    OutEvent = BaseEvent(SignedChange > 0.0f ? TEXT("science.ecology.recovery") : TEXT("science.ecology.stress"), LocationCm, Magnitude, 1.0f);
    return OutEvent.IsSane();
}
