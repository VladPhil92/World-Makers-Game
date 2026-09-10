#include "Visual/WMCharacterAnimationRuntime.h"

namespace WMCharacterAnimation
{
    constexpr float MovingThreshold = 0.06f;
    constexpr float RunThreshold = 0.62f;
    constexpr float StartDurationSeconds = 0.13f;
    constexpr float StopDurationSeconds = 0.14f;
    constexpr float LandDurationSeconds = 0.18f;

    float SafeDelta(const float Value)
    {
        return FMath::Clamp(FMath::IsFinite(Value) ? Value : 0.0f, 0.0f, 0.10f);
    }

    float SafeUnit(const float Value)
    {
        return FMath::Clamp(FMath::IsFinite(Value) ? Value : 0.0f, 0.0f, 1.0f);
    }
}

bool FWMCharacterAnimationPose::IsBounded() const
{
    const float* Values[] = {
        &RootOffsetZCm, &BodyLeanDegrees, &PelvisYawDegrees, &ChestYawDegrees, &HeadYawDegrees, &HeadPitchDegrees,
        &UpperArmLeftPitchDegrees, &UpperArmRightPitchDegrees, &LowerArmLeftPitchDegrees, &LowerArmRightPitchDegrees,
        &HandLeftPitchDegrees, &HandRightPitchDegrees, &ThighLeftPitchDegrees, &ThighRightPitchDegrees,
        &CalfLeftPitchDegrees, &CalfRightPitchDegrees, &FootLeftPitchDegrees, &FootRightPitchDegrees,
        &InteractionAlpha, &LandingAlpha
    };

    for (const float* Value : Values)
    {
        if (!FMath::IsFinite(*Value))
        {
            return false;
        }
    }

    return FMath::Abs(RootOffsetZCm) <= 8.0f
        && FMath::Abs(BodyLeanDegrees) <= 18.0f
        && FMath::Abs(PelvisYawDegrees) <= 18.0f
        && FMath::Abs(ChestYawDegrees) <= 24.0f
        && FMath::Abs(HeadYawDegrees) <= 60.0f
        && FMath::Abs(HeadPitchDegrees) <= 35.0f
        && FMath::Abs(UpperArmLeftPitchDegrees) <= 80.0f
        && FMath::Abs(UpperArmRightPitchDegrees) <= 80.0f
        && FMath::Abs(LowerArmLeftPitchDegrees) <= 80.0f
        && FMath::Abs(LowerArmRightPitchDegrees) <= 80.0f
        && FMath::Abs(ThighLeftPitchDegrees) <= 55.0f
        && FMath::Abs(ThighRightPitchDegrees) <= 55.0f
        && FMath::Abs(CalfLeftPitchDegrees) <= 55.0f
        && FMath::Abs(CalfRightPitchDegrees) <= 55.0f
        && FMath::Abs(FootLeftPitchDegrees) <= 35.0f
        && FMath::Abs(FootRightPitchDegrees) <= 35.0f
        && InteractionAlpha >= 0.0f && InteractionAlpha <= 1.0f
        && LandingAlpha >= 0.0f && LandingAlpha <= 1.0f;
}

void FWMCharacterAnimationRuntime::Reset()
{
    LocomotionState = EWMLocomotionState::Idle;
    InteractionAction = EWMCharacterInteractionAction::None;
    SpeedAlpha = 0.0f;
    PreviousSpeedAlpha = 0.0f;
    GaitPhaseRadians = 0.0f;
    StateAgeSeconds = 0.0f;
    StartTimerSeconds = 0.0f;
    StopTimerSeconds = 0.0f;
    LandTimerSeconds = 0.0f;
    InteractionRemainingSeconds = 0.0f;
    InteractionDurationSeconds = 0.0f;
    InteractionAlpha = 0.0f;
    TurnInput = 0.0f;
    SmoothedLookYawDegrees = 0.0f;
    SmoothedLookPitchDegrees = 0.0f;
    VerticalVelocityCmPerSec = 0.0f;
    bAirborne = false;
    bWasAirborne = false;
}

void FWMCharacterAnimationRuntime::SetLocomotionState(const EWMLocomotionState NewState)
{
    if (LocomotionState != NewState)
    {
        LocomotionState = NewState;
        StateAgeSeconds = 0.0f;
    }
}

void FWMCharacterAnimationRuntime::TriggerInteraction(const EWMCharacterInteractionAction Action, const float DurationSeconds)
{
    if (Action == EWMCharacterInteractionAction::None)
    {
        InteractionAction = EWMCharacterInteractionAction::None;
        InteractionRemainingSeconds = 0.0f;
        InteractionDurationSeconds = 0.0f;
        InteractionAlpha = 0.0f;
        return;
    }

    const float SafeDuration = FMath::Clamp(FMath::IsFinite(DurationSeconds) ? DurationSeconds : 0.45f, 0.18f, 1.25f);
    InteractionAction = Action;
    InteractionDurationSeconds = SafeDuration;
    InteractionRemainingSeconds = SafeDuration;
    InteractionAlpha = 0.0f;
}

FWMCharacterAnimationPose FWMCharacterAnimationRuntime::Update(const FWMCharacterAnimationInput& Input)
{
    const float DeltaSeconds = WMCharacterAnimation::SafeDelta(Input.DeltaSeconds);
    const float MaxSpeed = FMath::Max(FMath::IsFinite(Input.MaxSpeedCmPerSec) ? Input.MaxSpeedCmPerSec : 450.0f, 1.0f);
    const float Speed = FMath::Max(FMath::IsFinite(Input.SpeedCmPerSec) ? Input.SpeedCmPerSec : 0.0f, 0.0f);
    SpeedAlpha = FMath::Clamp(Speed / MaxSpeed, 0.0f, 1.0f);
    VerticalVelocityCmPerSec = FMath::IsFinite(Input.VerticalVelocityCmPerSec) ? Input.VerticalVelocityCmPerSec : 0.0f;
    TurnInput = FMath::Clamp(FMath::IsFinite(Input.TurnInput) ? Input.TurnInput : 0.0f, -1.0f, 1.0f);
    bAirborne = Input.bAirborne;

    const float TargetLookYaw = FMath::Clamp(FMath::IsFinite(Input.LookYawDegrees) ? Input.LookYawDegrees : 0.0f, -60.0f, 60.0f);
    const float TargetLookPitch = FMath::Clamp(FMath::IsFinite(Input.LookPitchDegrees) ? Input.LookPitchDegrees : 0.0f, -35.0f, 35.0f);
    SmoothedLookYawDegrees = FMath::FInterpTo(SmoothedLookYawDegrees, TargetLookYaw, DeltaSeconds, 10.0f);
    SmoothedLookPitchDegrees = FMath::FInterpTo(SmoothedLookPitchDegrees, TargetLookPitch, DeltaSeconds, 10.0f);

    if (InteractionAction != EWMCharacterInteractionAction::None && InteractionDurationSeconds > 0.0f)
    {
        InteractionRemainingSeconds = FMath::Max(0.0f, InteractionRemainingSeconds - DeltaSeconds);
        const float Progress = 1.0f - (InteractionRemainingSeconds / InteractionDurationSeconds);
        InteractionAlpha = FMath::Clamp(FMath::Sin(Progress * PI), 0.0f, 1.0f);
        if (InteractionRemainingSeconds <= KINDA_SMALL_NUMBER)
        {
            InteractionAction = EWMCharacterInteractionAction::None;
            InteractionDurationSeconds = 0.0f;
            InteractionAlpha = 0.0f;
        }
    }

    if (bAirborne)
    {
        StartTimerSeconds = 0.0f;
        StopTimerSeconds = 0.0f;
        LandTimerSeconds = 0.0f;
        SetLocomotionState(VerticalVelocityCmPerSec > 15.0f ? EWMLocomotionState::Jump : EWMLocomotionState::Fall);
    }
    else
    {
        if (bWasAirborne)
        {
            LandTimerSeconds = WMCharacterAnimation::LandDurationSeconds;
        }

        const bool bMoving = SpeedAlpha > WMCharacterAnimation::MovingThreshold;
        const bool bWasMoving = PreviousSpeedAlpha > WMCharacterAnimation::MovingThreshold;
        if (bMoving && !bWasMoving)
        {
            StartTimerSeconds = WMCharacterAnimation::StartDurationSeconds;
            StopTimerSeconds = 0.0f;
        }
        else if (!bMoving && bWasMoving)
        {
            StopTimerSeconds = WMCharacterAnimation::StopDurationSeconds;
            StartTimerSeconds = 0.0f;
        }

        if (LandTimerSeconds > 0.0f)
        {
            SetLocomotionState(EWMLocomotionState::Land);
            LandTimerSeconds = FMath::Max(0.0f, LandTimerSeconds - DeltaSeconds);
        }
        else if (StartTimerSeconds > 0.0f)
        {
            SetLocomotionState(EWMLocomotionState::Start);
            StartTimerSeconds = FMath::Max(0.0f, StartTimerSeconds - DeltaSeconds);
        }
        else if (StopTimerSeconds > 0.0f)
        {
            SetLocomotionState(EWMLocomotionState::Stop);
            StopTimerSeconds = FMath::Max(0.0f, StopTimerSeconds - DeltaSeconds);
        }
        else if (SpeedAlpha >= WMCharacterAnimation::RunThreshold)
        {
            SetLocomotionState(EWMLocomotionState::Run);
        }
        else if (bMoving)
        {
            SetLocomotionState(EWMLocomotionState::Walk);
        }
        else if (FMath::Abs(TurnInput) >= 0.35f)
        {
            SetLocomotionState(EWMLocomotionState::TurnInPlace);
        }
        else
        {
            SetLocomotionState(EWMLocomotionState::Idle);
        }
    }

    if (!bAirborne && SpeedAlpha > WMCharacterAnimation::MovingThreshold)
    {
        const float CyclesPerSecond = FMath::Lerp(1.35f, 2.45f, SpeedAlpha);
        GaitPhaseRadians = FMath::Fmod(GaitPhaseRadians + DeltaSeconds * CyclesPerSecond * 2.0f * PI, 2.0f * PI);
    }

    StateAgeSeconds += DeltaSeconds;
    const FWMCharacterAnimationPose Pose = EvaluatePose(DeltaSeconds);
    PreviousSpeedAlpha = SpeedAlpha;
    bWasAirborne = bAirborne;
    return Pose;
}

FWMCharacterAnimationPose FWMCharacterAnimationRuntime::EvaluatePose(const float DeltaSeconds) const
{
    FWMCharacterAnimationPose Pose;
    const float Gait = FMath::Sin(GaitPhaseRadians);
    const float Lift = FMath::Abs(FMath::Sin(GaitPhaseRadians * 2.0f));

    switch (LocomotionState)
    {
    case EWMLocomotionState::Idle:
        Pose.RootOffsetZCm = FMath::Sin(StateAgeSeconds * 2.1f) * 0.25f;
        break;
    case EWMLocomotionState::Start:
        Pose.BodyLeanDegrees = 6.0f;
        Pose.UpperArmLeftPitchDegrees = Gait * 12.0f;
        Pose.UpperArmRightPitchDegrees = -Pose.UpperArmLeftPitchDegrees;
        Pose.ThighLeftPitchDegrees = -Gait * 10.0f;
        Pose.ThighRightPitchDegrees = -Pose.ThighLeftPitchDegrees;
        break;
    case EWMLocomotionState::Walk:
    case EWMLocomotionState::Run:
    {
        const float RunBlend = FMath::Clamp((SpeedAlpha - 0.25f) / 0.75f, 0.0f, 1.0f);
        const float ArmAmplitude = FMath::Lerp(24.0f, 36.0f, RunBlend);
        const float LegAmplitude = FMath::Lerp(22.0f, 34.0f, RunBlend);
        Pose.RootOffsetZCm = Lift * FMath::Lerp(1.5f, 2.8f, RunBlend);
        Pose.BodyLeanDegrees = FMath::Lerp(2.5f, 7.0f, RunBlend);
        Pose.UpperArmLeftPitchDegrees = Gait * ArmAmplitude;
        Pose.UpperArmRightPitchDegrees = -Pose.UpperArmLeftPitchDegrees;
        Pose.ThighLeftPitchDegrees = -Gait * LegAmplitude;
        Pose.ThighRightPitchDegrees = -Pose.ThighLeftPitchDegrees;
        Pose.CalfLeftPitchDegrees = FMath::Max(0.0f, Gait) * 18.0f * SpeedAlpha;
        Pose.CalfRightPitchDegrees = FMath::Max(0.0f, -Gait) * 18.0f * SpeedAlpha;
        Pose.FootLeftPitchDegrees = -Pose.CalfLeftPitchDegrees * 0.35f;
        Pose.FootRightPitchDegrees = -Pose.CalfRightPitchDegrees * 0.35f;
        break;
    }
    case EWMLocomotionState::Stop:
        Pose.BodyLeanDegrees = -3.0f;
        Pose.ThighLeftPitchDegrees = 7.0f;
        Pose.ThighRightPitchDegrees = -4.0f;
        break;
    case EWMLocomotionState::TurnInPlace:
        Pose.PelvisYawDegrees = TurnInput * 9.0f;
        Pose.ChestYawDegrees = -TurnInput * 7.0f;
        Pose.UpperArmLeftPitchDegrees = TurnInput * 4.0f;
        Pose.UpperArmRightPitchDegrees = -TurnInput * 4.0f;
        break;
    case EWMLocomotionState::Jump:
        Pose.RootOffsetZCm = 1.5f;
        Pose.BodyLeanDegrees = 5.0f;
        Pose.UpperArmLeftPitchDegrees = -22.0f;
        Pose.UpperArmRightPitchDegrees = -22.0f;
        Pose.ThighLeftPitchDegrees = 14.0f;
        Pose.ThighRightPitchDegrees = 7.0f;
        Pose.CalfLeftPitchDegrees = 16.0f;
        Pose.CalfRightPitchDegrees = 10.0f;
        break;
    case EWMLocomotionState::Fall:
        Pose.BodyLeanDegrees = -2.0f;
        Pose.UpperArmLeftPitchDegrees = 15.0f;
        Pose.UpperArmRightPitchDegrees = 15.0f;
        Pose.ThighLeftPitchDegrees = -8.0f;
        Pose.ThighRightPitchDegrees = 8.0f;
        Pose.CalfLeftPitchDegrees = 12.0f;
        Pose.CalfRightPitchDegrees = 12.0f;
        break;
    case EWMLocomotionState::Land:
    {
        const float LandAlpha = FMath::Clamp(LandTimerSeconds / WMCharacterAnimation::LandDurationSeconds, 0.0f, 1.0f);
        Pose.LandingAlpha = LandAlpha;
        Pose.RootOffsetZCm = -3.2f * LandAlpha;
        Pose.BodyLeanDegrees = 4.0f * LandAlpha;
        Pose.ThighLeftPitchDegrees = -12.0f * LandAlpha;
        Pose.ThighRightPitchDegrees = -12.0f * LandAlpha;
        Pose.CalfLeftPitchDegrees = 22.0f * LandAlpha;
        Pose.CalfRightPitchDegrees = 22.0f * LandAlpha;
        break;
    }
    default:
        break;
    }

    Pose.HeadYawDegrees = SmoothedLookYawDegrees * 0.55f;
    Pose.HeadPitchDegrees = SmoothedLookPitchDegrees * 0.65f;

    const float A = InteractionAlpha;
    Pose.InteractionAlpha = A;
    switch (InteractionAction)
    {
    case EWMCharacterInteractionAction::BuildPlace:
        Pose.UpperArmRightPitchDegrees = FMath::Lerp(Pose.UpperArmRightPitchDegrees, -52.0f, A);
        Pose.LowerArmRightPitchDegrees = 48.0f * A;
        Pose.HandRightPitchDegrees = -12.0f * A;
        Pose.ChestYawDegrees += -8.0f * A;
        break;
    case EWMCharacterInteractionAction::BuildRemove:
        Pose.UpperArmRightPitchDegrees = FMath::Lerp(Pose.UpperArmRightPitchDegrees, -35.0f, A);
        Pose.LowerArmRightPitchDegrees = 62.0f * A;
        Pose.BodyLeanDegrees += -4.0f * A;
        break;
    case EWMCharacterInteractionAction::BuildMove:
    case EWMCharacterInteractionAction::Pickup:
        Pose.UpperArmLeftPitchDegrees = FMath::Lerp(Pose.UpperArmLeftPitchDegrees, -38.0f, A);
        Pose.UpperArmRightPitchDegrees = FMath::Lerp(Pose.UpperArmRightPitchDegrees, -38.0f, A);
        Pose.LowerArmLeftPitchDegrees = 55.0f * A;
        Pose.LowerArmRightPitchDegrees = 55.0f * A;
        Pose.BodyLeanDegrees += 4.0f * A;
        break;
    case EWMCharacterInteractionAction::Measure:
        Pose.UpperArmLeftPitchDegrees = FMath::Lerp(Pose.UpperArmLeftPitchDegrees, -48.0f, A);
        Pose.UpperArmRightPitchDegrees = FMath::Lerp(Pose.UpperArmRightPitchDegrees, -48.0f, A);
        Pose.LowerArmLeftPitchDegrees = 66.0f * A;
        Pose.LowerArmRightPitchDegrees = 66.0f * A;
        break;
    case EWMCharacterInteractionAction::Observe:
        Pose.UpperArmRightPitchDegrees = FMath::Lerp(Pose.UpperArmRightPitchDegrees, -30.0f, A);
        Pose.LowerArmRightPitchDegrees = 44.0f * A;
        Pose.HeadPitchDegrees += -8.0f * A;
        break;
    case EWMCharacterInteractionAction::Inspect:
        Pose.UpperArmLeftPitchDegrees = FMath::Lerp(Pose.UpperArmLeftPitchDegrees, -42.0f, A);
        Pose.UpperArmRightPitchDegrees = FMath::Lerp(Pose.UpperArmRightPitchDegrees, -42.0f, A);
        Pose.LowerArmLeftPitchDegrees = 58.0f * A;
        Pose.LowerArmRightPitchDegrees = 58.0f * A;
        Pose.HeadPitchDegrees += -10.0f * A;
        break;
    case EWMCharacterInteractionAction::ScienceManipulate:
        Pose.UpperArmLeftPitchDegrees = FMath::Lerp(Pose.UpperArmLeftPitchDegrees, -46.0f, A);
        Pose.UpperArmRightPitchDegrees = FMath::Lerp(Pose.UpperArmRightPitchDegrees, -54.0f, A);
        Pose.LowerArmLeftPitchDegrees = 48.0f * A;
        Pose.LowerArmRightPitchDegrees = 60.0f * A;
        Pose.HandLeftPitchDegrees = -8.0f * A;
        Pose.HandRightPitchDegrees = 8.0f * A;
        break;
    case EWMCharacterInteractionAction::None:
    default:
        break;
    }

    // Final clamps keep source poses safe even if future tuning data drifts.
    Pose.RootOffsetZCm = FMath::Clamp(Pose.RootOffsetZCm, -8.0f, 8.0f);
    Pose.BodyLeanDegrees = FMath::Clamp(Pose.BodyLeanDegrees, -18.0f, 18.0f);
    Pose.PelvisYawDegrees = FMath::Clamp(Pose.PelvisYawDegrees, -18.0f, 18.0f);
    Pose.ChestYawDegrees = FMath::Clamp(Pose.ChestYawDegrees, -24.0f, 24.0f);
    Pose.HeadYawDegrees = FMath::Clamp(Pose.HeadYawDegrees, -60.0f, 60.0f);
    Pose.HeadPitchDegrees = FMath::Clamp(Pose.HeadPitchDegrees, -35.0f, 35.0f);
    Pose.UpperArmLeftPitchDegrees = FMath::Clamp(Pose.UpperArmLeftPitchDegrees, -80.0f, 80.0f);
    Pose.UpperArmRightPitchDegrees = FMath::Clamp(Pose.UpperArmRightPitchDegrees, -80.0f, 80.0f);
    Pose.LowerArmLeftPitchDegrees = FMath::Clamp(Pose.LowerArmLeftPitchDegrees, -80.0f, 80.0f);
    Pose.LowerArmRightPitchDegrees = FMath::Clamp(Pose.LowerArmRightPitchDegrees, -80.0f, 80.0f);
    Pose.ThighLeftPitchDegrees = FMath::Clamp(Pose.ThighLeftPitchDegrees, -55.0f, 55.0f);
    Pose.ThighRightPitchDegrees = FMath::Clamp(Pose.ThighRightPitchDegrees, -55.0f, 55.0f);
    Pose.CalfLeftPitchDegrees = FMath::Clamp(Pose.CalfLeftPitchDegrees, -55.0f, 55.0f);
    Pose.CalfRightPitchDegrees = FMath::Clamp(Pose.CalfRightPitchDegrees, -55.0f, 55.0f);
    Pose.FootLeftPitchDegrees = FMath::Clamp(Pose.FootLeftPitchDegrees, -35.0f, 35.0f);
    Pose.FootRightPitchDegrees = FMath::Clamp(Pose.FootRightPitchDegrees, -35.0f, 35.0f);
    Pose.InteractionAlpha = WMCharacterAnimation::SafeUnit(Pose.InteractionAlpha);
    Pose.LandingAlpha = WMCharacterAnimation::SafeUnit(Pose.LandingAlpha);
    return Pose;
}

FName FWMCharacterAnimationRuntime::LocomotionStateToId(const EWMLocomotionState State)
{
    switch (State)
    {
    case EWMLocomotionState::Idle: return TEXT("locomotion.idle");
    case EWMLocomotionState::Start: return TEXT("locomotion.start");
    case EWMLocomotionState::Walk: return TEXT("locomotion.walk");
    case EWMLocomotionState::Run: return TEXT("locomotion.run");
    case EWMLocomotionState::Stop: return TEXT("locomotion.stop");
    case EWMLocomotionState::TurnInPlace: return TEXT("locomotion.turn-in-place");
    case EWMLocomotionState::Jump: return TEXT("locomotion.jump");
    case EWMLocomotionState::Fall: return TEXT("locomotion.fall");
    case EWMLocomotionState::Land: return TEXT("locomotion.land");
    default: return TEXT("locomotion.idle");
    }
}

FName FWMCharacterAnimationRuntime::InteractionActionToId(const EWMCharacterInteractionAction Action)
{
    switch (Action)
    {
    case EWMCharacterInteractionAction::BuildPlace: return TEXT("interaction.build-place");
    case EWMCharacterInteractionAction::BuildRemove: return TEXT("interaction.build-remove");
    case EWMCharacterInteractionAction::BuildMove: return TEXT("interaction.build-move");
    case EWMCharacterInteractionAction::Measure: return TEXT("interaction.measure");
    case EWMCharacterInteractionAction::Observe: return TEXT("interaction.observe");
    case EWMCharacterInteractionAction::Inspect: return TEXT("interaction.inspect");
    case EWMCharacterInteractionAction::Pickup: return TEXT("interaction.pickup");
    case EWMCharacterInteractionAction::ScienceManipulate: return TEXT("interaction.science-manipulate");
    case EWMCharacterInteractionAction::None:
    default: return TEXT("interaction.none");
    }
}

bool FWMCharacterAnimationRuntime::TryParseInteractionAction(const FName ActionId, EWMCharacterInteractionAction& OutAction)
{
    for (const EWMCharacterInteractionAction Candidate : {
        EWMCharacterInteractionAction::BuildPlace,
        EWMCharacterInteractionAction::BuildRemove,
        EWMCharacterInteractionAction::BuildMove,
        EWMCharacterInteractionAction::Measure,
        EWMCharacterInteractionAction::Observe,
        EWMCharacterInteractionAction::Inspect,
        EWMCharacterInteractionAction::Pickup,
        EWMCharacterInteractionAction::ScienceManipulate})
    {
        if (InteractionActionToId(Candidate) == ActionId)
        {
            OutAction = Candidate;
            return true;
        }
    }

    OutAction = EWMCharacterInteractionAction::None;
    return ActionId == InteractionActionToId(EWMCharacterInteractionAction::None);
}

FName FWMCharacterAnimationRuntime::GetLocomotionStateId() const
{
    return LocomotionStateToId(LocomotionState);
}

FName FWMCharacterAnimationRuntime::GetInteractionActionId() const
{
    return InteractionActionToId(InteractionAction);
}
