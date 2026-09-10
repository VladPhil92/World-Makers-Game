#pragma once

#include "CoreMinimal.h"

struct WORLDMAKERS_API FWMThoughtEvidenceResult
{
    bool bAccepted = false;
    FName PrimitiveId;
    FName EvidenceEventId;
    float NumericValue = 1.0f;
    FName NextNodeId;
};

struct WORLDMAKERS_API FWMCommunicationOptionDefinition
{
    FName ChoiceId;
    FName LanguageId;
    FName MeaningId;
    FName RegisterId;
    FName SyntaxPatternId;
    FName EvidenceEventId;

    bool IsSane() const;
};

struct WORLDMAKERS_API FWMCommunicationChallengeDefinition
{
    FName ChallengeId;
    FName TargetLanguageId;
    FName ContextId;
    FName RequiredMeaningId;
    TArray<FName> AcceptedRegisterIds;
    TArray<FWMCommunicationOptionDefinition> Options;

    bool IsSane() const;
    const FWMCommunicationOptionDefinition* FindOption(FName ChoiceId) const;
};

struct WORLDMAKERS_API FWMNarrativeNodeDefinition
{
    FName NodeId;
    FName PassageKey;
    FName PointOfViewId;

    bool IsSane() const;
};

struct WORLDMAKERS_API FWMNarrativeChoiceDefinition
{
    FName ChoiceId;
    FName FromNodeId;
    FName ToNodeId;
    FName InferenceTag;
    FName EvidenceEventId;

    bool IsSane() const;
};

struct WORLDMAKERS_API FWMNarrativeStoryDefinition
{
    FName StoryId;
    FName TraditionId;
    FName SourceClassId;
    FName ProvenanceKey;
    FName CulturalReviewState;
    FName StartNodeId;
    TArray<FWMNarrativeNodeDefinition> Nodes;
    TArray<FWMNarrativeChoiceDefinition> Choices;

    bool IsSane() const;
    const FWMNarrativeNodeDefinition* FindNode(FName NodeId) const;
    const FWMNarrativeChoiceDefinition* FindChoice(FName CurrentNodeId, FName ChoiceId) const;
};

struct WORLDMAKERS_API FWMEthicalOptionDefinition
{
    FName OptionId;
    TArray<FName> SupportedReasonIds;
    TArray<FName> AffectedPerspectiveIds;
    TArray<FName> TradeoffTags;
    TMap<FName, float> ConsequenceProfile;
    FName EvidenceEventId;

    bool IsSane() const;
};

struct WORLDMAKERS_API FWMEthicalDilemmaDefinition
{
    FName DilemmaId;
    TArray<FName> PerspectiveIds;
    TArray<FWMEthicalOptionDefinition> Options;
    int32 RequiredPerspectiveCount = 2;

    bool IsSane() const;
    const FWMEthicalOptionDefinition* FindOption(FName OptionId) const;
};

struct WORLDMAKERS_API FWMArgumentLinkDefinition
{
    FName LinkId;
    FName FromPropositionId;
    FName ToPropositionId;
    FName RelationId;

    bool IsSane() const;
};

struct WORLDMAKERS_API FWMPhilosophyProblemDefinition
{
    FName ProblemId;
    TArray<FName> ClaimIds;
    TArray<FName> AssumptionIds;
    TArray<FName> CounterexampleIds;
    TArray<FWMArgumentLinkDefinition> ReasonLinks;
    int32 MinReasonLinks = 1;
    int32 MinAssumptions = 1;
    int32 MinCounterexamples = 1;
    int32 MinRevisions = 1;
    FName EvidenceEventId;

    bool IsSane() const;
};

struct WORLDMAKERS_API FWMLanguageThoughtCatalog
{
    int32 SchemaVersion = 1;
    bool bPrototypeOnly = true;
    FName PrivacyModel;
    TMap<FName, FWMCommunicationChallengeDefinition> CommunicationChallenges;
    TMap<FName, FWMNarrativeStoryDefinition> NarrativeStories;
    TMap<FName, FWMEthicalDilemmaDefinition> EthicalDilemmas;
    TMap<FName, FWMPhilosophyProblemDefinition> PhilosophyProblems;

    bool IsSane() const;
    const FWMCommunicationChallengeDefinition* FindCommunicationChallenge(FName ChallengeId) const;
    const FWMNarrativeStoryDefinition* FindNarrativeStory(FName StoryId) const;
    const FWMEthicalDilemmaDefinition* FindEthicalDilemma(FName DilemmaId) const;
    const FWMPhilosophyProblemDefinition* FindPhilosophyProblem(FName ProblemId) const;
    static bool TryParseJson(const FString& Json, FWMLanguageThoughtCatalog& OutCatalog, FString& OutError);
};

/** Deterministic structured reasoning runtime. It stores stable IDs only, never child-authored free text. */
struct WORLDMAKERS_API FWMLanguageThoughtRuntime
{
    static bool EvaluateCommunication(
        const FWMCommunicationChallengeDefinition& Challenge,
        FName ChoiceId,
        FWMThoughtEvidenceResult& OutResult);

    static bool TraverseNarrative(
        const FWMNarrativeStoryDefinition& Story,
        FName CurrentNodeId,
        FName ChoiceId,
        FWMThoughtEvidenceResult& OutResult);

    /** Evaluates quality of ethical reasoning, not whether the selected moral option is ideologically correct. */
    static bool EvaluateEthicalReasoning(
        const FWMEthicalDilemmaDefinition& Dilemma,
        FName OptionId,
        const TArray<FName>& ReasonIds,
        const TArray<FName>& PerspectiveIds,
        bool bAcknowledgedTradeoff,
        FWMThoughtEvidenceResult& OutResult);

    /** Evaluates argument structure: reasons, assumptions, counterexamples and revision. */
    static bool EvaluatePhilosophicalArgument(
        const FWMPhilosophyProblemDefinition& Problem,
        FName ClaimId,
        const TArray<FName>& ReasonLinkIds,
        const TArray<FName>& AssumptionIds,
        const TArray<FName>& CounterexampleIds,
        int32 RevisionCount,
        FWMThoughtEvidenceResult& OutResult);
};
