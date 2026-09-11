#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Visual/WMFirstPersonInteractionRuntime.h"
#include "Visual/WMReferenceVisualPolishRuntime.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMFirstPersonInteractionActionSequenceTest,
    "WorldMakers.Visual.FirstPersonKit.ActionSequenceRemainsBounded",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMFirstPersonInteractionActionSequenceTest::RunTest(const FString& Parameters)
{
    FWMFirstPersonInteractionRuntime Runtime;
    Runtime.Trigger(EWMFirstPersonInteractionAction::BuildConfirm, 0.80f);
    const FWMFirstPersonInteractionPose Pose = Runtime.Update(0.10f, false);

    TestTrue(TEXT("Build confirmation remains active"), Runtime.IsActive());
    TestEqual(TEXT("Stable build action id"), Runtime.GetActionId(), FName(TEXT("build-confirm")));
    TestTrue(TEXT("First-person pose is bounded"), Pose.IsSane());
    TestTrue(TEXT("Interaction exposes a visible action envelope"), Pose.ActionAlpha > 0.0f);
    TestTrue(TEXT("Context panel is visible during the interaction"), Pose.ContextAlpha > 0.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMFirstPersonInteractionReducedMotionTest,
    "WorldMakers.Visual.FirstPersonKit.ReducedMotionPreservesStateWithoutBob",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMFirstPersonInteractionReducedMotionTest::RunTest(const FString& Parameters)
{
    FWMFirstPersonInteractionRuntime Normal;
    FWMFirstPersonInteractionRuntime Reduced;
    Normal.Trigger(EWMFirstPersonInteractionAction::Scan, 0.90f);
    Reduced.Trigger(EWMFirstPersonInteractionAction::Scan, 0.90f);

    const FWMFirstPersonInteractionPose NormalPose = Normal.Update(0.12f, false);
    const FWMFirstPersonInteractionPose ReducedPose = Reduced.Update(0.12f, true);

    TestTrue(TEXT("Normal scan has bounded viewmodel motion"), FMath::Abs(NormalPose.ViewModelBobCm) > 0.0f);
    TestEqual(TEXT("Reduced Motion removes viewmodel bob"), ReducedPose.ViewModelBobCm, 0.0f);
    TestTrue(TEXT("Reduced Motion preserves the context information state"), ReducedPose.ContextAlpha > 0.0f);
    TestTrue(TEXT("Reduced Motion pose remains bounded"), ReducedPose.IsSane());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMFirstPersonInteractionVocabularyTest,
    "WorldMakers.Visual.FirstPersonKit.ActionVocabularyMatchesReferenceContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMFirstPersonInteractionVocabularyTest::RunTest(const FString& Parameters)
{
    for (const FName ActionId : {
        FName(TEXT("tool-raise")),
        FName(TEXT("tool-lower")),
        FName(TEXT("scan-anticipate")),
        FName(TEXT("scan-hold")),
        FName(TEXT("scan-settle")),
        FName(TEXT("build-point")),
        FName(TEXT("build-confirm")),
        FName(TEXT("measure-focus")),
        FName(TEXT("observe-focus"))})
    {
        EWMFirstPersonInteractionAction Action;
        TestTrue(*FString::Printf(TEXT("Action parses: %s"), *ActionId.ToString()), FWMFirstPersonInteractionRuntime::TryParseAction(ActionId, Action));
    }

    TestEqual(
        TEXT("Build mode defaults to build-point"),
        FWMFirstPersonInteractionRuntime::ActionToId(FWMFirstPersonInteractionRuntime::DefaultActionForModeId(TEXT("firstperson.build"))),
        FName(TEXT("build-point")));
    TestEqual(
        TEXT("Measure mode defaults to measure-focus"),
        FWMFirstPersonInteractionRuntime::ActionToId(FWMFirstPersonInteractionRuntime::DefaultActionForModeId(TEXT("firstperson.measure"))),
        FName(TEXT("measure-focus")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMFirstPersonInteractionSemanticRoutingTest,
    "WorldMakers.Visual.FirstPersonKit.SemanticEventsResolveToContextModes",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMFirstPersonInteractionSemanticRoutingTest::RunTest(const FString& Parameters)
{
    TestEqual(
        TEXT("Build semantic event resolves Build mode"),
        FWMReferenceVisualPolishRuntime::FirstPersonModeToId(FWMReferenceVisualPolishRuntime::ResolveModeForSemanticEvent(TEXT("gameplay.build.confirm"))),
        FName(TEXT("firstperson.build")));
    TestEqual(
        TEXT("Science semantic event resolves Scan mode"),
        FWMReferenceVisualPolishRuntime::FirstPersonModeToId(FWMReferenceVisualPolishRuntime::ResolveModeForSemanticEvent(TEXT("science.chemistry.observe"))),
        FName(TEXT("firstperson.scan")));
    TestEqual(
        TEXT("Measurement semantic event resolves Measure mode"),
        FWMReferenceVisualPolishRuntime::FirstPersonModeToId(FWMReferenceVisualPolishRuntime::ResolveModeForSemanticEvent(TEXT("mission.measure.reveal"))),
        FName(TEXT("firstperson.measure")));
    TestEqual(
        TEXT("Observation semantic event resolves Observe mode"),
        FWMReferenceVisualPolishRuntime::FirstPersonModeToId(FWMReferenceVisualPolishRuntime::ResolveModeForSemanticEvent(TEXT("world.observe.reveal"))),
        FName(TEXT("firstperson.observe")));
    return true;
}

#endif
