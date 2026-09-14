#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WMEclipseSystemsRuntime.generated.h"

/** Deterministic game mechanics for Eclipse acts outside the dedicated mirror-lattice subsystem. */
struct WORLDMAKERS_API FWMEclipseSystemsRuntime
{
    static bool InferArithmeticPattern(const TArray<int32>& ObservedValues, int32 PredictedNextValue);
    static bool IsEquivalentRatio(int32 LeftA, int32 LeftB, int32 RightA, int32 RightB);
    static bool IsOptimizedRoute(float CandidateLengthCm, const TArray<float>& AlternativeLengthsCm, float MinimumImprovementFraction = 0.05f);

    static bool IsForcePredictionConsistent(
        float MassKg,
        FVector ForceNewtons,
        float DeltaSeconds,
        FVector PredictedVelocityMetersPerSecond,
        FVector MeasuredVelocityMetersPerSecond,
        float VelocityTolerance = 0.08f);

    static bool IsCircuitModelConsistent(
        float VoltageVolts,
        float ResistanceOhms,
        float PlayerCurrentAmps,
        float PlayerPowerWatts,
        float RelativeTolerance = 0.05f);

    static bool IsContextInferenceSupported(FName SelectedMeaningId, FName RequiredMeaningId, const TArray<FName>& ObservedClueIds);
};

/** Trusted adapters: only validated mechanic state is translated into hidden Eclipse evidence. */
UCLASS()
class WORLDMAKERS_API UWMEclipseSystemsSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "World Makers|Eclipse|Orbit")
    bool SubmitOrbitPattern(const TArray<int32>& ObservedValues, int32 PredictedNextValue);

    UFUNCTION(BlueprintCallable, Category = "World Makers|Eclipse|Orbit")
    bool SubmitOrbitRatio(int32 LeftA, int32 LeftB, int32 RightA, int32 RightB);

    UFUNCTION(BlueprintCallable, Category = "World Makers|Eclipse|Orbit")
    bool SubmitOptimizedRoute(float CandidateLengthCm, const TArray<float>& AlternativeLengthsCm);

    UFUNCTION(BlueprintCallable, Category = "World Makers|Eclipse|Power")
    bool SubmitForcePrediction(
        float MassKg,
        FVector ForceNewtons,
        float DeltaSeconds,
        FVector PredictedVelocityMetersPerSecond,
        FVector MeasuredVelocityMetersPerSecond);

    UFUNCTION(BlueprintCallable, Category = "World Makers|Eclipse|Power")
    bool SubmitCircuitModel(float VoltageVolts, float ResistanceOhms, float PlayerCurrentAmps, float PlayerPowerWatts);

    UFUNCTION(BlueprintCallable, Category = "World Makers|Eclipse|Archive")
    bool SubmitContextInference(FName SelectedMeaningId, FName RequiredMeaningId, const TArray<FName>& ObservedClueIds);
};
