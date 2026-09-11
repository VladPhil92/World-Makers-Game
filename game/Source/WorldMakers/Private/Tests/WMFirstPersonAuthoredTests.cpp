#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Visual/WMFirstPersonAuthoredRuntime.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMFirstPersonAuthoredPathsTest,
    "WorldMakers.Visual.FirstPersonAuthored.PathsMatchStableSlots",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMFirstPersonAuthoredPathsTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("Arms path is stable"), FWMFirstPersonAuthoredRuntime::ArmsAssetPath().Contains(TEXT("SK_WM_FirstPersonArms")));
    TestTrue(TEXT("Scanner path is stable"), FWMFirstPersonAuthoredRuntime::ToolAssetPathForMode(TEXT("firstperson.scan")).Contains(TEXT("SM_WM_Scanner")));
    TestTrue(TEXT("Build tool path is stable"), FWMFirstPersonAuthoredRuntime::ToolAssetPathForMode(TEXT("firstperson.build")).Contains(TEXT("SM_WM_BuildTool")));
    TestTrue(TEXT("Measure tool path is stable"), FWMFirstPersonAuthoredRuntime::ToolAssetPathForMode(TEXT("firstperson.measure")).Contains(TEXT("SM_WM_MeasureTool")));
    TestTrue(TEXT("Wrist path is stable"), FWMFirstPersonAuthoredRuntime::WristAssetPath().Contains(TEXT("SM_WM_WristDevice")));
    TestTrue(TEXT("Observe has no authored tool"), FWMFirstPersonAuthoredRuntime::ToolAssetPathForMode(TEXT("firstperson.observe")).IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMFirstPersonAuthoredAllOrProxyTest,
    "WorldMakers.Visual.FirstPersonAuthored.TakeoverIsAllOrProxy",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMFirstPersonAuthoredAllOrProxyTest::RunTest(const FString& Parameters)
{
    FWMFirstPersonAuthoredAvailability Availability;
    Availability.bArms = true;
    Availability.bScanner = true;
    Availability.bBuildTool = true;
    Availability.bMeasureTool = true;
    Availability.bWristDevice = true;
    Availability.AnimationCount = FWMFirstPersonAuthoredRuntime::RequiredAnimationCount - 1;
    TestFalse(TEXT("Eight animations cannot take over"), FWMFirstPersonAuthoredRuntime::CanTakeOver(Availability, true));

    Availability.AnimationCount = FWMFirstPersonAuthoredRuntime::RequiredAnimationCount;
    TestTrue(TEXT("Complete authored set can take over when explicitly enabled"), FWMFirstPersonAuthoredRuntime::CanTakeOver(Availability, true));
    TestFalse(TEXT("Complete set still requires explicit enablement"), FWMFirstPersonAuthoredRuntime::CanTakeOver(Availability, false));

    Availability.bWristDevice = false;
    TestFalse(TEXT("One missing asset restores proxy path"), FWMFirstPersonAuthoredRuntime::CanTakeOver(Availability, true));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMFirstPersonAuthoredAnimationContractTest,
    "WorldMakers.Visual.FirstPersonAuthored.AnimationContractIsComplete",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMFirstPersonAuthoredAnimationContractTest::RunTest(const FString& Parameters)
{
    const TArray<FName> Actions = FWMFirstPersonAuthoredRuntime::RequiredActionIds();
    TestEqual(TEXT("Exactly nine first-person authored actions are required"), Actions.Num(), 9);
    for (const FName ActionId : Actions)
    {
        TestFalse(TEXT("Every required action has a stable asset path"), FWMFirstPersonAuthoredRuntime::AnimationAssetPath(ActionId).IsEmpty());
    }
    TestTrue(TEXT("Scan hold is the only looping authored action"), FWMFirstPersonAuthoredRuntime::IsLoopingAction(TEXT("scan-hold")));
    TestFalse(TEXT("Build confirm is not looping"), FWMFirstPersonAuthoredRuntime::IsLoopingAction(TEXT("build-confirm")));
    return true;
}

#endif
