#if WITH_DEV_AUTOMATION_TESTS

#include "Building/WMBuildGridLibrary.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMBuildGridSnapTest,
    "WorldMakers.Building.Grid.SnapLocation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMBuildGridSnapTest::RunTest(const FString& Parameters)
{
    const FVector Snapped = UWMBuildGridLibrary::SnapLocationToGrid(FVector(149.0, 251.0, 0.0), 100.0f, true);
    TestEqual(TEXT("X snaps to nearest cell"), Snapped.X, 100.0);
    TestEqual(TEXT("Y snaps to nearest cell"), Snapped.Y, 300.0);
    TestEqual(TEXT("Z offsets half a cell for a cube resting on a surface"), Snapped.Z, 50.0);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMBuildRotationSnapTest,
    "WorldMakers.Building.Grid.SnapRotation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMBuildRotationSnapTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("Yaw snaps to 90-degree step"), UWMBuildGridLibrary::SnapYawToStep(91.0f, 90.0f), 90.0f);
    TestEqual(TEXT("Yaw wraps before snapping"), UWMBuildGridLibrary::SnapYawToStep(449.0f, 90.0f), 90.0f);

    return true;
}

#endif
