#pragma once

#include "CoreMinimal.h"

enum class EWMPresentationCameraMode : uint8
{
    Explore,
    Build,
    Observe,
    Science,
    Dialogue,
    AdventureReveal
};

struct WORLDMAKERS_API FWMPresentationCameraProfile
{
    EWMPresentationCameraMode Mode = EWMPresentationCameraMode::Explore;
    float ArmLengthCm = 500.0f;
    float FieldOfViewDegrees = 72.0f;
    FVector TargetOffsetCm = FVector(0.0f, 0.0f, 65.0f);
    float BlendSeconds = 0.35f;
    float HoldSeconds = 0.0f;

    bool IsSane() const;
};

struct WORLDMAKERS_API FWMUIMotionPose
{
    float Opacity = 0.0f;
    float TranslationYPx = 0.0f;
    float Scale = 1.0f;

    bool IsSane() const;
};

struct WORLDMAKERS_API FWMPresentationBeatState
{
    FName CueId;
    float AgeSeconds = 0.0f;
    float DurationSeconds = 0.0f;
    bool bReducedMotion = false;

    bool IsActive() const { return !CueId.IsNone() && AgeSeconds < DurationSeconds; }
};

struct WORLDMAKERS_API FWMPresentationRuntime
{
    static bool ResolveCameraProfile(EWMPresentationCameraMode Mode, bool bReducedMotion, FWMPresentationCameraProfile& OutProfile);
    static EWMPresentationCameraMode ResolveModeForSemanticEvent(FName EventId);
    static bool IsSupportedCue(FName CueId);
    static FWMUIMotionPose EvaluateUIMotion(float AgeSeconds, float DurationSeconds, bool bReducedMotion);
    static float ClampPulseDuration(float RequestedSeconds, bool bReducedMotion);
};
