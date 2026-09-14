#if WITH_DEV_AUTOMATION_TESTS

#include "Adventure/WMEpicJourneySaveGame.h"
#include "Adventure/WMEpicRuntime.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMEpicNativeSaveGameRoundTripTest,
    "WorldMakers.Epic.Persistence.NativeSaveGameRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMEpicNativeSaveGameRoundTripTest::RunTest(const FString& Parameters)
{
    const FString SlotName = FString::Printf(
        TEXT("WM_EpicPersistenceAutomation_%s"),
        *FGuid::NewGuid().ToString(EGuidFormats::Digits));
    constexpr int32 UserIndex = 0;

    // A unique slot prevents this automation test from touching a real player's journey save.
    UGameplayStatics::DeleteGameInSlot(SlotName, UserIndex);

    UWMEpicJourneySaveGame* Save = Cast<UWMEpicJourneySaveGame>(
        UGameplayStatics::CreateSaveGameObject(UWMEpicJourneySaveGame::StaticClass()));
    TestNotNull(TEXT("Epic SaveGame object can be created"), Save);
    if (!Save) return false;

    FWMEpicCheckpoint Checkpoint;
    Checkpoint.EpicId = TEXT("epic.eclipse-engine");
    Checkpoint.ChapterId = TEXT("chapter.eclipse.power-core");
    Checkpoint.ChapterIndex = 2;
    Checkpoint.ChapterCount = 6;
    Checkpoint.bCompleted = false;
    Save->FormatVersion = UWMEpicJourneySaveGame::CurrentFormatVersion;
    Save->Checkpoints.Add(Checkpoint);
    Save->PendingSyncCheckpoints.Add(Checkpoint);

    TestTrue(TEXT("Native SaveGame write succeeds"), UGameplayStatics::SaveGameToSlot(Save, SlotName, UserIndex));
    TestTrue(TEXT("Native SaveGame slot exists after write"), UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex));

    UWMEpicJourneySaveGame* Loaded = Cast<UWMEpicJourneySaveGame>(
        UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex));
    TestNotNull(TEXT("Native SaveGame can be loaded"), Loaded);
    if (!Loaded)
    {
        UGameplayStatics::DeleteGameInSlot(SlotName, UserIndex);
        return false;
    }

    TestEqual(TEXT("Save format round-trips"), Loaded->FormatVersion, UWMEpicJourneySaveGame::CurrentFormatVersion);
    TestEqual(TEXT("Exactly one checkpoint round-trips"), Loaded->Checkpoints.Num(), 1);
    TestEqual(TEXT("Exactly one pending sync checkpoint round-trips"), Loaded->PendingSyncCheckpoints.Num(), 1);
    if (Loaded->Checkpoints.Num() == 1)
    {
        const FWMEpicCheckpoint& Restored = Loaded->Checkpoints[0];
        TestEqual(TEXT("Epic id round-trips"), Restored.EpicId, Checkpoint.EpicId);
        TestEqual(TEXT("Chapter id round-trips"), Restored.ChapterId, Checkpoint.ChapterId);
        TestEqual(TEXT("Chapter index round-trips"), Restored.ChapterIndex, Checkpoint.ChapterIndex);
        TestEqual(TEXT("Chapter count round-trips"), Restored.ChapterCount, Checkpoint.ChapterCount);
        TestEqual(TEXT("Completion state round-trips"), Restored.bCompleted, Checkpoint.bCompleted);
    }
    if (Loaded->PendingSyncCheckpoints.Num() == 1)
    {
        const FWMEpicCheckpoint& Pending = Loaded->PendingSyncCheckpoints[0];
        TestEqual(TEXT("Outbox epic id round-trips"), Pending.EpicId, Checkpoint.EpicId);
        TestEqual(TEXT("Outbox chapter index round-trips"), Pending.ChapterIndex, Checkpoint.ChapterIndex);
    }

    TestTrue(TEXT("Automation SaveGame slot is deleted"), UGameplayStatics::DeleteGameInSlot(SlotName, UserIndex));
    TestFalse(TEXT("Automation SaveGame leaves no persistent slot"), UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex));
    return true;
}

#endif