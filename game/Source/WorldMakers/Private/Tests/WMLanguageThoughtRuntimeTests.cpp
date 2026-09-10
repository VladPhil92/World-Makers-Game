#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Mission/WMMissionTypes.h"
#include "Thought/WMLanguageThoughtRuntime.h"

namespace
{
    FWMCommunicationChallengeDefinition BuildEnglishChallenge()
    {
        FWMCommunicationChallengeDefinition Challenge;
        Challenge.ChallengeId = TEXT("language.en.labyrinth-ask-route");
        Challenge.TargetLanguageId = TEXT("en");
        Challenge.ContextId = TEXT("context.labyrinth.ask-route");
        Challenge.RequiredMeaningId = TEXT("meaning.ask-route-to-center");
        Challenge.AcceptedRegisterIds = { TEXT("polite"), TEXT("neutral") };
        Challenge.Options = {
            { TEXT("choice.en.correct"), TEXT("en"), TEXT("meaning.ask-route-to-center"), TEXT("polite"), TEXT("syntax.en.wh-question"), TEXT("language.english.contextual-communication-demonstrated") },
            { TEXT("choice.es.same-meaning"), TEXT("es"), TEXT("meaning.ask-route-to-center"), TEXT("polite"), TEXT("syntax.es.interrogative"), TEXT("language.english.contextual-communication-demonstrated") },
            { TEXT("choice.en.wrong-intent"), TEXT("en"), TEXT("meaning.demand-gate-open"), TEXT("abrupt"), TEXT("syntax.en.imperative"), TEXT("language.english.contextual-communication-demonstrated") }
        };
        return Challenge;
    }

    FWMNarrativeStoryDefinition BuildStory()
    {
        FWMNarrativeStoryDefinition Story;
        Story.StoryId = TEXT("story.labyrinth-minotaur-prototype");
        Story.TraditionId = TEXT("tradition.ancient-greek");
        Story.SourceClassId = TEXT("retelling");
        Story.ProvenanceKey = TEXT("provenance.greek.minotaur.general-tradition");
        Story.CulturalReviewState = TEXT("draft");
        Story.StartNodeId = TEXT("node.gate");
        Story.Nodes = {
            { TEXT("node.gate"), TEXT("story.gate.passage"), TEXT("pov.observer") },
            { TEXT("node.thread"), TEXT("story.thread.passage"), TEXT("pov.ariadne") }
        };
        Story.Choices = {
            { TEXT("choice.follow-thread"), TEXT("node.gate"), TEXT("node.thread"), TEXT("inference.symbol-as-clue"), TEXT("literature.inference.context-clue-used") }
        };
        return Story;
    }

    FWMEthicalDilemmaDefinition BuildDilemma()
    {
        FWMEthicalDilemmaDefinition Dilemma;
        Dilemma.DilemmaId = TEXT("ethics.bridge-two-villages-prototype");
        Dilemma.PerspectiveIds = { TEXT("perspective.river"), TEXT("perspective.hill"), TEXT("perspective.builders") };
        Dilemma.RequiredPerspectiveCount = 2;

        FWMEthicalOptionDefinition NeedFirst;
        NeedFirst.OptionId = TEXT("option.need-first");
        NeedFirst.SupportedReasonIds = { TEXT("reason.immediate-need") };
        NeedFirst.AffectedPerspectiveIds = Dilemma.PerspectiveIds;
        NeedFirst.TradeoffTags = { TEXT("tradeoff.speed-vs-equality") };
        NeedFirst.ConsequenceProfile = { { TEXT("fairness"), -0.2f }, { TEXT("welfare"), 0.7f } };
        NeedFirst.EvidenceEventId = TEXT("ethics.multi-perspective-reasoning-demonstrated");

        FWMEthicalOptionDefinition EqualSplit;
        EqualSplit.OptionId = TEXT("option.equal-split");
        EqualSplit.SupportedReasonIds = { TEXT("reason.equal-share") };
        EqualSplit.AffectedPerspectiveIds = Dilemma.PerspectiveIds;
        EqualSplit.TradeoffTags = { TEXT("tradeoff.equality-vs-urgency") };
        EqualSplit.ConsequenceProfile = { { TEXT("fairness"), 0.8f }, { TEXT("welfare"), 0.2f } };
        EqualSplit.EvidenceEventId = TEXT("ethics.multi-perspective-reasoning-demonstrated");

        Dilemma.Options = { NeedFirst, EqualSplit };
        return Dilemma;
    }

    FWMPhilosophyProblemDefinition BuildShipProblem()
    {
        FWMPhilosophyProblemDefinition Problem;
        Problem.ProblemId = TEXT("philosophy.ship-theseus-prototype");
        Problem.ClaimIds = { TEXT("claim.same"), TEXT("claim.different") };
        Problem.AssumptionIds = { TEXT("assumption.persistence"), TEXT("assumption.material") };
        Problem.CounterexampleIds = { TEXT("counterexample.reassembled"), TEXT("counterexample.cells") };
        Problem.ReasonLinks = {
            { TEXT("link.support-same"), TEXT("reason.history"), TEXT("claim.same"), TEXT("supports") },
            { TEXT("link.support-different"), TEXT("reason.material"), TEXT("claim.different"), TEXT("supports") }
        };
        Problem.MinReasonLinks = 1;
        Problem.MinAssumptions = 1;
        Problem.MinCounterexamples = 1;
        Problem.MinRevisions = 1;
        Problem.EvidenceEventId = TEXT("philosophy.argument-revised-after-counterexample");
        return Problem;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMLanguageContextTest,
    "WorldMakers.Thought.Language.ContextualBilingualCommunication",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMLanguageContextTest::RunTest(const FString& Parameters)
{
    const FWMCommunicationChallengeDefinition Challenge = BuildEnglishChallenge();
    TestTrue(TEXT("Challenge is sane"), Challenge.IsSane());

    FWMThoughtEvidenceResult Result;
    TestTrue(TEXT("Correct English intent/register succeeds"), FWMLanguageThoughtRuntime::EvaluateCommunication(Challenge, TEXT("choice.en.correct"), Result));
    TestEqual(TEXT("Communication primitive emitted"), Result.PrimitiveId, FName(TEXT("communicate-in-language")));
    TestFalse(TEXT("Same meaning in wrong target language does not satisfy the challenge"), FWMLanguageThoughtRuntime::EvaluateCommunication(Challenge, TEXT("choice.es.same-meaning"), Result));
    TestFalse(TEXT("Correct language with wrong intent/register is rejected"), FWMLanguageThoughtRuntime::EvaluateCommunication(Challenge, TEXT("choice.en.wrong-intent"), Result));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMLiteratureNarrativeTest,
    "WorldMakers.Thought.Literature.ProvenancePointOfViewAndInference",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMLiteratureNarrativeTest::RunTest(const FString& Parameters)
{
    const FWMNarrativeStoryDefinition Story = BuildStory();
    TestTrue(TEXT("Story graph with provenance is sane"), Story.IsSane());
    TestEqual(TEXT("Retelling classification preserved"), Story.SourceClassId, FName(TEXT("retelling")));
    TestEqual(TEXT("Ariadne point of view is explicit"), Story.FindNode(TEXT("node.thread"))->PointOfViewId, FName(TEXT("pov.ariadne")));

    FWMThoughtEvidenceResult Result;
    TestTrue(TEXT("Valid inference edge traverses"), FWMLanguageThoughtRuntime::TraverseNarrative(Story, TEXT("node.gate"), TEXT("choice.follow-thread"), Result));
    TestEqual(TEXT("Narrative moves to intended node"), Result.NextNodeId, FName(TEXT("node.thread")));
    TestEqual(TEXT("Literature primitive emitted"), Result.PrimitiveId, FName(TEXT("interpret-text-world")));
    TestFalse(TEXT("Choice cannot be replayed from the wrong node"), FWMLanguageThoughtRuntime::TraverseNarrative(Story, TEXT("node.thread"), TEXT("choice.follow-thread"), Result));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMEthicsReasoningTest,
    "WorldMakers.Thought.Ethics.ReasonsPerspectivesNotIdeology",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMEthicsReasoningTest::RunTest(const FString& Parameters)
{
    const FWMEthicalDilemmaDefinition Dilemma = BuildDilemma();
    TestTrue(TEXT("Dilemma is sane"), Dilemma.IsSane());
    FWMThoughtEvidenceResult Result;

    TestTrue(TEXT("Need-first option can pass with reasons and multiple perspectives"), FWMLanguageThoughtRuntime::EvaluateEthicalReasoning(
        Dilemma,
        TEXT("option.need-first"),
        { TEXT("reason.immediate-need") },
        { TEXT("perspective.river"), TEXT("perspective.hill") },
        true,
        Result));

    TestTrue(TEXT("Equal-split option can also pass; no hidden moral-answer key"), FWMLanguageThoughtRuntime::EvaluateEthicalReasoning(
        Dilemma,
        TEXT("option.equal-split"),
        { TEXT("reason.equal-share") },
        { TEXT("perspective.river"), TEXT("perspective.builders") },
        true,
        Result));

    TestFalse(TEXT("One-perspective justification is insufficient"), FWMLanguageThoughtRuntime::EvaluateEthicalReasoning(
        Dilemma,
        TEXT("option.need-first"),
        { TEXT("reason.immediate-need") },
        { TEXT("perspective.river") },
        true,
        Result));
    TestFalse(TEXT("Tradeoff must be acknowledged"), FWMLanguageThoughtRuntime::EvaluateEthicalReasoning(
        Dilemma,
        TEXT("option.need-first"),
        { TEXT("reason.immediate-need") },
        { TEXT("perspective.river"), TEXT("perspective.hill") },
        false,
        Result));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMPhilosophyArgumentTest,
    "WorldMakers.Thought.Philosophy.ArgumentCounterexampleAndRevision",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMPhilosophyArgumentTest::RunTest(const FString& Parameters)
{
    const FWMPhilosophyProblemDefinition Problem = BuildShipProblem();
    TestTrue(TEXT("Philosophy problem is sane"), Problem.IsSane());
    FWMThoughtEvidenceResult Result;

    TestTrue(TEXT("Same-ship position can demonstrate rigorous reasoning"), FWMLanguageThoughtRuntime::EvaluatePhilosophicalArgument(
        Problem, TEXT("claim.same"), { TEXT("link.support-same") }, { TEXT("assumption.persistence") }, { TEXT("counterexample.reassembled") }, 1, Result));
    TestTrue(TEXT("Different-ship position can also demonstrate rigorous reasoning"), FWMLanguageThoughtRuntime::EvaluatePhilosophicalArgument(
        Problem, TEXT("claim.different"), { TEXT("link.support-different") }, { TEXT("assumption.material") }, { TEXT("counterexample.cells") }, 1, Result));
    TestFalse(TEXT("Reasoning without revision does not satisfy the rubric"), FWMLanguageThoughtRuntime::EvaluatePhilosophicalArgument(
        Problem, TEXT("claim.same"), { TEXT("link.support-same") }, { TEXT("assumption.persistence") }, { TEXT("counterexample.reassembled") }, 0, Result));
    TestFalse(TEXT("A reason link for another claim cannot spoof this claim"), FWMLanguageThoughtRuntime::EvaluatePhilosophicalArgument(
        Problem, TEXT("claim.same"), { TEXT("link.support-different") }, { TEXT("assumption.persistence") }, { TEXT("counterexample.reassembled") }, 1, Result));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMThoughtComposableMissionTest,
    "WorldMakers.Thought.Integration.ComposableEvidenceAcrossDomains",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMThoughtComposableMissionTest::RunTest(const FString& Parameters)
{
    FWMMissionRuntimeDefinition Mission;
    Mission.MissionId = TEXT("mission.thought.cross-domain-proof");
    Mission.Evaluator = TEXT("composable");
    Mission.LearningObjectiveIds = { TEXT("language.context"), TEXT("literature.inference"), TEXT("ethics.reasoning"), TEXT("philosophy.argument") };
    Mission.RequiredEvidenceEventIds = {
        TEXT("language.english.contextual-communication-demonstrated"),
        TEXT("literature.inference.context-clue-used"),
        TEXT("ethics.multi-perspective-reasoning-demonstrated"),
        TEXT("philosophy.argument-revised-after-counterexample")
    };
    Mission.ComposableRequirements = {
        { TEXT("communicate-in-language"), TEXT("language.english.contextual-communication-demonstrated"), TEXT("language.context"), 1 },
        { TEXT("interpret-text-world"), TEXT("literature.inference.context-clue-used"), TEXT("literature.inference"), 1 },
        { TEXT("reason-through-dilemma"), TEXT("ethics.multi-perspective-reasoning-demonstrated"), TEXT("ethics.reasoning"), 1 },
        { TEXT("argue-and-revise"), TEXT("philosophy.argument-revised-after-counterexample"), TEXT("philosophy.argument"), 1 }
    };
    Mission.RewardIds = { TEXT("reward.thought.cross-domain-proof") };
    Mission.bPrototypeOnly = true;

    FWMMissionProgressModel Progress;
    TestTrue(TEXT("Composable thought mission begins"), Progress.Begin(Mission));

    FWMThoughtEvidenceResult Communication;
    TestTrue(TEXT("Language evidence generated"), FWMLanguageThoughtRuntime::EvaluateCommunication(BuildEnglishChallenge(), TEXT("choice.en.correct"), Communication));
    TestTrue(TEXT("Language evidence recorded"), Progress.RecordComposableEvidence(Communication.PrimitiveId, Communication.EvidenceEventId));

    FWMThoughtEvidenceResult Literature;
    TestTrue(TEXT("Literature evidence generated"), FWMLanguageThoughtRuntime::TraverseNarrative(BuildStory(), TEXT("node.gate"), TEXT("choice.follow-thread"), Literature));
    TestTrue(TEXT("Literature evidence recorded"), Progress.RecordComposableEvidence(Literature.PrimitiveId, Literature.EvidenceEventId));

    FWMThoughtEvidenceResult Ethics;
    TestTrue(TEXT("Ethics evidence generated"), FWMLanguageThoughtRuntime::EvaluateEthicalReasoning(
        BuildDilemma(), TEXT("option.need-first"), { TEXT("reason.immediate-need") }, { TEXT("perspective.river"), TEXT("perspective.hill") }, true, Ethics));
    TestTrue(TEXT("Ethics evidence recorded"), Progress.RecordComposableEvidence(Ethics.PrimitiveId, Ethics.EvidenceEventId));

    FWMThoughtEvidenceResult Philosophy;
    TestTrue(TEXT("Philosophy evidence generated"), FWMLanguageThoughtRuntime::EvaluatePhilosophicalArgument(
        BuildShipProblem(), TEXT("claim.same"), { TEXT("link.support-same") }, { TEXT("assumption.persistence") }, { TEXT("counterexample.reassembled") }, 1, Philosophy));
    TestTrue(TEXT("Philosophy evidence recorded"), Progress.RecordComposableEvidence(Philosophy.PrimitiveId, Philosophy.EvidenceEventId));

    TestEqual(TEXT("Cross-domain mission completes only after all evidence"), Progress.State, EWMMissionRuntimeState::Completed);
    TestTrue(TEXT("Progress reaches one"), FMath::IsNearlyEqual(Progress.GetProgressFraction(), 1.0f));
    return true;
}

#endif
