#pragma once

#include "CoreMinimal.h"
#include "Science/WMScienceSimulationCore.h"
#include "Visual/WMVFXRuntime.h"

/**
 * Converts already-accepted science results into presentation-only VFX events.
 * This layer never changes simulation state or learning evidence.
 */
struct WORLDMAKERS_API FWMScienceVFXAdapter
{
    static bool FromDissolution(const FWMDissolutionResult& Result, const FVector& LocationCm, FWMVFXEvent& OutEvent);
    static bool FromFiltration(const FWMFiltrationResult& Result, const FVector& LocationCm, FWMVFXEvent& OutEvent);
    static bool FromReaction(const FWMReactionResult& Result, const FVector& LocationCm, FWMVFXEvent& OutEvent);
    static bool FromForce(const FVector& ForceNewtons, const FVector& LocationCm, float ReferenceForceNewtons, FWMVFXEvent& OutEvent);
    static bool FromCircuit(float CurrentAmps, float PowerWatts, const FVector& LocationCm, FWMVFXEvent& OutEvent);
    static bool FromCell(const FWMCellStepResult& Result, const FVector& LocationCm, FWMVFXEvent& OutEvent);
    static bool FromPlant(const FWMPlantStepResult& Result, const FVector& LocationCm, FWMVFXEvent& OutEvent);
    static bool FromEnvironmentDelta(const FWMEnvironmentStateDelta& Delta, const FVector& LocationCm, FWMVFXEvent& OutEvent);
};
