#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Visual/WMPrototypeMotionStyle.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMPrototypeMotionIdleTest,
    "WorldMakers.Visual.Motion.IdlePoseIsStable",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMPrototypeMotionIdleTest::RunTest(const FString& Parameters)
{
    const FWMPrototypeMotionPose Pose = FWMPrototypeMotionStyle::Evaluate(0.0f, PI * 0.5f, false);
    TestTrue(TEXT("Idle arms do not swing"), FMath::IsNearlyZero(Pose.ArmSwingDegrees));
    TestTrue(TEXT("Idle legs do not swing"), FMath::IsNearlyZero(Pose.LegSwingDegrees));
    TestTrue(TEXT("Idle body does not bob"), FMath::IsNearlyZero(Pose.BodyBobCm));
    TestTrue(TEXT("Idle head does not bob"), FMath::IsNearlyZero(Pose.HeadBobCm));
    TestTrue(TEXT("Idle body does not lean"), FMath::IsNearlyZero(Pose.BodyLeanDegrees));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMPrototypeMotionLocomotionTest,
    "WorldMakers.Visual.Motion.LocomotionHasReadableOpposition",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMPrototypeMotionLocomotionTest::RunTest(const FString& Parameters)
{
    const FWMPrototypeMotionPose Pose = FWMPrototypeMotionStyle::Evaluate(1.0f, PI * 0.5f, false);
    TestTrue(TEXT("Arm swing is visible"), Pose.ArmSwingDegrees > 20.0f && Pose.ArmSwingDegrees <= 28.0f);
    TestTrue(TEXT("Leg swing opposes arm swing"), Pose.LegSwingDegrees < -18.0f && Pose.LegSwingDegrees >= -24.0f);
    TestTrue(TEXT("Lean remains restrained"), Pose.BodyLeanDegrees >= 0.0f && Pose.BodyLeanDegrees <= 4.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMPrototypeMotionBoundsTest,
    "WorldMakers.Visual.Motion.InputsRemainBounded",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMPrototypeMotionBoundsTest::RunTest(const FString& Parameters)
{
    const FWMPrototypeMotionPose Overspeed = FWMPrototypeMotionStyle::Evaluate(5.0f, PI * 0.5f, false);
    TestTrue(TEXT("Overspeed clamps arm amplitude"), FMath::Abs(Overspeed.ArmSwingDegrees) <= 28.0f);
    TestTrue(TEXT("Overspeed clamps leg amplitude"), FMath::Abs(Overspeed.LegSwingDegrees) <= 24.0f);

    const FWMPrototypeMotionPose NegativeSpeed = FWMPrototypeMotionStyle::Evaluate(-2.0f, PI * 0.5f, false);
    TestTrue(TEXT("Negative speed clamps to idle arm pose"), FMath::IsNearlyZero(NegativeSpeed.ArmSwingDegrees));
    TestTrue(TEXT("Negative speed clamps to idle body pose"), FMath::IsNearlyZero(NegativeSpeed.BodyBobCm));

    const FWMPrototypeMotionPose Airborne = FWMPrototypeMotionStyle::Evaluate(0.5f, 0.0f, true);
    TestTrue(TEXT("Airborne pose is distinct"), !FMath::IsNearlyZero(Airborne.ArmSwingDegrees));
    TestTrue(TEXT("Airborne pose remains bounded"), FMath::Abs(Airborne.BodyLeanDegrees) <= 6.0f);
    return true;
}

#endif
