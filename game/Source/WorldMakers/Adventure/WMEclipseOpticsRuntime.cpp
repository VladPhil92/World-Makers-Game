#include "Adventure/WMEclipseOpticsRuntime.h"

#include "Adventure/WMEclipseEngineExperienceSubsystem.h"

namespace
{
    float NormalizeSignedAngle(const float Degrees)
    {
        return FMath::UnwindDegrees(Degrees);
    }
}

bool FWMEclipseOpticsRuntime::IsSymmetrySolved(
    const float LeftMirrorAngleDeg,
    const float RightMirrorAngleDeg,
    const float ToleranceDeg)
{
    if (!FMath::IsFinite(LeftMirrorAngleDeg) || !FMath::IsFinite(RightMirrorAngleDeg) ||
        !FMath::IsFinite(ToleranceDeg) || ToleranceDeg <= 0.0f || ToleranceDeg > 10.0f)
    {
        return false;
    }

    const float Left = NormalizeSignedAngle(LeftMirrorAngleDeg);
    const float Right = NormalizeSignedAngle(RightMirrorAngleDeg);
    const bool bMeaningfulDeflection = FMath::Abs(Left) >= 8.0f && FMath::Abs(Right) >= 8.0f;
    return bMeaningfulDeflection && FMath::Abs(Left + Right) <= ToleranceDeg;
}

bool FWMEclipseOpticsRuntime::IsReflectionSolved(
    const float IncidenceAngleDeg,
    const float ReflectionAngleDeg,
    const float TargetDeviationDeg,
    const float ToleranceDeg)
{
    if (!FMath::IsFinite(IncidenceAngleDeg) || !FMath::IsFinite(ReflectionAngleDeg) ||
        !FMath::IsFinite(TargetDeviationDeg) || !FMath::IsFinite(ToleranceDeg) ||
        ToleranceDeg <= 0.0f || ToleranceDeg > 10.0f)
    {
        return false;
    }

    const float Incidence = FMath::Abs(NormalizeSignedAngle(IncidenceAngleDeg));
    const float Reflection = FMath::Abs(NormalizeSignedAngle(ReflectionAngleDeg));
    if (Incidence > 89.0f || Reflection > 89.0f) return false;
    return FMath::Abs(Incidence - Reflection) <= ToleranceDeg && FMath::Abs(TargetDeviationDeg) <= ToleranceDeg * 2.0f;
}

bool FWMEclipseOpticsRuntime::IsPathStable(
    const TArray<float>& PerturbationDeviationDeg,
    const float MaxAllowedDeviationDeg)
{
    if (PerturbationDeviationDeg.Num() < 3 || PerturbationDeviationDeg.Num() > 12 ||
        !FMath::IsFinite(MaxAllowedDeviationDeg) || MaxAllowedDeviationDeg <= 0.0f || MaxAllowedDeviationDeg > 12.0f)
    {
        return false;
    }

    bool bHasPositivePerturbation = false;
    bool bHasNegativePerturbation = false;
    for (const float Deviation : PerturbationDeviationDeg)
    {
        if (!FMath::IsFinite(Deviation) || FMath::Abs(Deviation) > MaxAllowedDeviationDeg) return false;
        bHasPositivePerturbation |= Deviation > KINDA_SMALL_NUMBER;
        bHasNegativePerturbation |= Deviation < -KINDA_SMALL_NUMBER;
    }
    return bHasPositivePerturbation && bHasNegativePerturbation;
}

bool UWMEclipseOpticsSubsystem::SubmitMirrorSymmetry(const float LeftMirrorAngleDeg, const float RightMirrorAngleDeg)
{
    if (!FWMEclipseOpticsRuntime::IsSymmetrySolved(LeftMirrorAngleDeg, RightMirrorAngleDeg) || !GetWorld()) return false;
    UWMEclipseEngineExperienceSubsystem* Experience = GetWorld()->GetSubsystem<UWMEclipseEngineExperienceSubsystem>();
    return Experience && Experience->ResolveTrustedAction(TEXT("eclipse.restore-mirror-symmetry"));
}

bool UWMEclipseOpticsSubsystem::SubmitReflectionBridge(
    const float IncidenceAngleDeg,
    const float ReflectionAngleDeg,
    const float TargetDeviationDeg)
{
    if (!FWMEclipseOpticsRuntime::IsReflectionSolved(IncidenceAngleDeg, ReflectionAngleDeg, TargetDeviationDeg) || !GetWorld()) return false;
    UWMEclipseEngineExperienceSubsystem* Experience = GetWorld()->GetSubsystem<UWMEclipseEngineExperienceSubsystem>();
    return Experience && Experience->ResolveTrustedAction(TEXT("eclipse-angle-light-bridge"));
}

bool UWMEclipseOpticsSubsystem::SubmitSpatialStability(const TArray<float>& PerturbationDeviationDeg)
{
    if (!FWMEclipseOpticsRuntime::IsPathStable(PerturbationDeviationDeg) || !GetWorld()) return false;
    UWMEclipseEngineExperienceSubsystem* Experience = GetWorld()->GetSubsystem<UWMEclipseEngineExperienceSubsystem>();
    return Experience && Experience->ResolveTrustedAction(TEXT("eclipse-prove-light-path"));
}
