#pragma once

#include "CoreMinimal.h"

enum class EWMFirstPersonInteractionAction : uint8
{
    Idle,
    ToolRaise,
    ToolLower,
    Scan,
    BuildPoint,
    BuildConfirm,
    MeasureFocus,
    ObserveFocus
};

struct WORLDMAKERS_API FWMFirstPersonInteractionPose
{
    FVector ToolTranslationCm = FVector::ZeroVector;
    FRotator ToolRotationDegrees = FRotator::ZeroRotator;
    FVector LeftHandTranslationCm = FVector::ZeroVector;
    FVector RightHandTranslationCm = FVector::ZeroVector;
    float ToolScale = 1.0f;
    float ContextAlpha = 0.0f;
    float ViewModelBobCm = 0.0f;
    float ActionAlpha = 0.0f;

    bool IsSane() const;
};

/**
 * Deterministic, presentation-only first-person interaction state machine.
 * It never grants gameplay outcomes and does not own movement, collision, mission or science authority.
 */
class WORLDMAKERS_API FWMFirstPersonInteractionRuntime
{
public:
    void Reset();
    void Trigger(EWMFirstPersonInteractionAction Action, float DurationSeconds);
    FWMFirstPersonInteractionPose Update(float DeltaSeconds, bool bReducedMotion);

    EWMFirstPersonInteractionAction GetAction() const { return Action; }
    FName GetActionId() const { return ActionToId(Action); }
    float GetRemainingSeconds() const { return RemainingSeconds; }
    bool IsActive() const { return Action != EWMFirstPersonInteractionAction::Idle && RemainingSeconds > 0.0f; }

    static FName ActionToId(EWMFirstPersonInteractionAction Action);
    static bool TryParseAction(FName ActionId, EWMFirstPersonInteractionAction& OutAction);
    static EWMFirstPersonInteractionAction DefaultActionForModeId(FName ModeId);

private:
    FWMFirstPersonInteractionPose EvaluatePose(bool bReducedMotion) const;

    EWMFirstPersonInteractionAction Action = EWMFirstPersonInteractionAction::Idle;
    float DurationSeconds = 0.0f;
    float RemainingSeconds = 0.0f;
    float AgeSeconds = 0.0f;
};
