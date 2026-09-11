#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Visual/WMPresentationRuntime.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMPresentationCameraProfilesTest,
    "WorldMakers.Visual.Presentation.CameraProfilesRemainBounded",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMPresentationCameraProfilesTest::RunTest(const FString& Parameters)
{
    for (const EWMPresentationCameraMode Mode : {
        EWMPresentationCameraMode::Explore,
        EWMPresentationCameraMode::Build,
        EWMPresentationCameraMode::Observe,
        EWMPresentationCameraMode::Science,
        EWMPresentationCameraMode::Dialogue,
        EWMPresentationCameraMode::AdventureReveal})
    {
        FWMPresentationCameraProfile Profile;
        TestTrue(TEXT("Each camera mode resolves"), FWMPresentationRuntime::ResolveCameraProfile(Mode, false, Profile));
        TestTrue(TEXT("Each camera profile remains sane"), Profile.IsSane());
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMPresentationReducedMotionTest,
    "WorldMakers.Visual.Presentation.ReducedMotionConstrainsCamera",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMPresentationReducedMotionTest::RunTest(const FString& Parameters)
{
    FWMPresentationCameraProfile Explore;
    FWMPresentationRuntime::ResolveCameraProfile(EWMPresentationCameraMode::Explore, true, Explore);

    FWMPresentationCameraProfile Reveal;
    TestTrue(TEXT("Reduced reveal resolves"), FWMPresentationRuntime::ResolveCameraProfile(EWMPresentationCameraMode::AdventureReveal, true, Reveal));
    TestTrue(TEXT("Arm delta limited"), FMath::Abs(Reveal.ArmLengthCm - Explore.ArmLengthCm) <= 80.0f + KINDA_SMALL_NUMBER);
    TestTrue(TEXT("FOV delta limited"), FMath::Abs(Reveal.FieldOfViewDegrees - Explore.FieldOfViewDegrees) <= 3.0f + KINDA_SMALL_NUMBER);
    TestTrue(TEXT("Offset delta limited"), FMath::Abs(Reveal.TargetOffsetCm.Z - Explore.TargetOffsetCm.Z) <= 16.0f + KINDA_SMALL_NUMBER);
    TestTrue(TEXT("Blend limited"), Reveal.BlendSeconds <= 0.18f + KINDA_SMALL_NUMBER);
    TestTrue(TEXT("Hold limited"), Reveal.HoldSeconds <= 0.70f + KINDA_SMALL_NUMBER);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMPresentationSemanticRoutingTest,
    "WorldMakers.Visual.Presentation.SemanticEventsSelectContext",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMPresentationSemanticRoutingTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("Build event routes to Build"), FWMPresentationRuntime::ResolveModeForSemanticEvent(TEXT("gameplay.build.place")) == EWMPresentationCameraMode::Build);
    TestTrue(TEXT("Observation routes to Observe"), FWMPresentationRuntime::ResolveModeForSemanticEvent(TEXT("world.observe.reveal")) == EWMPresentationCameraMode::Observe);
    TestTrue(TEXT("Science routes to Science"), FWMPresentationRuntime::ResolveModeForSemanticEvent(TEXT("science.physics.force")) == EWMPresentationCameraMode::Science);
    TestTrue(TEXT("Fantasy routes to reveal"), FWMPresentationRuntime::ResolveModeForSemanticEvent(TEXT("fantasy.portal.open")) == EWMPresentationCameraMode::AdventureReveal);
    TestTrue(TEXT("Unknown remains Explore"), FWMPresentationRuntime::ResolveModeForSemanticEvent(TEXT("unknown.event")) == EWMPresentationCameraMode::Explore);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMPresentationUIMotionTest,
    "WorldMakers.Visual.Presentation.UIMotionPreservesMeaningWithoutTravel",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMPresentationUIMotionTest::RunTest(const FString& Parameters)
{
    const FWMUIMotionPose Standard = FWMPresentationRuntime::EvaluateUIMotion(0.12f, 1.0f, false);
    const FWMUIMotionPose Reduced = FWMPresentationRuntime::EvaluateUIMotion(0.12f, 1.0f, true);
    TestTrue(TEXT("Standard pose sane"), Standard.IsSane());
    TestTrue(TEXT("Reduced pose sane"), Reduced.IsSane());
    TestTrue(TEXT("Standard entrance can travel"), Standard.TranslationYPx > 0.0f);
    TestEqual(TEXT("Reduced motion removes UI translation"), Reduced.TranslationYPx, 0.0f);
    TestEqual(TEXT("Reduced motion removes UI scale motion"), Reduced.Scale, 1.0f);
    TestTrue(TEXT("Reduced cue remains visible"), Reduced.Opacity > 0.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMPresentationBeatContractTest,
    "WorldMakers.Visual.Presentation.MicrobeatsStayShortAndStable",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMPresentationBeatContractTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("Mission cue supported"), FWMPresentationRuntime::IsSupportedCue(TEXT("presentation.mission.changed")));
    TestTrue(TEXT("Science cue supported"), FWMPresentationRuntime::IsSupportedCue(TEXT("presentation.science.focus")));
    TestFalse(TEXT("Unknown cue rejected"), FWMPresentationRuntime::IsSupportedCue(TEXT("presentation.purchase.celebration")));
    TestEqual(TEXT("Long standard pulse is capped"), FWMPresentationRuntime::ClampPulseDuration(10.0f, false), 2.5f);
    TestEqual(TEXT("Reduced pulse is capped lower"), FWMPresentationRuntime::ClampPulseDuration(10.0f, true), 0.85f);
    return true;
}

#endif
