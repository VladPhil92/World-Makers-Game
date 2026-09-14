#include "Adventure/WMEclipseSystemsRuntime.h"

#include "Adventure/WMEclipseEngineExperienceSubsystem.h"
#include "Science/WMScienceSimulationCore.h"

namespace
{
    const FName ContextRequiredMeaning(TEXT("meaning.eclipse.archive-linked-symbols"));
    const TSet<FName> ContextAllowedClues = {
        TEXT("clue.eclipse.archive-neighbor-symbol"),
        TEXT("clue.eclipse.archive-response-pattern"),
        TEXT("clue.eclipse.archive-repeated-glyph")
    };
}

bool FWMEclipseSystemsRuntime::InferArithmeticPattern(const TArray<int32>& ObservedValues, const int32 PredictedNextValue)
{
    if (ObservedValues.Num() < 4 || ObservedValues.Num() > 12) return false;
    const int64 Difference = static_cast<int64>(ObservedValues[1]) - static_cast<int64>(ObservedValues[0]);
    if (Difference == 0 || FMath::Abs(Difference) > 100000) return false;
    for (int32 Index = 2; Index < ObservedValues.Num(); ++Index)
    {
        if (static_cast<int64>(ObservedValues[Index]) - static_cast<int64>(ObservedValues[Index - 1]) != Difference) return false;
    }
    const int64 Expected = static_cast<int64>(ObservedValues.Last()) + Difference;
    return Expected >= MIN_int32 && Expected <= MAX_int32 && PredictedNextValue == static_cast<int32>(Expected);
}

bool FWMEclipseSystemsRuntime::IsEquivalentRatio(const int32 LeftA, const int32 LeftB, const int32 RightA, const int32 RightB)
{
    if (LeftA <= 0 || LeftB <= 0 || RightA <= 0 || RightB <= 0 ||
        LeftA > 10000 || LeftB > 10000 || RightA > 10000 || RightB > 10000) return false;
    if (LeftA == RightA && LeftB == RightB) return false;
    return static_cast<int64>(LeftA) * static_cast<int64>(RightB) == static_cast<int64>(RightA) * static_cast<int64>(LeftB);
}

bool FWMEclipseSystemsRuntime::IsOptimizedRoute(
    const float CandidateLengthCm,
    const TArray<float>& AlternativeLengthsCm,
    const float MinimumImprovementFraction)
{
    if (!FMath::IsFinite(CandidateLengthCm) || CandidateLengthCm <= 0.0f || AlternativeLengthsCm.Num() < 2 ||
        AlternativeLengthsCm.Num() > 12 || !FMath::IsFinite(MinimumImprovementFraction) ||
        MinimumImprovementFraction < 0.0f || MinimumImprovementFraction > 0.5f) return false;

    float BestAlternative = TNumericLimits<float>::Max();
    for (const float Length : AlternativeLengthsCm)
    {
        if (!FMath::IsFinite(Length) || Length <= 0.0f) return false;
        BestAlternative = FMath::Min(BestAlternative, Length);
    }
    return CandidateLengthCm <= BestAlternative * (1.0f - MinimumImprovementFraction);
}

bool FWMEclipseSystemsRuntime::IsForcePredictionConsistent(
    const float MassKg,
    const FVector ForceNewtons,
    const float DeltaSeconds,
    const FVector PredictedVelocityMetersPerSecond,
    const FVector MeasuredVelocityMetersPerSecond,
    const float VelocityTolerance)
{
    if (!FMath::IsFinite(MassKg) || MassKg < 0.1f || MassKg > 1000.0f || ForceNewtons.ContainsNaN() ||
        ForceNewtons.Size() > 10000.0f || !FMath::IsFinite(DeltaSeconds) || DeltaSeconds <= 0.0f || DeltaSeconds > 10.0f ||
        PredictedVelocityMetersPerSecond.ContainsNaN() || MeasuredVelocityMetersPerSecond.ContainsNaN() ||
        !FMath::IsFinite(VelocityTolerance) || VelocityTolerance <= 0.0f || VelocityTolerance > 1.0f) return false;

    FWMPhysicsBodyState Simulated;
    Simulated.MassKg = MassKg;
    Simulated.PositionMeters = FVector::ZeroVector;
    Simulated.VelocityMetersPerSecond = FVector::ZeroVector;
    if (!Simulated.StepConstantForce(ForceNewtons, DeltaSeconds)) return false;

    const float Scale = FMath::Max(1.0f, Simulated.VelocityMetersPerSecond.Size());
    const float AllowedError = VelocityTolerance * Scale;
    return FVector::Dist(PredictedVelocityMetersPerSecond, Simulated.VelocityMetersPerSecond) <= AllowedError &&
        FVector::Dist(MeasuredVelocityMetersPerSecond, Simulated.VelocityMetersPerSecond) <= AllowedError;
}

bool FWMEclipseSystemsRuntime::IsCircuitModelConsistent(
    const float VoltageVolts,
    const float ResistanceOhms,
    const float PlayerCurrentAmps,
    const float PlayerPowerWatts,
    const float RelativeTolerance)
{
    if (!FMath::IsFinite(PlayerCurrentAmps) || !FMath::IsFinite(PlayerPowerWatts) ||
        !FMath::IsFinite(RelativeTolerance) || RelativeTolerance <= 0.0f || RelativeTolerance > 0.25f) return false;

    FWMDirectCurrentCircuit Circuit;
    Circuit.VoltageVolts = VoltageVolts;
    Circuit.ResistanceOhms = ResistanceOhms;
    float ExpectedCurrent = 0.0f;
    float ExpectedPower = 0.0f;
    if (!Circuit.Solve(ExpectedCurrent, ExpectedPower)) return false;

    const float CurrentTolerance = FMath::Max(0.01f, FMath::Abs(ExpectedCurrent) * RelativeTolerance);
    const float PowerTolerance = FMath::Max(0.01f, FMath::Abs(ExpectedPower) * RelativeTolerance);
    return FMath::Abs(PlayerCurrentAmps - ExpectedCurrent) <= CurrentTolerance &&
        FMath::Abs(PlayerPowerWatts - ExpectedPower) <= PowerTolerance;
}

bool FWMEclipseSystemsRuntime::IsContextInferenceSupported(
    const FName SelectedMeaningId,
    const FName RequiredMeaningId,
    const TArray<FName>& ObservedClueIds,
    const TSet<FName>& AllowedClueIds,
    const int32 RequiredClueCount)
{
    if (SelectedMeaningId.IsNone() || RequiredMeaningId.IsNone() || SelectedMeaningId != RequiredMeaningId ||
        RequiredClueCount < 1 || RequiredClueCount > 8 || AllowedClueIds.Num() < RequiredClueCount ||
        ObservedClueIds.Num() < RequiredClueCount || ObservedClueIds.Num() > AllowedClueIds.Num()) return false;

    TSet<FName> Unique;
    for (const FName ClueId : ObservedClueIds)
    {
        if (ClueId.IsNone() || !AllowedClueIds.Contains(ClueId)) return false;
        Unique.Add(ClueId);
    }
    return Unique.Num() >= RequiredClueCount;
}

void UWMEclipseSystemsSubsystem::ResetPrototypeState()
{
    ObservedContextClues.Reset();
}

bool UWMEclipseSystemsSubsystem::ObserveContextClue(const FName ClueId)
{
    if (!ContextAllowedClues.Contains(ClueId)) return false;
    ObservedContextClues.Add(ClueId);
    return true;
}

bool UWMEclipseSystemsSubsystem::SubmitOrbitPattern(const TArray<int32>& ObservedValues, const int32 PredictedNextValue)
{
    if (!FWMEclipseSystemsRuntime::InferArithmeticPattern(ObservedValues, PredictedNextValue) || !GetWorld()) return false;
    UWMEclipseEngineExperienceSubsystem* Experience = GetWorld()->GetSubsystem<UWMEclipseEngineExperienceSubsystem>();
    return Experience && Experience->ResolveTrustedAction(TEXT("eclipse.read-orbit-rhythm"));
}

bool UWMEclipseSystemsSubsystem::SubmitOrbitRatio(const int32 LeftA, const int32 LeftB, const int32 RightA, const int32 RightB)
{
    if (!FWMEclipseSystemsRuntime::IsEquivalentRatio(LeftA, LeftB, RightA, RightB) || !GetWorld()) return false;
    UWMEclipseEngineExperienceSubsystem* Experience = GetWorld()->GetSubsystem<UWMEclipseEngineExperienceSubsystem>();
    return Experience && Experience->ResolveTrustedAction(TEXT("eclipse.balance-orbit-ratio"));
}

bool UWMEclipseSystemsSubsystem::SubmitOptimizedRoute(const float CandidateLengthCm, const TArray<float>& AlternativeLengthsCm)
{
    if (!FWMEclipseSystemsRuntime::IsOptimizedRoute(CandidateLengthCm, AlternativeLengthsCm) || !GetWorld()) return false;
    UWMEclipseEngineExperienceSubsystem* Experience = GetWorld()->GetSubsystem<UWMEclipseEngineExperienceSubsystem>();
    return Experience && Experience->ResolveTrustedAction(TEXT("eclipse.build-orbit-route"));
}

bool UWMEclipseSystemsSubsystem::SubmitForcePrediction(
    const float MassKg,
    const FVector ForceNewtons,
    const float DeltaSeconds,
    const FVector PredictedVelocityMetersPerSecond,
    const FVector MeasuredVelocityMetersPerSecond)
{
    if (!FWMEclipseSystemsRuntime::IsForcePredictionConsistent(
        MassKg, ForceNewtons, DeltaSeconds, PredictedVelocityMetersPerSecond, MeasuredVelocityMetersPerSecond) || !GetWorld()) return false;
    UWMEclipseEngineExperienceSubsystem* Experience = GetWorld()->GetSubsystem<UWMEclipseEngineExperienceSubsystem>();
    return Experience && Experience->ResolveTrustedAction(TEXT("eclipse-test-counterweight"));
}

bool UWMEclipseSystemsSubsystem::SubmitCircuitModel(
    const float VoltageVolts,
    const float ResistanceOhms,
    const float PlayerCurrentAmps,
    const float PlayerPowerWatts)
{
    if (!FWMEclipseSystemsRuntime::IsCircuitModelConsistent(
        VoltageVolts, ResistanceOhms, PlayerCurrentAmps, PlayerPowerWatts) || !GetWorld()) return false;
    UWMEclipseEngineExperienceSubsystem* Experience = GetWorld()->GetSubsystem<UWMEclipseEngineExperienceSubsystem>();
    return Experience && Experience->ResolveTrustedAction(TEXT("eclipse-route-core-current"));
}

bool UWMEclipseSystemsSubsystem::SubmitContextInference(const FName SelectedMeaningId)
{
    if (!GetWorld()) return false;
    TArray<FName> Clues = ObservedContextClues.Array();
    Clues.Sort([](const FName& A, const FName& B) { return A.ToString() < B.ToString(); });
    if (!FWMEclipseSystemsRuntime::IsContextInferenceSupported(
        SelectedMeaningId, ContextRequiredMeaning, Clues, ContextAllowedClues, 2)) return false;

    UWMEclipseEngineExperienceSubsystem* Experience = GetWorld()->GetSubsystem<UWMEclipseEngineExperienceSubsystem>();
    return Experience && Experience->ResolveTrustedAction(TEXT("eclipse-decode-context-fragment"));
}
