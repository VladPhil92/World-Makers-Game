#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WMEclipseOpticsRuntime.generated.h"

/** Pure deterministic optics checks used by The Eclipse Engine mirror-lattice puzzle. */
struct WORLDMAKERS_API FWMEclipseOpticsRuntime
{
    static bool IsSymmetrySolved(float LeftMirrorAngleDeg, float RightMirrorAngleDeg, float ToleranceDeg = 2.0f);
    static bool IsReflectionSolved(float IncidenceAngleDeg, float ReflectionAngleDeg, float TargetDeviationDeg, float ToleranceDeg = 2.0f);
    static bool IsPathStable(const TArray<float>& PerturbationDeviationDeg, float MaxAllowedDeviationDeg = 4.0f);
};

/**
 * Trusted C++ adapter between physical/spatial puzzle results and the hidden learning-evidence ledger.
 * Raw clicks and player-facing Blueprint/UI cannot directly award evidence.
 */
UCLASS()
class WORLDMAKERS_API UWMEclipseOpticsSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    bool SubmitMirrorSymmetry(float LeftMirrorAngleDeg, float RightMirrorAngleDeg);
    bool SubmitReflectionBridge(float IncidenceAngleDeg, float ReflectionAngleDeg, float TargetDeviationDeg);
    bool SubmitSpatialStability(const TArray<float>& PerturbationDeviationDeg);
};
