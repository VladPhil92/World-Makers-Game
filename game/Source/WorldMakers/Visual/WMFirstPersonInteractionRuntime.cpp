#include "Visual/WMFirstPersonInteractionRuntime.h"

namespace
{
    float SmoothStep01(const float Value)
    {
        const float X = FMath::Clamp(Value, 0.0f, 1.0f);
        return X * X * (3.0f - 2.0f * X);
    }

    float Bell01(const float Value)
    {
        const float X = FMath::Clamp(Value, 0.0f, 1.0f);
        return FMath::Sin(X * PI);
    }
}

bool FWMFirstPersonInteractionPose::IsSane() const
{
    return !ToolTranslationCm.ContainsNaN() && !LeftHandTranslationCm.ContainsNaN() && !RightHandTranslationCm.ContainsNaN() &&
        FMath::IsFinite(ToolRotationDegrees.Pitch) && FMath::IsFinite(ToolRotationDegrees.Yaw) && FMath::IsFinite(ToolRotationDegrees.Roll) &&
        ToolTranslationCm.Size() <= 18.0f && LeftHandTranslationCm.Size() <= 12.0f && RightHandTranslationCm.Size() <= 12.0f &&
        FMath::Abs(ToolRotationDegrees.Pitch) <= 18.0f && FMath::Abs(ToolRotationDegrees.Yaw) <= 18.0f && FMath::Abs(ToolRotationDegrees.Roll) <= 18.0f &&
        FMath::IsFinite(ToolScale) && ToolScale >= 0.85f && ToolScale <= 1.15f &&
        FMath::IsFinite(ContextAlpha) && ContextAlpha >= 0.0f && ContextAlpha <= 1.0f &&
        FMath::IsFinite(ViewModelBobCm) && FMath::Abs(ViewModelBobCm) <= 1.5f &&
        FMath::IsFinite(ActionAlpha) && ActionAlpha >= 0.0f && ActionAlpha <= 1.0f;
}

void FWMFirstPersonInteractionRuntime::Reset()
{
    Action = EWMFirstPersonInteractionAction::Idle;
    DurationSeconds = 0.0f;
    RemainingSeconds = 0.0f;
    AgeSeconds = 0.0f;
}

void FWMFirstPersonInteractionRuntime::Trigger(const EWMFirstPersonInteractionAction NewAction, const float RequestedDurationSeconds)
{
    if (NewAction == EWMFirstPersonInteractionAction::Idle)
    {
        Reset();
        return;
    }

    Action = NewAction;
    DurationSeconds = FMath::Clamp(FMath::IsFinite(RequestedDurationSeconds) ? RequestedDurationSeconds : 0.70f, 0.18f, 2.25f);
    RemainingSeconds = DurationSeconds;
    AgeSeconds = 0.0f;
}

FWMFirstPersonInteractionPose FWMFirstPersonInteractionRuntime::Update(const float DeltaSeconds, const bool bReducedMotion)
{
    const float SafeDelta = FMath::Clamp(FMath::IsFinite(DeltaSeconds) ? DeltaSeconds : 0.0f, 0.0f, 0.10f);
    if (Action == EWMFirstPersonInteractionAction::Idle || RemainingSeconds <= 0.0f)
    {
        Reset();
        return EvaluatePose(bReducedMotion);
    }

    AgeSeconds = FMath::Min(DurationSeconds, AgeSeconds + SafeDelta);
    RemainingSeconds = FMath::Max(0.0f, DurationSeconds - AgeSeconds);
    const FWMFirstPersonInteractionPose Pose = EvaluatePose(bReducedMotion);
    if (RemainingSeconds <= KINDA_SMALL_NUMBER)
    {
        Action = EWMFirstPersonInteractionAction::Idle;
        RemainingSeconds = 0.0f;
    }
    return Pose;
}

FWMFirstPersonInteractionPose FWMFirstPersonInteractionRuntime::EvaluatePose(const bool bReducedMotion) const
{
    FWMFirstPersonInteractionPose Pose;
    if (Action == EWMFirstPersonInteractionAction::Idle || DurationSeconds <= 0.0f)
    {
        return Pose;
    }

    const float T = FMath::Clamp(AgeSeconds / DurationSeconds, 0.0f, 1.0f);
    const float In = SmoothStep01(FMath::Min(T / 0.22f, 1.0f));
    const float Out = SmoothStep01(FMath::Min((1.0f - T) / 0.24f, 1.0f));
    const float A = FMath::Min(In, Out);
    Pose.ActionAlpha = A;
    Pose.ContextAlpha = FMath::Clamp(FMath::Min(In, SmoothStep01(FMath::Min((1.0f - T) / 0.14f, 1.0f))), 0.0f, 1.0f);

    if (!bReducedMotion)
    {
        Pose.ViewModelBobCm = FMath::Sin(AgeSeconds * 7.0f) * 0.28f * A;
    }

    switch (Action)
    {
    case EWMFirstPersonInteractionAction::ToolRaise:
        Pose.ToolTranslationCm = FVector(0.0f, 0.0f, FMath::Lerp(-7.0f, 0.0f, In));
        Pose.ToolRotationDegrees = FRotator(FMath::Lerp(10.0f, 0.0f, In), 0.0f, 0.0f);
        Pose.ContextAlpha = In;
        break;
    case EWMFirstPersonInteractionAction::ToolLower:
        Pose.ToolTranslationCm = FVector(0.0f, 0.0f, FMath::Lerp(0.0f, -7.0f, SmoothStep01(T)));
        Pose.ToolRotationDegrees = FRotator(FMath::Lerp(0.0f, 10.0f, SmoothStep01(T)), 0.0f, 0.0f);
        Pose.ContextAlpha = 1.0f - SmoothStep01(T);
        break;
    case EWMFirstPersonInteractionAction::Scan:
        Pose.ToolTranslationCm = FVector(1.2f * Bell01(T), 0.0f, 0.5f * Bell01(T));
        Pose.ToolRotationDegrees = FRotator(-3.0f * Bell01(T), 2.0f * Bell01(T), 0.0f);
        Pose.ToolScale = 1.0f + 0.03f * Bell01(T);
        Pose.LeftHandTranslationCm = FVector(0.0f, -0.6f * A, 0.0f);
        Pose.RightHandTranslationCm = FVector(0.8f * A, 0.4f * A, 0.0f);
        break;
    case EWMFirstPersonInteractionAction::BuildPoint:
        Pose.ToolTranslationCm = FVector(3.2f * A, 0.0f, 1.0f * A);
        Pose.ToolRotationDegrees = FRotator(-5.0f * A, -3.0f * A, 0.0f);
        Pose.RightHandTranslationCm = FVector(2.2f * A, 0.0f, 0.4f * A);
        break;
    case EWMFirstPersonInteractionAction::BuildConfirm:
        Pose.ToolTranslationCm = FVector(1.8f * A, 0.0f, -0.8f * Bell01(T));
        Pose.ToolRotationDegrees = FRotator(-7.0f * Bell01(T), 0.0f, 2.0f * Bell01(T));
        Pose.ToolScale = 1.0f + 0.05f * Bell01(T);
        Pose.RightHandTranslationCm = FVector(1.5f * A, 0.0f, -0.5f * A);
        break;
    case EWMFirstPersonInteractionAction::MeasureFocus:
        Pose.ToolTranslationCm = FVector(2.0f * A, -0.6f * A, 1.4f * A);
        Pose.ToolRotationDegrees = FRotator(-2.0f * A, -2.0f * A, 0.0f);
        Pose.LeftHandTranslationCm = FVector(0.8f * A, -0.8f * A, 0.2f * A);
        Pose.RightHandTranslationCm = FVector(1.0f * A, 0.6f * A, 0.2f * A);
        break;
    case EWMFirstPersonInteractionAction::ObserveFocus:
        Pose.ToolTranslationCm = FVector(-1.5f * A, 0.0f, -1.2f * A);
        Pose.ToolRotationDegrees = FRotator(2.0f * A, 0.0f, 0.0f);
        Pose.LeftHandTranslationCm = FVector(-0.8f * A, -0.4f * A, -0.2f * A);
        Pose.RightHandTranslationCm = FVector(-0.8f * A, 0.4f * A, -0.2f * A);
        break;
    case EWMFirstPersonInteractionAction::Idle:
    default:
        break;
    }

    if (bReducedMotion)
    {
        Pose.ViewModelBobCm = 0.0f;
        Pose.ToolTranslationCm *= 0.45f;
        Pose.ToolRotationDegrees.Pitch *= 0.45f;
        Pose.ToolRotationDegrees.Yaw *= 0.45f;
        Pose.ToolRotationDegrees.Roll *= 0.45f;
        Pose.LeftHandTranslationCm *= 0.45f;
        Pose.RightHandTranslationCm *= 0.45f;
    }

    return Pose;
}

FName FWMFirstPersonInteractionRuntime::ActionToId(const EWMFirstPersonInteractionAction Value)
{
    switch (Value)
    {
    case EWMFirstPersonInteractionAction::ToolRaise: return TEXT("tool-raise");
    case EWMFirstPersonInteractionAction::ToolLower: return TEXT("tool-lower");
    case EWMFirstPersonInteractionAction::Scan: return TEXT("scan-hold");
    case EWMFirstPersonInteractionAction::BuildPoint: return TEXT("build-point");
    case EWMFirstPersonInteractionAction::BuildConfirm: return TEXT("build-confirm");
    case EWMFirstPersonInteractionAction::MeasureFocus: return TEXT("measure-focus");
    case EWMFirstPersonInteractionAction::ObserveFocus: return TEXT("observe-focus");
    case EWMFirstPersonInteractionAction::Idle:
    default: return TEXT("idle");
    }
}

bool FWMFirstPersonInteractionRuntime::TryParseAction(const FName ActionId, EWMFirstPersonInteractionAction& OutAction)
{
    if (ActionId == TEXT("tool-raise")) { OutAction = EWMFirstPersonInteractionAction::ToolRaise; return true; }
    if (ActionId == TEXT("tool-lower")) { OutAction = EWMFirstPersonInteractionAction::ToolLower; return true; }
    if (ActionId == TEXT("scan-anticipate") || ActionId == TEXT("scan-hold") || ActionId == TEXT("scan-settle")) { OutAction = EWMFirstPersonInteractionAction::Scan; return true; }
    if (ActionId == TEXT("build-point")) { OutAction = EWMFirstPersonInteractionAction::BuildPoint; return true; }
    if (ActionId == TEXT("build-confirm")) { OutAction = EWMFirstPersonInteractionAction::BuildConfirm; return true; }
    if (ActionId == TEXT("measure-focus")) { OutAction = EWMFirstPersonInteractionAction::MeasureFocus; return true; }
    if (ActionId == TEXT("observe-focus")) { OutAction = EWMFirstPersonInteractionAction::ObserveFocus; return true; }
    if (ActionId == TEXT("idle")) { OutAction = EWMFirstPersonInteractionAction::Idle; return true; }
    return false;
}

EWMFirstPersonInteractionAction FWMFirstPersonInteractionRuntime::DefaultActionForModeId(const FName ModeId)
{
    if (ModeId == TEXT("firstperson.build")) return EWMFirstPersonInteractionAction::BuildPoint;
    if (ModeId == TEXT("firstperson.scan")) return EWMFirstPersonInteractionAction::Scan;
    if (ModeId == TEXT("firstperson.measure")) return EWMFirstPersonInteractionAction::MeasureFocus;
    if (ModeId == TEXT("firstperson.observe")) return EWMFirstPersonInteractionAction::ObserveFocus;
    return EWMFirstPersonInteractionAction::ToolRaise;
}
