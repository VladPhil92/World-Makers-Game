#if WITH_DEV_AUTOMATION_TESTS

#include "Adventure/WMEclipseEngineExperience.h"
#include "Adventure/WMEclipseOpticsRuntime.h"
#include "Adventure/WMEpicRuntime.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
    bool LoadExperience(FWMEclipseExperienceCatalog& OutCatalog, FString& OutError)
    {
        const FString Path = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("WorldMakers/Epics/eclipse-engine-player-experience-v1.json"));
        FString Json;
        return FFileHelper::LoadFileToString(Json, *Path) && FWMEclipseExperienceCatalog::TryParseJson(Json, OutCatalog, OutError);
    }

    bool LoadEpicCatalog(FWMEpicCatalog& OutCatalog, FString& OutError)
    {
        const FString Path = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("WorldMakers/Epics/cross-disciplinary-epics-v1.json"));
        FString Json;
        return FFileHelper::LoadFileToString(Json, *Path) && FWMEpicCatalog::TryParseJson(Json, OutCatalog, OutError);
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMEclipseWorldFirstContractTest,
    "WorldMakers.Eclipse.PlayerExperience.WorldFirstContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMEclipseWorldFirstContractTest::RunTest(const FString& Parameters)
{
    FWMEclipseExperienceCatalog Experience;
    FString Error;
    TestTrue(TEXT("Eclipse experience loads"), LoadExperience(Experience, Error));
    TestEqual(TEXT("Eclipse has six game acts"), Experience.Chapters.Num(), 6);
    TestEqual(TEXT("Presentation is world-first"), Experience.PresentationRule, FName(TEXT("world-first-no-school-ui")));
    TestEqual(TEXT("Reward is world transformation"), Experience.RewardModel, FName(TEXT("world-transformation-and-new-capability")));
    TestEqual(TEXT("Failure is reversible experimentation"), Experience.FailureModel, FName(TEXT("reversible-experimentation-with-visible-consequence")));

    for (const FWMEclipseChapterExperience& Chapter : Experience.Chapters)
    {
        TestTrue(*FString::Printf(TEXT("%s has multiple world actions"), *Chapter.ChapterId.ToString()), Chapter.Actions.Num() >= 2);
        TestTrue(*FString::Printf(TEXT("%s ends in a causal world-state action"), *Chapter.ChapterId.ToString()), Chapter.Actions.Last().bHasWorldStateRoute);
        TestFalse(*FString::Printf(TEXT("%s fantasy goal is not a curriculum label"), *Chapter.ChapterId.ToString()), Chapter.FantasyGoalKey.ToString().Contains(TEXT("learning")));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMEclipseEpicExperienceParityTest,
    "WorldMakers.Eclipse.PlayerExperience.HiddenEvidenceParity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMEclipseEpicExperienceParityTest::RunTest(const FString& Parameters)
{
    FWMEclipseExperienceCatalog Experience;
    FWMEpicCatalog EpicCatalog;
    FString Error;
    TestTrue(TEXT("Experience loads"), LoadExperience(Experience, Error));
    TestTrue(TEXT("Epic catalog loads"), LoadEpicCatalog(EpicCatalog, Error));

    const FWMEpicDefinition* Epic = EpicCatalog.FindEpic(TEXT("epic.eclipse-engine"));
    TestNotNull(TEXT("Eclipse epic exists"), Epic);
    if (!Epic) return false;
    TestEqual(TEXT("Experience and epic have identical chapter count"), Experience.Chapters.Num(), Epic->Chapters.Num());

    for (const FWMEclipseChapterExperience& ExperienceChapter : Experience.Chapters)
    {
        const FWMEpicChapterDefinition* EpicChapter = Epic->FindChapter(ExperienceChapter.ChapterId);
        TestNotNull(*FString::Printf(TEXT("%s exists in hidden epic ledger"), *ExperienceChapter.ChapterId.ToString()), EpicChapter);
        if (!EpicChapter) continue;

        int32 EvidenceRoutes = 0;
        int32 WorldStateRoutes = 0;
        for (const FWMEclipseActionDefinition& Action : ExperienceChapter.Actions)
        {
            if (Action.bHasEvidenceRoute)
            {
                ++EvidenceRoutes;
                const bool bFound = EpicChapter->EvidenceRequirements.ContainsByPredicate([&Action](const FWMEpicEvidenceRequirement& Requirement)
                {
                    return Requirement.ObjectiveId == Action.Evidence.ObjectiveId &&
                        Requirement.DisciplineId == Action.Evidence.DisciplineId &&
                        Requirement.ProducerKind == Action.Evidence.ProducerKind &&
                        Requirement.ProducerRefId == Action.Evidence.ProducerRefId &&
                        Requirement.PrimitiveId == Action.Evidence.PrimitiveId &&
                        Requirement.EvidenceEventId == Action.Evidence.EvidenceEventId;
                });
                TestTrue(*FString::Printf(TEXT("%s maps to exact hidden evidence"), *Action.ActionId.ToString()), bFound);
            }
            if (Action.bHasWorldStateRoute)
            {
                ++WorldStateRoutes;
                const bool bFound = EpicChapter->WorldStateRequirements.ContainsByPredicate([&Action](const FWMEpicWorldStateRequirement& Requirement)
                {
                    return Requirement.ProducerKind == Action.WorldState.ProducerKind &&
                        Requirement.ProducerRefId == Action.WorldState.ProducerRefId &&
                        Requirement.WorldStateId == Action.WorldState.WorldStateId;
                });
                TestTrue(*FString::Printf(TEXT("%s maps to exact causal world state"), *Action.ActionId.ToString()), bFound);
            }
        }
        TestEqual(TEXT("Every hidden evidence requirement has one diegetic action"), EvidenceRoutes, EpicChapter->EvidenceRequirements.Num());
        TestEqual(TEXT("Every chapter has exactly one visible stabilization outcome"), WorldStateRoutes, 1);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMEclipseProgressiveHintTest,
    "WorldMakers.Eclipse.PlayerExperience.ProgressivePlayerRequestedHints",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMEclipseProgressiveHintTest::RunTest(const FString& Parameters)
{
    FWMEclipseExperienceCatalog Experience;
    FString Error;
    TestTrue(TEXT("Experience loads"), LoadExperience(Experience, Error));
    const FWMEclipseChapterExperience* Chapter = Experience.FindChapter(TEXT("chapter.eclipse.align-mirror-lattice"));
    TestNotNull(TEXT("Mirror chapter exists"), Chapter);
    if (!Chapter) return false;
    const FWMEclipseActionDefinition* Action = Chapter->FindAction(TEXT("eclipse.restore-mirror-symmetry"));
    TestNotNull(TEXT("Mirror action exists"), Action);
    if (!Action) return false;

    FWMEclipseHintRuntime Hints;
    TestEqual(TEXT("First requested hint is subtle"), Hints.RequestHint(*Action), Action->HintKeys[0]);
    Hints.RecordAttempt(Action->ActionId);
    TestEqual(TEXT("Later hint escalates after an attempt"), Hints.RequestHint(*Action), Action->HintKeys[1]);
    Hints.RecordAttempt(Action->ActionId);
    TestEqual(TEXT("Final hint becomes more explicit without penalty"), Hints.RequestHint(*Action), Action->HintKeys[2]);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMEclipseOpticsPuzzleTest,
    "WorldMakers.Eclipse.Gameplay.OpticsRequiresSpatialReasoning",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMEclipseOpticsPuzzleTest::RunTest(const FString& Parameters)
{
    TestFalse(TEXT("Same-sign mirrors are not symmetric"), FWMEclipseOpticsRuntime::IsSymmetrySolved(32.0f, 31.0f));
    TestTrue(TEXT("Mirrored angles solve symmetry"), FWMEclipseOpticsRuntime::IsSymmetrySolved(32.0f, -31.0f));

    TestFalse(TEXT("Unequal incidence/reflection fails"), FWMEclipseOpticsRuntime::IsReflectionSolved(35.0f, 48.0f, 1.0f));
    TestFalse(TEXT("Correct reflection that misses the receiver fails"), FWMEclipseOpticsRuntime::IsReflectionSolved(35.0f, 35.5f, 9.0f));
    TestTrue(TEXT("Reflection law plus receiver alignment solves"), FWMEclipseOpticsRuntime::IsReflectionSolved(35.0f, 35.5f, 2.0f));

    TestFalse(TEXT("Single lucky sample cannot prove stability"), FWMEclipseOpticsRuntime::IsPathStable({1.0f}));
    TestFalse(TEXT("One unstable perturbation rejects the model"), FWMEclipseOpticsRuntime::IsPathStable({-1.2f, 0.6f, 6.2f}));
    TestTrue(TEXT("Several bounded perturbations support stable spatial model"), FWMEclipseOpticsRuntime::IsPathStable({-1.2f, 0.6f, 1.5f, -0.4f}));
    return true;
}

#endif
