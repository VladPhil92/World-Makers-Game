#pragma once

#include "CoreMinimal.h"

/** Asset-agnostic locomotion states consumed by the procedural V5 pose and future AnimBP. */
enum class EWMLocomotionState : uint8
{
    Idle,
    Start,
    Walk,
    Run,
    Stop,
    TurnInPlace,
    Jump,
    Fall,
    Land
};

/** Stable semantic interaction actions. These describe intent, never commerce or reward pressure. */
enum class EWMCharacterInteractionAction : uint8
{
    None,
    BuildPlace,
    BuildRemove,
    BuildMove,
    Measure,
    Observe,
    Inspect,
    Pickup,
    ScienceManipulate
};

struct WORLDMAKERS_API FWMCharacterAnimationInput
{
    float DeltaSeconds = 0.0f;
    float SpeedCmPerSec = 0.0f;
    float MaxSpeedCmPerSec = 450.0f;
    float VerticalVelocityCmPerSec = 0.0f;
    float TurnInput = 0.0f;
    float LookYawDegrees = 0.0f;
    float LookPitchDegrees = 0.0f;
    bool bAirborne = false;
};

/**
 * Compact source-visible pose vocabulary. Production skeletal animation can ignore these transforms and
 * consume the same runtime state/weights through UWMCharacterAnimationComponent.
 */
struct WORLDMAKERS_API FWMCharacterAnimationPose
{
    float RootOffsetZCm = 0.0f;
    float BodyLeanDegrees = 0.0f;
    float PelvisYawDegrees = 0.0f;
    float ChestYawDegrees = 0.0f;
    float HeadYawDegrees = 0.0f;
    float HeadPitchDegrees = 0.0f;

    float UpperArmLeftPitchDegrees = 0.0f;
    float UpperArmRightPitchDegrees = 0.0f;
    float LowerArmLeftPitchDegrees = 0.0f;
    float LowerArmRightPitchDegrees = 0.0f;
    float HandLeftPitchDegrees = 0.0f;
    float HandRightPitchDegrees = 0.0f;

    float ThighLeftPitchDegrees = 0.0f;
    float ThighRightPitchDegrees = 0.0f;
    float CalfLeftPitchDegrees = 0.0f;
    float CalfRightPitchDegrees = 0.0f;
    float FootLeftPitchDegrees = 0.0f;
    float FootRightPitchDegrees = 0.0f;

    float InteractionAlpha = 0.0f;
    float LandingAlpha = 0.0f;

    bool IsBounded() const;
};

/**
 * Deterministic animation state machine independent from animation assets, networking and rendering.
 * CharacterMovementComponent remains movement authority; this model only derives presentation state.
 */
class WORLDMAKERS_API FWMCharacterAnimationRuntime
{
public:
    void Reset();
    FWMCharacterAnimationPose Update(const FWMCharacterAnimationInput& Input);
    void TriggerInteraction(EWMCharacterInteractionAction Action, float DurationSeconds = 0.45f);

    EWMLocomotionState GetLocomotionState() const { return LocomotionState; }
    EWMCharacterInteractionAction GetInteractionAction() const { return InteractionAction; }
    FName GetLocomotionStateId() const;
    FName GetInteractionActionId() const;
    float GetSpeedAlpha() const { return SpeedAlpha; }
    float GetInteractionAlpha() const { return InteractionAlpha; }
    float GetStateAgeSeconds() const { return StateAgeSeconds; }
    float GetLookYawDegrees() const { return SmoothedLookYawDegrees; }
    float GetLookPitchDegrees() const { return SmoothedLookPitchDegrees; }
    bool IsAirborne() const { return bAirborne; }

    static FName LocomotionStateToId(EWMLocomotionState State);
    static FName InteractionActionToId(EWMCharacterInteractionAction Action);
    static bool TryParseInteractionAction(FName ActionId, EWMCharacterInteractionAction& OutAction);

private:
    void SetLocomotionState(EWMLocomotionState NewState);
    FWMCharacterAnimationPose EvaluatePose(float DeltaSeconds) const;

    EWMLocomotionState LocomotionState = EWMLocomotionState::Idle;
    EWMCharacterInteractionAction InteractionAction = EWMCharacterInteractionAction::None;

    float SpeedAlpha = 0.0f;
    float PreviousSpeedAlpha = 0.0f;
    float GaitPhaseRadians = 0.0f;
    float StateAgeSeconds = 0.0f;
    float StartTimerSeconds = 0.0f;
    float StopTimerSeconds = 0.0f;
    float LandTimerSeconds = 0.0f;
    float InteractionRemainingSeconds = 0.0f;
    float InteractionDurationSeconds = 0.0f;
    float InteractionAlpha = 0.0f;
    float TurnInput = 0.0f;
    float SmoothedLookYawDegrees = 0.0f;
    float SmoothedLookPitchDegrees = 0.0f;
    float VerticalVelocityCmPerSec = 0.0f;
    bool bAirborne = false;
    bool bWasAirborne = false;
};
