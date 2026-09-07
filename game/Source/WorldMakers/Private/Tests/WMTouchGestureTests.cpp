#if WITH_DEV_AUTOMATION_TESTS

#include "Input/WMTouchGestureLibrary.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMTouchDragDeadZoneTest,
    "WorldMakers.Input.Touch.DragDeadZone",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMTouchDragDeadZoneTest::RunTest(const FString& Parameters)
{
    const FVector2D Start(100.0f, 100.0f);
    TestFalse(TEXT("Small touch jitter stays below drag threshold"),
        UWMTouchGestureLibrary::HasExceededDragDeadZone(Start, FVector2D(110.0f, 108.0f), 24.0f));
    TestTrue(TEXT("Intentional drag exceeds threshold"),
        UWMTouchGestureLibrary::HasExceededDragDeadZone(Start, FVector2D(130.0f, 100.0f), 24.0f));
    return true;
}

#endif
