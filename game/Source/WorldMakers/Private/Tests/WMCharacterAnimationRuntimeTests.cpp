#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Visual/WMCharacterAnimationRuntime.h"

namespace WMAnimationTest
{
    FWMCharacterAnimationInput GroundInput(const float Speed, const float DeltaSeconds = 0.02f)
    {
        FWMCharacterAnimationInput Input;
        Input.DeltaSeconds = DeltaSeconds;
        Input.SpeedCmPerSec = Speed;
        Input.MaxSpeedCmPerSec = 450.0f;
        return Input;
    }

    void Advance(FWMCharacterAnimationRuntime& Runtime, const FWMCharacterAnimationInput& Input, const int32 Frames)
    {
        for (int32 Index = 0; Index < Frames; ++Index)
        {
            Runtime.Update(Input);
        }
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMCharacterAnimationLocomotionStateTest,
    "WorldMakers.Visual.Animation.LocomotionTransitionsAreDeterministic",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMCharacterAnimationLocomotionStateTest::RunTest(const FString& Parameters)
{
    FWMCharacterAnimationRuntime Runtime;
    Runtime.Reset();

    Runtime.Update(WMAnimationTest::GroundInput(0.0f));
    TestEqual(TEXT("Starts idle"), Runtime.GetLocomotionStateId(), FName(TEXT("locomotion.idle")));

    Runtime.Update(WMAnimationTest::GroundInput(160.0f));
    TestEqual(TEXT("First movement frame is start"), Runtime.GetLocomotionStateId(), FName(TEXT("locomotion.start")));

    WMAnimationTest::Advance(Runtime, WMAnimationTest::GroundInput(160.0f), 10);
    TestEqual(TEXT("Settles into walk"), Runtime.GetLocomotionStateId(), FName(TEXT("locomotion.walk")));

    Runtime.Update(WMAnimationTest::GroundInput(400.0f));
    TestEqual(TEXT("High normalized speed selects run"), Runtime.GetLocomotionStateId(), FName(TEXT("locomotion.run")));

    Runtime.Update(WMAnimationTest::GroundInput(0.0f));
    TestEqual(TEXT("Movement release produces stop"), Runtime.GetLocomotionStateId(), FName(TEXT("locomotion.stop")));

    WMAnimationTest::Advance(Runtime, WMAnimationTest::GroundInput(0.0f), 10);
    TestEqual(TEXT("Stop settles to idle"), Runtime.GetLocomotionStateId(), FName(TEXT("locomotion.idle")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMCharacterAnimationAirborneStateTest,
    "WorldMakers.Visual.Animation.JumpFallLandSequenceIsExplicit",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMCharacterAnimationAirborneStateTest::RunTest(const FString& Parameters)
{
    FWMCharacterAnimationRuntime Runtime;
    Runtime.Reset();

    FWMCharacterAnimationInput Input = WMAnimationTest::GroundInput(120.0f);
    Input.bAirborne = true;
    Input.VerticalVelocityCmPerSec = 320.0f;
    FWMCharacterAnimationPose Pose = Runtime.Update(Input);
    TestEqual(TEXT("Positive airborne velocity is jump"), Runtime.GetLocomotionStateId(), FName(TEXT("locomotion.jump")));
    TestTrue(TEXT("Jump pose is bounded"), Pose.IsBounded());

    Input.VerticalVelocityCmPerSec = -160.0f;
    Pose = Runtime.Update(Input);
    TestEqual(TEXT("Descending airborne velocity is fall"), Runtime.GetLocomotionStateId(), FName(TEXT("locomotion.fall")));
    TestTrue(TEXT("Fall pose is bounded"), Pose.IsBounded());

    Input.bAirborne = false;
    Input.VerticalVelocityCmPerSec = 0.0f;
    Input.SpeedCmPerSec = 0.0f;
    Pose = Runtime.Update(Input);
    TestEqual(TEXT("First grounded frame after air is land"), Runtime.GetLocomotionStateId(), FName(TEXT("locomotion.land")));
    TestTrue(TEXT("Land compression is active"), Pose.LandingAlpha > 0.0f);

    WMAnimationTest::Advance(Runtime, Input, 12);
    TestEqual(TEXT("Landing resolves to idle"), Runtime.GetLocomotionStateId(), FName(TEXT("locomotion.idle")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMCharacterAnimationInteractionLayerTest,
    "WorldMakers.Visual.Animation.InteractionLayerDoesNotOwnMovement",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMCharacterAnimationInteractionLayerTest::RunTest(const FString& Parameters)
{
    FWMCharacterAnimationRuntime Runtime;
    Runtime.Reset();
    Runtime.TriggerInteraction(EWMCharacterInteractionAction::Measure, 0.40f);

    const FWMCharacterAnimationInput Input = WMAnimationTest::GroundInput(180.0f, 0.05f);
    FWMCharacterAnimationPose Pose = Runtime.Update(Input);
    TestEqual(TEXT("Measure action is exposed"), Runtime.GetInteractionActionId(), FName(TEXT("interaction.measure")));
    TestTrue(TEXT("Interaction produces a bounded upper-body layer"), Pose.IsBounded());
    TestTrue(TEXT("Interaction alpha rises"), Pose.InteractionAlpha > 0.0f);

    WMAnimationTest::Advance(Runtime, Input, 5);
    TestTrue(TEXT("Locomotion continues while interaction plays"), Runtime.GetLocomotionStateId() == FName(TEXT("locomotion.walk")) || Runtime.GetLocomotionStateId() == FName(TEXT("locomotion.start")));

    WMAnimationTest::Advance(Runtime, Input, 8);
    TestEqual(TEXT("Interaction expires to none"), Runtime.GetInteractionActionId(), FName(TEXT("interaction.none")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMCharacterAnimationLookBoundsTest,
    "WorldMakers.Visual.Animation.LookAndPoseRemainBounded",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMCharacterAnimationLookBoundsTest::RunTest(const FString& Parameters)
{
    FWMCharacterAnimationRuntime Runtime;
    Runtime.Reset();

    FWMCharacterAnimationInput Input = WMAnimationTest::GroundInput(10000.0f, 0.10f);
    Input.LookYawDegrees = 9999.0f;
    Input.LookPitchDegrees = -9999.0f;
    Input.TurnInput = 99.0f;
    const FWMCharacterAnimationPose Pose = Runtime.Update(Input);

    TestTrue(TEXT("Runtime speed is normalized"), Runtime.GetSpeedAlpha() >= 0.0f && Runtime.GetSpeedAlpha() <= 1.0f);
    TestTrue(TEXT("Aim yaw is bounded"), FMath::Abs(Runtime.GetLookYawDegrees()) <= 60.0f);
    TestTrue(TEXT("Aim pitch is bounded"), FMath::Abs(Runtime.GetLookPitchDegrees()) <= 35.0f);
    TestTrue(TEXT("Final pose is bounded"), Pose.IsBounded());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMCharacterAnimationStableIdsTest,
    "WorldMakers.Visual.Animation.StableActionIdsRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMCharacterAnimationStableIdsTest::RunTest(const FString& Parameters)
{
    for (const EWMCharacterInteractionAction Action : {
        EWMCharacterInteractionAction::BuildPlace,
        EWMCharacterInteractionAction::BuildRemove,
        EWMCharacterInteractionAction::BuildMove,
        EWMCharacterInteractionAction::Measure,
        EWMCharacterInteractionAction::Observe,
        EWMCharacterInteractionAction::Inspect,
        EWMCharacterInteractionAction::Pickup,
        EWMCharacterInteractionAction::ScienceManipulate})
    {
        const FName Id = FWMCharacterAnimationRuntime::InteractionActionToId(Action);
        EWMCharacterInteractionAction Parsed = EWMCharacterInteractionAction::None;
        TestTrue(*FString::Printf(TEXT("Parses %s"), *Id.ToString()), FWMCharacterAnimationRuntime::TryParseInteractionAction(Id, Parsed));
        TestTrue(*FString::Printf(TEXT("Round-trips %s"), *Id.ToString()), Parsed == Action);
    }
    return true;
}

#endif
