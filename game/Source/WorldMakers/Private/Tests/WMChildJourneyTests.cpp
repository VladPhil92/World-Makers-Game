#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UI/WMChildJourneySubsystem.h"
#include "UI/WMChildJourneyTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMChildJourneyMissionStateMappingTest,
    "WorldMakers.UI.ChildJourney.StateMapping",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMChildJourneyMissionStateMappingTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("Locked mission stays out of the child panel"),
        FWMChildJourneyRules::FromMissionState(EWMJourneyMissionState::Locked), EWMChildAdventureState::Hidden);
    TestEqual(TEXT("Available mission becomes ready"),
        FWMChildJourneyRules::FromMissionState(EWMJourneyMissionState::Available), EWMChildAdventureState::Ready);
    TestEqual(TEXT("Active mission becomes in progress"),
        FWMChildJourneyRules::FromMissionState(EWMJourneyMissionState::Active), EWMChildAdventureState::InProgress);
    TestEqual(TEXT("Completed mission remains complete"),
        FWMChildJourneyRules::FromMissionState(EWMJourneyMissionState::Completed), EWMChildAdventureState::Complete);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMChildJourneyRevealOrderTest,
    "WorldMakers.UI.ChildJourney.ProgressiveReveal",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMChildJourneyRevealOrderTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("Future intervention remains hidden before prerequisites"),
        FWMChildJourneyRules::FromInterventionState(false, false, 0, 2), EWMChildAdventureState::Hidden);
    TestEqual(TEXT("Open intervention is ready without artificial task pressure"),
        FWMChildJourneyRules::FromInterventionState(false, true, 0, 2), EWMChildAdventureState::Ready);
    TestEqual(TEXT("Partially built intervention is in progress"),
        FWMChildJourneyRules::FromInterventionState(false, true, 1, 2), EWMChildAdventureState::InProgress);
    TestEqual(TEXT("Completed intervention remains complete"),
        FWMChildJourneyRules::FromInterventionState(true, true, 2, 2), EWMChildAdventureState::Complete);
    TestEqual(TEXT("Future creative piece remains hidden"),
        FWMChildJourneyRules::FromUnlockState(false, false), EWMChildAdventureState::Hidden);
    TestEqual(TEXT("Next creative piece can be previewed calmly"),
        FWMChildJourneyRules::FromUnlockState(false, true), EWMChildAdventureState::Ready);
    TestEqual(TEXT("Granted creative piece remains complete"),
        FWMChildJourneyRules::FromUnlockState(true, false), EWMChildAdventureState::Complete);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMChildJourneyFriendlyCopyTest,
    "WorldMakers.UI.ChildJourney.FriendlyLabelsHideTechnicalIds",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMChildJourneyFriendlyCopyTest::RunTest(const FString& Parameters)
{
    const FName MissionId(TEXT("mission.science.rainforest-ecosystem-01"));
    const FString FriendlyMission = UWMChildJourneySubsystem::ResolveChildTitle(MissionId).ToString();
    TestFalse(TEXT("Primary child title does not expose raw mission ID"), FriendlyMission.Contains(TEXT("mission.")));
    TestTrue(TEXT("Known mission has meaningful child-facing title"), FriendlyMission.Contains(TEXT("Rainforest")));

    const FName PieceId(TEXT("eco.bamboo-bridge"));
    const FString FriendlyPiece = UWMChildJourneySubsystem::ResolveChildTitle(PieceId).ToString();
    TestFalse(TEXT("Creative title does not expose raw piece ID"), FriendlyPiece.Contains(TEXT("eco.")));
    TestTrue(TEXT("Creative title identifies the piece"), FriendlyPiece.Contains(TEXT("Bamboo")));
    return true;
}

#endif
