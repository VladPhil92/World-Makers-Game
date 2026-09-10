#if WITH_DEV_AUTOMATION_TESTS

#include "Adventure/WMAdventureRuntime.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
    FWMAdventureBeatDefinition MakeBeat(
        const TCHAR* BeatId,
        const TCHAR* ProducerKind,
        const TCHAR* ProducerRefId,
        const TCHAR* PrimitiveId,
        const TCHAR* EvidenceEventId,
        const int32 RequiredCount = 1)
    {
        FWMAdventureBeatDefinition Beat;
        Beat.BeatId = BeatId;
        Beat.ProducerKind = ProducerKind;
        Beat.ProducerRefId = ProducerRefId;
        Beat.PrimitiveId = PrimitiveId;
        Beat.EvidenceEventId = EvidenceEventId;
        Beat.PromptKey = FName(*(FString(BeatId) + TEXT(".prompt")));
        Beat.FormalizationKey = FName(*(FString(BeatId) + TEXT(".formalization")));
        Beat.RequiredCount = RequiredCount;
        return Beat;
    }

    FWMFantasticAdventureDefinition MakeTwoBeatAdventure()
    {
        FWMFantasticAdventureDefinition Adventure;
        Adventure.AdventureId = TEXT("adventure.test.ordered");
        Adventure.MissionId = TEXT("mission.test.ordered");
        Adventure.TitleKey = TEXT("adventure.test.ordered.title");
        Adventure.PremiseKey = TEXT("adventure.test.ordered.premise");
        Adventure.PrimaryDiscipline = TEXT("mathematics");
        Adventure.SecondaryDisciplines = { TEXT("geometry") };
        Adventure.AgeBand = TEXT("9-10");
        Adventure.Beats = {
            MakeBeat(TEXT("beat.test.observe"), TEXT("world"), TEXT("world.test.pattern"), TEXT("sequence-and-infer"), TEXT("test.pattern.inferred"), 2),
            MakeBeat(TEXT("beat.test.build"), TEXT("building"), TEXT("building.test.route"), TEXT("construct-to-constraint"), TEXT("test.route.built"), 1)
        };
        return Adventure;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMFantasticAdventurePackCatalogTest,
    "WorldMakers.Adventure.Pack.CatalogHasElevenStreams",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMFantasticAdventurePackCatalogTest::RunTest(const FString& Parameters)
{
    const FString Path = FPaths::Combine(
        FPaths::ProjectContentDir(), TEXT("WorldMakers/Adventures/first-fantastic-adventure-pack-v1.json"));
    FString Json;
    TestTrue(TEXT("Pack file loads"), FFileHelper::LoadFileToString(Json, *Path));

    FWMFantasticAdventurePack Pack;
    FString Error;
    TestTrue(TEXT("Pack parses and validates"), FWMFantasticAdventurePack::TryParseJson(Json, Pack, Error));
    TestEqual(TEXT("First pack has eleven adventures"), Pack.Adventures.Num(), 11);

    TSet<FName> Disciplines;
    for (const FWMFantasticAdventureDefinition& Adventure : Pack.Adventures)
    {
        Disciplines.Add(Adventure.PrimaryDiscipline);
    }
    TestEqual(TEXT("Every first-class stream has a primary adventure"), Disciplines.Num(), 11);
    TestTrue(TEXT("Chemistry is represented"), Disciplines.Contains(TEXT("chemistry")));
    TestTrue(TEXT("Philosophy is represented"), Disciplines.Contains(TEXT("philosophy-for-children")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMFantasticAdventureOrderedEvidenceTest,
    "WorldMakers.Adventure.Pack.OrderedEvidenceGate",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMFantasticAdventureOrderedEvidenceTest::RunTest(const FString& Parameters)
{
    FWMAdventureProgressModel Progress;
    const FWMFantasticAdventureDefinition Adventure = MakeTwoBeatAdventure();
    TestTrue(TEXT("Adventure begins"), Progress.Begin(Adventure));
    TestEqual(TEXT("First beat is active"), Progress.GetCurrentBeat()->BeatId, FName(TEXT("beat.test.observe")));

    TestFalse(TEXT("Future beat cannot be completed early"), Progress.CommitEvidence(
        TEXT("building"), TEXT("building.test.route"), TEXT("construct-to-constraint"), TEXT("test.route.built")));
    TestFalse(TEXT("Correct event from wrong producer is rejected"), Progress.CommitEvidence(
        TEXT("thought"), TEXT("world.test.pattern"), TEXT("sequence-and-infer"), TEXT("test.pattern.inferred")));

    TestTrue(TEXT("First evidence unit accepted"), Progress.CommitEvidence(
        TEXT("world"), TEXT("world.test.pattern"), TEXT("sequence-and-infer"), TEXT("test.pattern.inferred")));
    TestTrue(TEXT("Progress is one of three evidence units"), FMath::IsNearlyEqual(Progress.GetProgressFraction(), 1.0f / 3.0f));
    TestEqual(TEXT("Beat remains until requiredCount reached"), Progress.GetCurrentBeat()->BeatId, FName(TEXT("beat.test.observe")));

    TestTrue(TEXT("Second required unit advances beat"), Progress.CommitEvidence(
        TEXT("world"), TEXT("world.test.pattern"), TEXT("sequence-and-infer"), TEXT("test.pattern.inferred")));
    TestEqual(TEXT("Second beat is now active"), Progress.GetCurrentBeat()->BeatId, FName(TEXT("beat.test.build")));

    TestTrue(TEXT("Final beat completes adventure"), Progress.CommitEvidence(
        TEXT("building"), TEXT("building.test.route"), TEXT("construct-to-constraint"), TEXT("test.route.built")));
    TestTrue(TEXT("Adventure reports complete"), Progress.IsCompleted());
    TestTrue(TEXT("Progress reaches one"), FMath::IsNearlyEqual(Progress.GetProgressFraction(), 1.0f));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMFantasticAdventureProducerPrivacyTest,
    "WorldMakers.Adventure.Pack.StableProducerEvidenceOnly",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMFantasticAdventureProducerPrivacyTest::RunTest(const FString& Parameters)
{
    FWMFantasticAdventureDefinition Adventure;
    Adventure.AdventureId = TEXT("adventure.test.thought");
    Adventure.MissionId = TEXT("mission.test.thought");
    Adventure.TitleKey = TEXT("adventure.test.thought.title");
    Adventure.PremiseKey = TEXT("adventure.test.thought.premise");
    Adventure.PrimaryDiscipline = TEXT("ethics");
    Adventure.SecondaryDisciplines = { TEXT("philosophy-for-children") };
    Adventure.AgeBand = TEXT("9-10");
    Adventure.Beats = {
        MakeBeat(TEXT("beat.test.reason"), TEXT("thought"), TEXT("ethics.test"), TEXT("reason-through-dilemma"), TEXT("ethics.test.reasoned"))
    };

    FWMAdventureProgressModel Progress;
    TestTrue(TEXT("Thought adventure begins"), Progress.Begin(Adventure));
    TestFalse(TEXT("Unstructured free-text producer kind is not accepted"), Progress.CommitEvidence(
        TEXT("free-text"), TEXT("ethics.test"), TEXT("reason-through-dilemma"), TEXT("ethics.test.reasoned")));
    TestTrue(TEXT("Stable thought producer is accepted"), Progress.CommitEvidence(
        TEXT("thought"), TEXT("ethics.test"), TEXT("reason-through-dilemma"), TEXT("ethics.test.reasoned")));
    return true;
}

#endif
