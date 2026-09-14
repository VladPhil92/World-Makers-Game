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
 * Trusted adapter between physical/spatial puzzle results and the hidden learning-evidence ledger.
 * Raw clicks never reach this subsystem; callers submit measured puzzle state.
 */
UCLASS()
class WORLDMAKERS_API UWMEclipseOpticsSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "World Makers|Eclipse|Optics")
    bool SubmitMirrorSymmetry(float LeftMirrorAngleDeg, float RightMirrorAngleDeg);

    UFUNCTION(BlueprintCallable, Category = "World Makers|Eclipse|Optics")
    bool SubmitReflectionBridge(float IncidenceAngleDeg, float ReflectionAngleDeg, float TargetDeviationDeg);

    UFUNCTION(BlueprintCallable, Category = "World Makers|Eclipse|Optics")
    bool SubmitSpatialStability(const TArray<float>& PerturbationDeviationDeg);
};
