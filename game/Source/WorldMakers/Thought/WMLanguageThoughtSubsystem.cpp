#include "Thought/WMLanguageThoughtSubsystem.h"

#include "Mission/WMMissionRuntimeSubsystem.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

void UWMLanguageThoughtSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    ReloadLanguageThoughtCatalog();
}

bool UWMLanguageThoughtSubsystem::ReloadLanguageThoughtCatalog()
{
    bCatalogLoaded = false;
    Catalog = FWMLanguageThoughtCatalog();

    const FString CatalogPath = FPaths::Combine(
        FPaths::ProjectContentDir(),
        TEXT("WorldMakers/Thought/language-literature-thought-v1.json"));

    FString Json;
    if (!FFileHelper::LoadFileToString(Json, *CatalogPath)) return false;

    FWMLanguageThoughtCatalog Candidate;
    FString Error;
    if (!FWMLanguageThoughtCatalog::TryParseJson(Json, Candidate, Error)) return false;

    Catalog = MoveTemp(Candidate);
    bCatalogLoaded = true;
    return true;
}

bool UWMLanguageThoughtSubsystem::SubmitEvidenceToActiveMission(const FWMThoughtEvidenceResult& Result)
{
    if (!Result.bAccepted || Result.PrimitiveId.IsNone() || Result.EvidenceEventId.IsNone() || !GetWorld()) return false;
    UWMMissionRuntimeSubsystem* MissionSubsystem = GetWorld()->GetSubsystem<UWMMissionRuntimeSubsystem>();
    return MissionSubsystem && MissionSubsystem->RecordComposableEvidence(Result.PrimitiveId, Result.EvidenceEventId, Result.NumericValue);
}

bool UWMLanguageThoughtSubsystem::EvaluateCommunicationAndRecord(const FName ChallengeId, const FName ChoiceId)
{
    if (!bCatalogLoaded) return false;
    const FWMCommunicationChallengeDefinition* Challenge = Catalog.FindCommunicationChallenge(ChallengeId);
    if (!Challenge) return false;
    FWMThoughtEvidenceResult Result;
    return FWMLanguageThoughtRuntime::EvaluateCommunication(*Challenge, ChoiceId, Result) && SubmitEvidenceToActiveMission(Result);
}

bool UWMLanguageThoughtSubsystem::TraverseNarrativeAndRecord(
    const FName StoryId,
    const FName CurrentNodeId,
    const FName ChoiceId,
    FName& OutNextNodeId)
{
    OutNextNodeId = NAME_None;
    if (!bCatalogLoaded) return false;
    const FWMNarrativeStoryDefinition* Story = Catalog.FindNarrativeStory(StoryId);
    if (!Story) return false;
    FWMThoughtEvidenceResult Result;
    if (!FWMLanguageThoughtRuntime::TraverseNarrative(*Story, CurrentNodeId, ChoiceId, Result)) return false;
    if (!SubmitEvidenceToActiveMission(Result)) return false;
    OutNextNodeId = Result.NextNodeId;
    return true;
}

bool UWMLanguageThoughtSubsystem::EvaluateEthicalReasoningAndRecord(
    const FName DilemmaId,
    const FName OptionId,
    const TArray<FName>& ReasonIds,
    const TArray<FName>& PerspectiveIds,
    const bool bAcknowledgedTradeoff)
{
    if (!bCatalogLoaded) return false;
    const FWMEthicalDilemmaDefinition* Dilemma = Catalog.FindEthicalDilemma(DilemmaId);
    if (!Dilemma) return false;
    FWMThoughtEvidenceResult Result;
    return FWMLanguageThoughtRuntime::EvaluateEthicalReasoning(
        *Dilemma, OptionId, ReasonIds, PerspectiveIds, bAcknowledgedTradeoff, Result) &&
        SubmitEvidenceToActiveMission(Result);
}

bool UWMLanguageThoughtSubsystem::EvaluatePhilosophicalArgumentAndRecord(
    const FName ProblemId,
    const FName ClaimId,
    const TArray<FName>& ReasonLinkIds,
    const TArray<FName>& AssumptionIds,
    const TArray<FName>& CounterexampleIds,
    const int32 RevisionCount)
{
    if (!bCatalogLoaded) return false;
    const FWMPhilosophyProblemDefinition* Problem = Catalog.FindPhilosophyProblem(ProblemId);
    if (!Problem) return false;
    FWMThoughtEvidenceResult Result;
    return FWMLanguageThoughtRuntime::EvaluatePhilosophicalArgument(
        *Problem, ClaimId, ReasonLinkIds, AssumptionIds, CounterexampleIds, RevisionCount, Result) &&
        SubmitEvidenceToActiveMission(Result);
}
