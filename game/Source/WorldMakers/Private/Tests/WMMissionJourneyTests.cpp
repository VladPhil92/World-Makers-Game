#include "Misc/AutomationTest.h"
#include "Mission/WMMissionJourneySaveGame.h"
#include "Mission/WMMissionTypes.h"

namespace
{
    FWMMissionRuntimeDefinition MakeJourneyDefinition(const FName MissionId, const TArray<FName>& Prerequisites, const TArray<FName>& Rewards)
    {
        FWMMissionRuntimeDefinition Definition;
        Definition.MissionId = MissionId;
        Definition.LearningObjectiveIds = { TEXT("math.measure.compare-lengths"), TEXT("math.spatial.plan-to-constraint") };
        Definition.RequiredEvidenceEventIds = { TEXT("measurement_used_before_build"), TEXT("structure_fits_target_span") };
        Definition.PrerequisiteMissionIds = Prerequisites;
        Definition.TargetSpanCm = 300.0f;
        Definition.ToleranceCm = 20.0f;
        Definition.RewardIds = Rewards;
        Definition.bPrototypeOnly = true;
        return Definition;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMMissionJourneyPrerequisiteTest,
    "WorldMakers.Missions.Journey.PrerequisiteUnlockOrder",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMMissionJourneyPrerequisiteTest::RunTest(const FString& Parameters)
{
    const FName MissionOne(TEXT("mission.mathematics.measure-and-build-01"));
    const FName MissionTwo(TEXT("mission.mathematics.measure-and-build-02"));
    const FWMMissionRuntimeDefinition First = MakeJourneyDefinition(MissionOne, {}, { TEXT("reward.math.measurement-tool-01") });
    const FWMMissionRuntimeDefinition Second = MakeJourneyDefinition(MissionTwo, { MissionOne }, { TEXT("reward.math.spatial-builder-badge-01") });

    FWMMissionJourneyModel Journey;
    TestTrue(TEXT("First mission is immediately activatable"), Journey.CanActivate(First));
    TestFalse(TEXT("Second mission is locked before prerequisite completion"), Journey.CanActivate(Second));
    TestTrue(TEXT("Second mission resolves as locked"), Journey.ResolveState(Second, NAME_None, EWMMissionRuntimeState::Inactive) == EWMJourneyMissionState::Locked);

    TArray<FName> NewRewards;
    TestTrue(TEXT("First completion is new"), Journey.ApplyCompletion(First, NewRewards));
    TestTrue(TEXT("Second mission unlocks after first completion"), Journey.CanActivate(Second));
    TestTrue(TEXT("Second mission resolves as available"), Journey.ResolveState(Second, NAME_None, EWMMissionRuntimeState::Inactive) == EWMJourneyMissionState::Available);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMMissionJourneyIdempotentRewardsTest,
    "WorldMakers.Missions.Journey.IdempotentRewardGrants",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMMissionJourneyIdempotentRewardsTest::RunTest(const FString& Parameters)
{
    const FWMMissionRuntimeDefinition Definition = MakeJourneyDefinition(
        TEXT("mission.mathematics.measure-and-build-01"),
        {},
        { TEXT("reward.math.measurement-tool-01"), TEXT("reward.math.spatial-builder-badge-01") });

    FWMMissionJourneyModel Journey;
    TArray<FName> FirstGrant;
    TArray<FName> ReplayGrant;
    TestTrue(TEXT("Initial completion is recorded"), Journey.ApplyCompletion(Definition, FirstGrant));
    TestEqual(TEXT("Initial completion grants both unique rewards"), FirstGrant.Num(), 2);
    TestFalse(TEXT("Replay is not a new completion"), Journey.ApplyCompletion(Definition, ReplayGrant));
    TestEqual(TEXT("Replay grants no duplicate rewards"), ReplayGrant.Num(), 0);
    TestEqual(TEXT("Lifetime reward ledger remains unique"), Journey.GrantedRewardIds.Num(), 2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMMissionJourneyExportRestoreTest,
    "WorldMakers.Missions.Journey.ExportRestoreStableIds",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMMissionJourneyExportRestoreTest::RunTest(const FString& Parameters)
{
    const FWMMissionRuntimeDefinition Definition = MakeJourneyDefinition(
        TEXT("mission.mathematics.measure-and-build-01"),
        {},
        { TEXT("reward.math.measurement-tool-01") });

    FWMMissionJourneyModel Original;
    TArray<FName> NewRewards;
    Original.ApplyCompletion(Definition, NewRewards);

    TArray<FName> CompletedIds;
    TArray<FName> RewardIds;
    Original.Export(CompletedIds, RewardIds);

    FWMMissionJourneyModel Restored;
    Restored.Restore(CompletedIds, RewardIds);
    TestTrue(TEXT("Completed MissionId survives export/restore"), Restored.CompletedMissionIds.Contains(Definition.MissionId));
    TestTrue(TEXT("Granted RewardId survives export/restore"), Restored.GrantedRewardIds.Contains(TEXT("reward.math.measurement-tool-01")));

    const UWMMissionJourneySaveGame* SaveDefaults = GetDefault<UWMMissionJourneySaveGame>();
    TestNotNull(TEXT("Journey SaveGame class is constructible"), SaveDefaults);
    TestEqual(TEXT("Journey save format is versioned"), SaveDefaults->FormatVersion, UWMMissionJourneySaveGame::CurrentFormatVersion);
    return true;
}
