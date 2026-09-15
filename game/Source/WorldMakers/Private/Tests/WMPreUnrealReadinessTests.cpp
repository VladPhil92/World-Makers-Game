#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Accessibility/WMAccessibilitySubsystem.h"
#include "Audio/WMAudioReadinessSubsystem.h"
#include "Input/WMInputReadinessLibrary.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMPreUnrealReadinessContractsTest,
    "WorldMakers.PreUnreal.SourceContracts",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMPreUnrealReadinessContractsTest::RunTest(const FString& Parameters)
{
    TestTrue(
        TEXT("Canonical rainforest ambience cue is registered"),
        UWMAudioReadinessSubsystem::IsCanonicalCueId(TEXT("audio.ambience.rainforest.day")));
    TestFalse(
        TEXT("Unknown audio cue is rejected"),
        UWMAudioReadinessSubsystem::IsCanonicalCueId(TEXT("audio.unknown")));

    const TArray<FName> ActionIds = UWMInputReadinessLibrary::GetCanonicalActionIds();
    TestEqual(TEXT("Canonical action count"), ActionIds.Num(), 16);
    TestTrue(TEXT("Interact action exists"), ActionIds.Contains(TEXT("input.interact")));
    TestTrue(TEXT("Pause action exists"), ActionIds.Contains(TEXT("input.pause")));

    const TArray<FWMTouchControlSpec> TouchLayout = UWMInputReadinessLibrary::GetDefaultTabletTouchLayout();
    TestTrue(TEXT("Tablet touch layout has core controls"), TouchLayout.Num() >= 7);

    const FWMAccessibilitySettings Defaults;
    TestTrue(TEXT("Subtitles enabled by default"), Defaults.bSubtitlesEnabled);
    TestEqual(TEXT("Default text scale"), Defaults.TextScale, 1.0f);
    TestEqual(TEXT("Default camera shake scale"), Defaults.CameraShakeScale, 1.0f);

    return true;
}

#endif
