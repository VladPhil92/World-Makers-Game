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

    /**
     * Context inference is accepted only when the selected meaning matches the authoritative target
     * and every observed clue belongs to the authoritative puzzle clue set. Caller-controlled clue IDs
     * can therefore never create a tautological completion path.
     */
    static bool IsContextInferenceSupported(
        FName SelectedMeaningId,
        FName RequiredMeaningId,
        const TArray<FName>& ObservedClueIds,
        const TSet<FName>& AllowedClueIds,
        int32 RequiredClueCount = 2);
};

/** Trusted C++ adapters: only validated mechanic state is translated into hidden Eclipse evidence. */
UCLASS()
class WORLDMAKERS_API UWMEclipseSystemsSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    void ResetPrototypeState();
    bool ObserveContextClue(FName ClueId);

    bool SubmitOrbitPattern(const TArray<int32>& ObservedValues, int32 PredictedNextValue);
    bool SubmitOrbitRatio(int32 LeftA, int32 LeftB, int32 RightA, int32 RightB);
    bool SubmitOptimizedRoute(float CandidateLengthCm, const TArray<float>& AlternativeLengthsCm);
    bool SubmitForcePrediction(
        float MassKg,
        FVector ForceNewtons,
        float DeltaSeconds,
        FVector PredictedVelocityMetersPerSecond,
        FVector MeasuredVelocityMetersPerSecond);
    bool SubmitCircuitModel(float VoltageVolts, float ResistanceOhms, float PlayerCurrentAmps, float PlayerPowerWatts);

    /** The required meaning and valid clue IDs are derived internally from the active prototype puzzle. */
    bool SubmitContextInference(FName SelectedMeaningId);

private:
    TSet<FName> ObservedContextClues;
};
