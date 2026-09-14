#if WITH_DEV_AUTOMATION_TESTS

#include "Adventure/WMEpicRuntime.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
    FWMEpicEvidenceRequirement MakeEvidence(
        const TCHAR* RequirementId,
        const TCHAR* ObjectiveId,
        const TCHAR* DisciplineId,
        const TCHAR* ProducerKind,
        const TCHAR* ProducerRefId,
        const TCHAR* PrimitiveId,
        const TCHAR* EventId)
    {
        FWMEpicEvidenceRequirement Requirement;
        Requirement.RequirementId = RequirementId;
        Requirement.ObjectiveId = ObjectiveId;
        Requirement.DisciplineId = DisciplineId;
        Requirement.ProducerKind = ProducerKind;
        Requirement.ProducerRefId = ProducerRefId;
        Requirement.PrimitiveId = PrimitiveId;
        Requirement.EvidenceEventId = EventId;
        Requirement.RequiredCount = 1;
        return Requirement;
    }

    FWMEpicWorldStateRequirement MakeState(const TCHAR* StateId, const TCHAR* ProducerKind, const TCHAR* ProducerRefId)
    {
        FWMEpicWorldStateRequirement Requirement;
        Requirement.WorldStateId = StateId;
        Requirement.ProducerKind = ProducerKind;
        Requirement.ProducerRefId = ProducerRefId;
        return Requirement;
    }

    FWMEpicDefinition MakeTwoChapterEpic()
    {
        FWMEpicDefinition Epic;
        Epic.EpicId = TEXT("epic.test.gated");
        Epic.TitleKey = TEXT("epic.test.gated.title");
        Epic.PremiseKey = TEXT("epic.test.gated.premise");
        Epic.AgeBand = TEXT("9-10");
        Epic.Disciplines = { TEXT("mathematics"), TEXT("physics"), TEXT("english-language"), TEXT("philosophy-for-children") };

        FWMEpicChapterDefinition First;
        First.ChapterId = TEXT("chapter.test.first");
        First.MissionId = TEXT("mission.test.first");
        First.TitleKey = TEXT("chapter.test.first.title");
        First.PromptKey = TEXT("chapter.test.first.prompt");
        First.EvidenceRequirements = {
            MakeEvidence(TEXT("req.test.first"), TEXT("math.test.objective"), TEXT("mathematics"), TEXT("world"), TEXT("world.test.pattern"), TEXT("sequence-and-infer"), TEXT("test.pattern.inferred"))
        };
        First.WorldStateRequirements = { MakeState(TEXT("world-state.test.first-ready"), TEXT("world"), TEXT("world.test.machine")) };

        FWMEpicChapterDefinition Second;
        Second.ChapterId = TEXT("chapter.test.second");
        Second.MissionId = TEXT("mission.test.second");
        Second.TitleKey = TEXT("chapter.test.second.title");
        Second.PromptKey = TEXT("chapter.test.second.prompt");
        Second.EvidenceRequirements = {
            MakeEvidence(TEXT("req.test.second"), TEXT("physics.test.objective"), TEXT("physics"), TEXT("science"), TEXT("science.test.force"), TEXT("predict-test-revise"), TEXT("test.force.predicted"))
        };
        Second.WorldStateRequirements = { MakeState(TEXT("world-state.test.second-ready"), TEXT("science"), TEXT("science.test.machine")) };
        Epic.Chapters = { First, Second };
        return Epic;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMEpicCatalogBreadthTest,
    "WorldMakers.Epic.Catalog.CrossDisciplinaryBreadth",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMEpicCatalogBreadthTest::RunTest(const FString& Parameters)
{
    const FString Path = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("WorldMakers/Epics/cross-disciplinary-epics-v1.json"));
    FString Json;
    TestTrue(TEXT("Epic catalog file loads"), FFileHelper::LoadFileToString(Json, *Path));

    FWMEpicCatalog Catalog;
    FString Error;
    TestTrue(TEXT("Epic catalog parses and validates"), FWMEpicCatalog::TryParseJson(Json, Catalog, Error));
    TestTrue(TEXT("At least two epics exist"), Catalog.Epics.Num() >= 2);
    TestNotNull(TEXT("The Eclipse Engine exists"), Catalog.FindEpic(TEXT("epic.eclipse-engine")));
    TestNotNull(TEXT("The Garden at the End of Winter exists"), Catalog.FindEpic(TEXT("epic.garden-end-winter")));

    for (const FWMEpicDefinition& Epic : Catalog.Epics)
    {
        TSet<FName> AttributedDisciplines;
        for (const FWMEpicChapterDefinition& Chapter : Epic.Chapters)
            for (const FWMEpicEvidenceRequirement& Requirement : Chapter.EvidenceRequirements)
                AttributedDisciplines.Add(Requirement.DisciplineId);
        TestTrue(*FString::Printf(TEXT("%s has four or more evidence-attributed streams"), *Epic.EpicId.ToString()), AttributedDisciplines.Num() >= 4);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMEpicWorldStateGateTest,
    "WorldMakers.Epic.Runtime.WorldStateGate",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMEpicWorldStateGateTest::RunTest(const FString& Parameters)
{
    FWMEpicProgressModel Progress;
    TestTrue(TEXT("Epic begins"), Progress.Begin(MakeTwoChapterEpic()));
    TestEqual(TEXT("First chapter active"), Progress.GetCurrentChapter()->ChapterId, FName(TEXT("chapter.test.first")));
    TestFalse(TEXT("Cannot advance without evidence and world state"), Progress.AdvanceChapter());

    TestTrue(TEXT("Correct evidence accepted"), Progress.CommitEvidence(
        TEXT("math.test.objective"), TEXT("mathematics"), TEXT("world"), TEXT("world.test.pattern"), TEXT("sequence-and-infer"), TEXT("test.pattern.inferred")));
    TestFalse(TEXT("Evidence alone cannot advance chapter"), Progress.AdvanceChapter());
    TestFalse(TEXT("Spoofed world-state producer rejected"), Progress.CommitWorldState(
        TEXT("thought"), TEXT("world.test.machine"), TEXT("world-state.test.first-ready")));
    TestTrue(TEXT("Trusted world state accepted"), Progress.CommitWorldState(
        TEXT("world"), TEXT("world.test.machine"), TEXT("world-state.test.first-ready")));
    TestTrue(TEXT("Chapter is ready only after both gates"), Progress.IsCurrentChapterReadyToAdvance());
    TestTrue(TEXT("Ready chapter advances"), Progress.AdvanceChapter());
    TestEqual(TEXT("Second chapter active"), Progress.GetCurrentChapter()->ChapterId, FName(TEXT("chapter.test.second")));

    TestFalse(TEXT("Future evidence from prior chapter is rejected"), Progress.CommitEvidence(
        TEXT("math.test.objective"), TEXT("mathematics"), TEXT("world"), TEXT("world.test.pattern"), TEXT("sequence-and-infer"), TEXT("test.pattern.inferred")));
    TestTrue(TEXT("Second world state may arrive before evidence"), Progress.CommitWorldState(
        TEXT("science"), TEXT("science.test.machine"), TEXT("world-state.test.second-ready")));
    TestTrue(TEXT("Second evidence accepted"), Progress.CommitEvidence(
        TEXT("physics.test.objective"), TEXT("physics"), TEXT("science"), TEXT("science.test.force"), TEXT("predict-test-revise"), TEXT("test.force.predicted")));
    TestTrue(TEXT("Final chapter advances to completion"), Progress.AdvanceChapter());
    TestTrue(TEXT("Epic reports complete"), Progress.IsCompleted());
    TestTrue(TEXT("Progress reaches one"), FMath::IsNearlyEqual(Progress.GetProgressFraction(), 1.0f));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMEpicAttributionFailClosedTest,
    "WorldMakers.Epic.Runtime.AttributionFailClosed",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMEpicAttributionFailClosedTest::RunTest(const FString& Parameters)
{
    FWMEpicProgressModel Progress;
    TestTrue(TEXT("Epic begins"), Progress.Begin(MakeTwoChapterEpic()));
    TestFalse(TEXT("Wrong objective cannot launder valid event"), Progress.CommitEvidence(
        TEXT("physics.fake.objective"), TEXT("mathematics"), TEXT("world"), TEXT("world.test.pattern"), TEXT("sequence-and-infer"), TEXT("test.pattern.inferred")));
    TestFalse(TEXT("Wrong discipline cannot launder valid objective"), Progress.CommitEvidence(
        TEXT("math.test.objective"), TEXT("physics"), TEXT("world"), TEXT("world.test.pattern"), TEXT("sequence-and-infer"), TEXT("test.pattern.inferred")));
    TestFalse(TEXT("Wrong producer reference cannot receive credit"), Progress.CommitEvidence(
        TEXT("math.test.objective"), TEXT("mathematics"), TEXT("world"), TEXT("world.test.other"), TEXT("sequence-and-infer"), TEXT("test.pattern.inferred")));
    TestFalse(TEXT("Client cannot call advance as a completion flag"), Progress.AdvanceChapter());
    return true;
}

#endif
