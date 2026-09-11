#include "Visual/WMPresentationRuntime.h"

namespace
{
    bool IsFiniteVector(const FVector& Value)
    {
        return FMath::IsFinite(Value.X) && FMath::IsFinite(Value.Y) && FMath::IsFinite(Value.Z);
    }

    float SmoothStep01(const float T)
    {
        const float X = FMath::Clamp(T, 0.0f, 1.0f);
        return X * X * (3.0f - 2.0f * X);
    }
}

bool FWMPresentationCameraProfile::IsSane() const
{
    return FMath::IsFinite(ArmLengthCm) && ArmLengthCm >= 220.0f && ArmLengthCm <= 800.0f &&
        FMath::IsFinite(FieldOfViewDegrees) && FieldOfViewDegrees >= 50.0f && FieldOfViewDegrees <= 90.0f &&
        IsFiniteVector(TargetOffsetCm) &&
        FMath::IsFinite(BlendSeconds) && BlendSeconds >= 0.0f && BlendSeconds <= 1.5f &&
        FMath::IsFinite(HoldSeconds) && HoldSeconds >= 0.0f && HoldSeconds <= 3.0f;
}

bool FWMUIMotionPose::IsSane() const
{
    return FMath::IsFinite(Opacity) && Opacity >= 0.0f && Opacity <= 1.0f &&
        FMath::IsFinite(TranslationYPx) && FMath::Abs(TranslationYPx) <= 64.0f &&
        FMath::IsFinite(Scale) && Scale >= 0.8f && Scale <= 1.2f;
}

bool FWMPresentationRuntime::ResolveCameraProfile(
    const EWMPresentationCameraMode Mode,
    const bool bReducedMotion,
    FWMPresentationCameraProfile& OutProfile)
{
    OutProfile = FWMPresentationCameraProfile();
    OutProfile.Mode = Mode;

    switch (Mode)
    {
        case EWMPresentationCameraMode::Build:
            OutProfile.ArmLengthCm = 580.0f;
            OutProfile.FieldOfViewDegrees = 77.0f;
            OutProfile.TargetOffsetCm = FVector(0.0f, 0.0f, 78.0f);
            OutProfile.BlendSeconds = 0.32f;
            OutProfile.HoldSeconds = 0.65f;
            break;
        case EWMPresentationCameraMode::Observe:
            OutProfile.ArmLengthCm = 405.0f;
            OutProfile.FieldOfViewDegrees = 65.0f;
            OutProfile.TargetOffsetCm = FVector(0.0f, 0.0f, 72.0f);
            OutProfile.BlendSeconds = 0.28f;
            OutProfile.HoldSeconds = 0.85f;
            break;
        case EWMPresentationCameraMode::Science:
            OutProfile.ArmLengthCm = 365.0f;
            OutProfile.FieldOfViewDegrees = 62.0f;
            OutProfile.TargetOffsetCm = FVector(0.0f, 0.0f, 70.0f);
            OutProfile.BlendSeconds = 0.30f;
            OutProfile.HoldSeconds = 0.90f;
            break;
        case EWMPresentationCameraMode::Dialogue:
            OutProfile.ArmLengthCm = 345.0f;
            OutProfile.FieldOfViewDegrees = 60.0f;
            OutProfile.TargetOffsetCm = FVector(0.0f, 0.0f, 75.0f);
            OutProfile.BlendSeconds = 0.30f;
            OutProfile.HoldSeconds = 1.20f;
            break;
        case EWMPresentationCameraMode::AdventureReveal:
            OutProfile.ArmLengthCm = 650.0f;
            OutProfile.FieldOfViewDegrees = 69.0f;
            OutProfile.TargetOffsetCm = FVector(0.0f, 0.0f, 105.0f);
            OutProfile.BlendSeconds = 0.45f;
            OutProfile.HoldSeconds = 1.35f;
            break;
        case EWMPresentationCameraMode::Explore:
        default:
            OutProfile.ArmLengthCm = 500.0f;
            OutProfile.FieldOfViewDegrees = 72.0f;
            OutProfile.TargetOffsetCm = FVector(0.0f, 0.0f, 65.0f);
            OutProfile.BlendSeconds = 0.34f;
            OutProfile.HoldSeconds = 0.0f;
            break;
    }

    if (bReducedMotion && Mode != EWMPresentationCameraMode::Explore)
    {
        const FWMPresentationCameraProfile Explore;
        OutProfile.ArmLengthCm = FMath::Clamp(OutProfile.ArmLengthCm, Explore.ArmLengthCm - 80.0f, Explore.ArmLengthCm + 80.0f);
        OutProfile.FieldOfViewDegrees = FMath::Clamp(OutProfile.FieldOfViewDegrees, Explore.FieldOfViewDegrees - 3.0f, Explore.FieldOfViewDegrees + 3.0f);
        OutProfile.TargetOffsetCm.Z = FMath::Clamp(OutProfile.TargetOffsetCm.Z, Explore.TargetOffsetCm.Z - 16.0f, Explore.TargetOffsetCm.Z + 16.0f);
        OutProfile.BlendSeconds = FMath::Min(OutProfile.BlendSeconds, 0.18f);
        OutProfile.HoldSeconds = FMath::Min(OutProfile.HoldSeconds, 0.70f);
    }

    return OutProfile.IsSane();
}

EWMPresentationCameraMode FWMPresentationRuntime::ResolveModeForSemanticEvent(const FName EventId)
{
    const FString Id = EventId.ToString();
    if (Id.StartsWith(TEXT("gameplay.build."))) return EWMPresentationCameraMode::Build;
    if (Id.StartsWith(TEXT("science."))) return EWMPresentationCameraMode::Science;
    if (Id.StartsWith(TEXT("fantasy."))) return EWMPresentationCameraMode::AdventureReveal;
    if (Id == TEXT("world.observe.reveal")) return EWMPresentationCameraMode::Observe;
    if (Id == TEXT("mission.measure.reveal")) return EWMPresentationCameraMode::Science;
    return EWMPresentationCameraMode::Explore;
}

bool FWMPresentationRuntime::IsSupportedCue(const FName CueId)
{
    return CueId == TEXT("presentation.build.confirm") ||
        CueId == TEXT("presentation.observe.focus") ||
        CueId == TEXT("presentation.science.focus") ||
        CueId == TEXT("presentation.adventure.reveal") ||
        CueId == TEXT("presentation.mission.changed") ||
        CueId == TEXT("presentation.ecology.changed");
}

FWMUIMotionPose FWMPresentationRuntime::EvaluateUIMotion(
    const float AgeSeconds,
    const float DurationSeconds,
    const bool bReducedMotion)
{
    FWMUIMotionPose Pose;
    if (!FMath::IsFinite(AgeSeconds) || !FMath::IsFinite(DurationSeconds) || DurationSeconds <= 0.0f)
    {
        return Pose;
    }

    const float T = FMath::Clamp(AgeSeconds / DurationSeconds, 0.0f, 1.0f);
    const float FadeIn = SmoothStep01(T / 0.18f);
    const float FadeOut = SmoothStep01((1.0f - T) / 0.22f);
    Pose.Opacity = FMath::Min(FadeIn, FadeOut);

    if (bReducedMotion)
    {
        Pose.TranslationYPx = 0.0f;
        Pose.Scale = 1.0f;
    }
    else
    {
        Pose.TranslationYPx = FMath::Lerp(18.0f, 0.0f, SmoothStep01(FMath::Min(T / 0.24f, 1.0f)));
        Pose.Scale = FMath::Lerp(0.98f, 1.0f, SmoothStep01(FMath::Min(T / 0.24f, 1.0f)));
    }
    return Pose;
}

float FWMPresentationRuntime::ClampPulseDuration(const float RequestedSeconds, const bool bReducedMotion)
{
    if (!FMath::IsFinite(RequestedSeconds)) return 0.0f;
    const float Ceiling = bReducedMotion ? 0.85f : 2.5f;
    return FMath::Clamp(RequestedSeconds, 0.20f, Ceiling);
}
