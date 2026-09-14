#if WITH_DEV_AUTOMATION_TESTS

#include "Adventure/WMEpicRuntime.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
    bool LoadEpicCatalog(FWMEpicCatalog& OutCatalog)
    {
        const FString Path = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("WorldMakers/Epics/cross-disciplinary-epics-v1.json"));
        FString Json;
        FString Error;
        return FFileHelper::LoadFileToString(Json, *Path) && FWMEpicCatalog::TryParseJson(Json, OutCatalog, Error);
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMEpicChapterCheckpointDropsPartialEvidenceTest,
    "WorldMakers.Epic.Persistence.ChapterCheckpointDropsPartialEvidence",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMEpicChapterCheckpointDropsPartialEvidenceTest::RunTest(const FString& Parameters)
{
    FWMEpicCatalog Catalog;
    TestTrue(TEXT("Epic catalog loads"), LoadEpicCatalog(Catalog));
    const FWMEpicDefinition* Epic = Catalog.FindEpic(TEXT("epic.eclipse-engine"));
    TestNotNull(TEXT("Eclipse epic exists"), Epic);
    if (!Epic || Epic->Chapters.Num() < 2) return false;

    FWMEpicProgressModel Progress;
    TestTrue(TEXT("Epic starts"), Progress.Begin(*Epic));
    const FWMEpicEvidenceRequirement& Requirement = Epic->Chapters[0].EvidenceRequirements[0];
    TestTrue(TEXT("Partial evidence can be committed"), Progress.CommitEvidence(
        Requirement.ObjectiveId,
        Requirement.DisciplineId,
        Requirement.ProducerKind,
        Requirement.ProducerRefId,
        Requirement.PrimitiveId,
        Requirement.EvidenceEventId));
    TestTrue(TEXT("Partial evidence exists before restart"), Progress.BuildReadModel().CurrentEvidenceUnits > 0);

    TestTrue(TEXT("Checkpoint resumes chapter one"), Progress.ResumeAtChapter(*Epic, 1));
    const FWMEpicProgressReadModel Resumed = Progress.BuildReadModel();
    TestEqual(TEXT("Correct chapter index restored"), Resumed.CurrentChapterIndex, 1);
    TestEqual(TEXT("Correct chapter id restored"), Resumed.CurrentChapterId, Epic->Chapters[1].ChapterId);
    TestEqual(TEXT("Partial evidence is not persisted"), Resumed.CurrentEvidenceUnits, 0);
    TestEqual(TEXT("Partial world state is not persisted"), Resumed.CurrentWorldStateCount, 0);
    TestFalse(TEXT("Freshly resumed chapter is not ready"), Resumed.bCurrentChapterReady);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMEpicResumeIndexFailClosedTest,
    "WorldMakers.Epic.Persistence.ResumeIndexFailClosed",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMEpicResumeIndexFailClosedTest::RunTest(const FString& Parameters)
{
    FWMEpicCatalog Catalog;
    TestTrue(TEXT("Epic catalog loads"), LoadEpicCatalog(Catalog));
    const FWMEpicDefinition* Garden = Catalog.FindEpic(TEXT("epic.garden-end-winter"));
    TestNotNull(TEXT("Garden epic exists"), Garden);
    if (!Garden) return false;

    FWMEpicProgressModel Progress;
    TestFalse(TEXT("Negative checkpoint is rejected"), Progress.ResumeAtChapter(*Garden, -1));
    TestFalse(TEXT("Out-of-range checkpoint is rejected"), Progress.ResumeAtChapter(*Garden, Garden->Chapters.Num()));
    TestFalse(TEXT("Rejected checkpoint leaves runtime inactive"), Progress.IsActive());
    TestTrue(TEXT("Valid final chapter checkpoint resumes"), Progress.ResumeAtChapter(*Garden, Garden->Chapters.Num() - 1));
    TestEqual(TEXT("Final chapter id matches catalog"), Progress.BuildReadModel().CurrentChapterId, Garden->Chapters.Last().ChapterId);
    return true;
}

#endif
