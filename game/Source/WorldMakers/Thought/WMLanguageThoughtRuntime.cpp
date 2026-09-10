#include "Thought/WMLanguageThoughtRuntime.h"

#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
    const FName CommunicatePrimitive(TEXT("communicate-in-language"));
    const FName InterpretPrimitive(TEXT("interpret-text-world"));
    const FName EthicsPrimitive(TEXT("reason-through-dilemma"));
    const FName ArgumentPrimitive(TEXT("argue-and-revise"));
    const FName StableIdsPrivacyModel(TEXT("stable-ids-no-child-free-text"));

    bool ReadNameArray(const TSharedPtr<FJsonObject>& Object, const TCHAR* FieldName, TArray<FName>& OutValues, const bool bAllowEmpty = false)
    {
        const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
        if (!Object.IsValid() || !Object->TryGetArrayField(FieldName, Values) || !Values)
        {
            return false;
        }
        OutValues.Reset();
        TSet<FName> Seen;
        for (const TSharedPtr<FJsonValue>& Value : *Values)
        {
            if (!Value.IsValid() || Value->Type != EJson::String || Value->AsString().IsEmpty()) return false;
            const FName Name(*Value->AsString());
            if (Name.IsNone() || Seen.Contains(Name)) return false;
            Seen.Add(Name);
            OutValues.Add(Name);
        }
        return bAllowEmpty || !OutValues.IsEmpty();
    }

    bool ReadRequiredName(const TSharedPtr<FJsonObject>& Object, const TCHAR* FieldName, FName& OutValue)
    {
        FString Value;
        if (!Object.IsValid() || !Object->TryGetStringField(FieldName, Value) || Value.IsEmpty()) return false;
        OutValue = FName(*Value);
        return !OutValue.IsNone();
    }

    bool IsSubset(const TArray<FName>& Candidate, const TArray<FName>& Allowed)
    {
        for (const FName Value : Candidate)
        {
            if (!Allowed.Contains(Value)) return false;
        }
        return true;
    }

    bool HasUniqueNames(const TArray<FName>& Values)
    {
        TSet<FName> Seen;
        for (const FName Value : Values)
        {
            if (Value.IsNone() || Seen.Contains(Value)) return false;
            Seen.Add(Value);
        }
        return true;
    }

    int32 CountUniqueValid(const TArray<FName>& Values, const TArray<FName>& Allowed)
    {
        TSet<FName> Seen;
        for (const FName Value : Values)
        {
            if (!Allowed.Contains(Value)) return -1;
            Seen.Add(Value);
        }
        return Seen.Num();
    }

    bool ParseCommunicationChallenge(const TSharedPtr<FJsonObject>& Object, FWMCommunicationChallengeDefinition& OutChallenge)
    {
        if (!ReadRequiredName(Object, TEXT("challengeId"), OutChallenge.ChallengeId) ||
            !ReadRequiredName(Object, TEXT("targetLanguageId"), OutChallenge.TargetLanguageId) ||
            !ReadRequiredName(Object, TEXT("contextId"), OutChallenge.ContextId) ||
            !ReadRequiredName(Object, TEXT("requiredMeaningId"), OutChallenge.RequiredMeaningId) ||
            !ReadNameArray(Object, TEXT("acceptedRegisterIds"), OutChallenge.AcceptedRegisterIds))
        {
            return false;
        }

        const TArray<TSharedPtr<FJsonValue>>* Options = nullptr;
        if (!Object->TryGetArrayField(TEXT("options"), Options) || !Options || Options->IsEmpty()) return false;
        for (const TSharedPtr<FJsonValue>& Value : *Options)
        {
            const TSharedPtr<FJsonObject>* OptionObject = nullptr;
            if (!Value.IsValid() || !Value->TryGetObject(OptionObject) || !OptionObject || !OptionObject->IsValid()) return false;
            FWMCommunicationOptionDefinition Option;
            if (!ReadRequiredName(*OptionObject, TEXT("choiceId"), Option.ChoiceId) ||
                !ReadRequiredName(*OptionObject, TEXT("languageId"), Option.LanguageId) ||
                !ReadRequiredName(*OptionObject, TEXT("meaningId"), Option.MeaningId) ||
                !ReadRequiredName(*OptionObject, TEXT("registerId"), Option.RegisterId) ||
                !ReadRequiredName(*OptionObject, TEXT("syntaxPatternId"), Option.SyntaxPatternId) ||
                !ReadRequiredName(*OptionObject, TEXT("evidenceEventId"), Option.EvidenceEventId))
            {
                return false;
            }
            OutChallenge.Options.Add(MoveTemp(Option));
        }
        return OutChallenge.IsSane();
    }

    bool ParseNarrativeStory(const TSharedPtr<FJsonObject>& Object, FWMNarrativeStoryDefinition& OutStory)
    {
        if (!ReadRequiredName(Object, TEXT("storyId"), OutStory.StoryId) ||
            !ReadRequiredName(Object, TEXT("traditionId"), OutStory.TraditionId) ||
            !ReadRequiredName(Object, TEXT("sourceClassId"), OutStory.SourceClassId) ||
            !ReadRequiredName(Object, TEXT("provenanceKey"), OutStory.ProvenanceKey) ||
            !ReadRequiredName(Object, TEXT("culturalReviewState"), OutStory.CulturalReviewState) ||
            !ReadRequiredName(Object, TEXT("startNodeId"), OutStory.StartNodeId))
        {
            return false;
        }

        const TArray<TSharedPtr<FJsonValue>>* Nodes = nullptr;
        const TArray<TSharedPtr<FJsonValue>>* Choices = nullptr;
        if (!Object->TryGetArrayField(TEXT("nodes"), Nodes) || !Nodes || Nodes->IsEmpty() ||
            !Object->TryGetArrayField(TEXT("choices"), Choices) || !Choices || Choices->IsEmpty())
        {
            return false;
        }

        for (const TSharedPtr<FJsonValue>& Value : *Nodes)
        {
            const TSharedPtr<FJsonObject>* NodeObject = nullptr;
            if (!Value.IsValid() || !Value->TryGetObject(NodeObject) || !NodeObject || !NodeObject->IsValid()) return false;
            FWMNarrativeNodeDefinition Node;
            if (!ReadRequiredName(*NodeObject, TEXT("nodeId"), Node.NodeId) ||
                !ReadRequiredName(*NodeObject, TEXT("passageKey"), Node.PassageKey) ||
                !ReadRequiredName(*NodeObject, TEXT("pointOfViewId"), Node.PointOfViewId)) return false;
            OutStory.Nodes.Add(MoveTemp(Node));
        }

        for (const TSharedPtr<FJsonValue>& Value : *Choices)
        {
            const TSharedPtr<FJsonObject>* ChoiceObject = nullptr;
            if (!Value.IsValid() || !Value->TryGetObject(ChoiceObject) || !ChoiceObject || !ChoiceObject->IsValid()) return false;
            FWMNarrativeChoiceDefinition Choice;
            if (!ReadRequiredName(*ChoiceObject, TEXT("choiceId"), Choice.ChoiceId) ||
                !ReadRequiredName(*ChoiceObject, TEXT("fromNodeId"), Choice.FromNodeId) ||
                !ReadRequiredName(*ChoiceObject, TEXT("toNodeId"), Choice.ToNodeId) ||
                !ReadRequiredName(*ChoiceObject, TEXT("inferenceTag"), Choice.InferenceTag) ||
                !ReadRequiredName(*ChoiceObject, TEXT("evidenceEventId"), Choice.EvidenceEventId)) return false;
            OutStory.Choices.Add(MoveTemp(Choice));
        }
        return OutStory.IsSane();
    }

    bool ParseEthicalDilemma(const TSharedPtr<FJsonObject>& Object, FWMEthicalDilemmaDefinition& OutDilemma)
    {
        if (!ReadRequiredName(Object, TEXT("dilemmaId"), OutDilemma.DilemmaId) ||
            !ReadNameArray(Object, TEXT("perspectiveIds"), OutDilemma.PerspectiveIds) ||
            !Object->TryGetNumberField(TEXT("requiredPerspectiveCount"), OutDilemma.RequiredPerspectiveCount))
        {
            return false;
        }

        const TArray<TSharedPtr<FJsonValue>>* Options = nullptr;
        if (!Object->TryGetArrayField(TEXT("options"), Options) || !Options || Options->Num() < 2) return false;
        for (const TSharedPtr<FJsonValue>& Value : *Options)
        {
            const TSharedPtr<FJsonObject>* OptionObject = nullptr;
            if (!Value.IsValid() || !Value->TryGetObject(OptionObject) || !OptionObject || !OptionObject->IsValid()) return false;
            FWMEthicalOptionDefinition Option;
            if (!ReadRequiredName(*OptionObject, TEXT("optionId"), Option.OptionId) ||
                !ReadNameArray(*OptionObject, TEXT("supportedReasonIds"), Option.SupportedReasonIds) ||
                !ReadNameArray(*OptionObject, TEXT("affectedPerspectiveIds"), Option.AffectedPerspectiveIds) ||
                !ReadNameArray(*OptionObject, TEXT("tradeoffTags"), Option.TradeoffTags) ||
                !ReadRequiredName(*OptionObject, TEXT("evidenceEventId"), Option.EvidenceEventId)) return false;

            const TSharedPtr<FJsonObject>* Consequences = nullptr;
            if (!(*OptionObject)->TryGetObjectField(TEXT("consequenceProfile"), Consequences) || !Consequences || !Consequences->IsValid()) return false;
            for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : (*Consequences)->Get()->Values)
            {
                if (!Pair.Value.IsValid() || Pair.Value->Type != EJson::Number) return false;
                Option.ConsequenceProfile.Add(FName(*Pair.Key), static_cast<float>(Pair.Value->AsNumber()));
            }
            OutDilemma.Options.Add(MoveTemp(Option));
        }
        return OutDilemma.IsSane();
    }

    bool ParsePhilosophyProblem(const TSharedPtr<FJsonObject>& Object, FWMPhilosophyProblemDefinition& OutProblem)
    {
        if (!ReadRequiredName(Object, TEXT("problemId"), OutProblem.ProblemId) ||
            !ReadNameArray(Object, TEXT("claimIds"), OutProblem.ClaimIds) ||
            !ReadNameArray(Object, TEXT("assumptionIds"), OutProblem.AssumptionIds) ||
            !ReadNameArray(Object, TEXT("counterexampleIds"), OutProblem.CounterexampleIds) ||
            !ReadRequiredName(Object, TEXT("evidenceEventId"), OutProblem.EvidenceEventId) ||
            !Object->TryGetNumberField(TEXT("minReasonLinks"), OutProblem.MinReasonLinks) ||
            !Object->TryGetNumberField(TEXT("minAssumptions"), OutProblem.MinAssumptions) ||
            !Object->TryGetNumberField(TEXT("minCounterexamples"), OutProblem.MinCounterexamples) ||
            !Object->TryGetNumberField(TEXT("minRevisions"), OutProblem.MinRevisions))
        {
            return false;
        }

        const TArray<TSharedPtr<FJsonValue>>* Links = nullptr;
        if (!Object->TryGetArrayField(TEXT("reasonLinks"), Links) || !Links || Links->IsEmpty()) return false;
        for (const TSharedPtr<FJsonValue>& Value : *Links)
        {
            const TSharedPtr<FJsonObject>* LinkObject = nullptr;
            if (!Value.IsValid() || !Value->TryGetObject(LinkObject) || !LinkObject || !LinkObject->IsValid()) return false;
            FWMArgumentLinkDefinition Link;
            if (!ReadRequiredName(*LinkObject, TEXT("linkId"), Link.LinkId) ||
                !ReadRequiredName(*LinkObject, TEXT("fromPropositionId"), Link.FromPropositionId) ||
                !ReadRequiredName(*LinkObject, TEXT("toPropositionId"), Link.ToPropositionId) ||
                !ReadRequiredName(*LinkObject, TEXT("relationId"), Link.RelationId)) return false;
            OutProblem.ReasonLinks.Add(MoveTemp(Link));
        }
        return OutProblem.IsSane();
    }
}

bool FWMCommunicationOptionDefinition::IsSane() const
{
    return !ChoiceId.IsNone() && !LanguageId.IsNone() && !MeaningId.IsNone() && !RegisterId.IsNone() &&
        !SyntaxPatternId.IsNone() && !EvidenceEventId.IsNone();
}

const FWMCommunicationOptionDefinition* FWMCommunicationChallengeDefinition::FindOption(const FName ChoiceId) const
{
    return Options.FindByPredicate([ChoiceId](const FWMCommunicationOptionDefinition& Option) { return Option.ChoiceId == ChoiceId; });
}

bool FWMCommunicationChallengeDefinition::IsSane() const
{
    if (ChallengeId.IsNone() || TargetLanguageId.IsNone() || ContextId.IsNone() || RequiredMeaningId.IsNone() ||
        AcceptedRegisterIds.IsEmpty() || Options.Num() < 2 || !HasUniqueNames(AcceptedRegisterIds)) return false;
    TSet<FName> ChoiceIds;
    bool bHasSuccessfulChoice = false;
    for (const FWMCommunicationOptionDefinition& Option : Options)
    {
        if (!Option.IsSane() || ChoiceIds.Contains(Option.ChoiceId)) return false;
        ChoiceIds.Add(Option.ChoiceId);
        bHasSuccessfulChoice |= Option.LanguageId == TargetLanguageId && Option.MeaningId == RequiredMeaningId && AcceptedRegisterIds.Contains(Option.RegisterId);
    }
    return bHasSuccessfulChoice;
}

bool FWMNarrativeNodeDefinition::IsSane() const
{
    return !NodeId.IsNone() && !PassageKey.IsNone() && !PointOfViewId.IsNone();
}

bool FWMNarrativeChoiceDefinition::IsSane() const
{
    return !ChoiceId.IsNone() && !FromNodeId.IsNone() && !ToNodeId.IsNone() && !InferenceTag.IsNone() && !EvidenceEventId.IsNone();
}

const FWMNarrativeNodeDefinition* FWMNarrativeStoryDefinition::FindNode(const FName NodeId) const
{
    return Nodes.FindByPredicate([NodeId](const FWMNarrativeNodeDefinition& Node) { return Node.NodeId == NodeId; });
}

const FWMNarrativeChoiceDefinition* FWMNarrativeStoryDefinition::FindChoice(const FName CurrentNodeId, const FName ChoiceId) const
{
    return Choices.FindByPredicate([CurrentNodeId, ChoiceId](const FWMNarrativeChoiceDefinition& Choice)
    {
        return Choice.FromNodeId == CurrentNodeId && Choice.ChoiceId == ChoiceId;
    });
}

bool FWMNarrativeStoryDefinition::IsSane() const
{
    static const TSet<FName> AllowedSourceClasses = { TEXT("retelling"), TEXT("adaptation"), TEXT("historical-source") };
    static const TSet<FName> AllowedReviewStates = { TEXT("draft"), TEXT("approved"), TEXT("changes-requested") };
    if (StoryId.IsNone() || TraditionId.IsNone() || ProvenanceKey.IsNone() || !AllowedSourceClasses.Contains(SourceClassId) ||
        !AllowedReviewStates.Contains(CulturalReviewState) || StartNodeId.IsNone() || Nodes.Num() < 2 || Choices.IsEmpty()) return false;

    TSet<FName> NodeIds;
    for (const FWMNarrativeNodeDefinition& Node : Nodes)
    {
        if (!Node.IsSane() || NodeIds.Contains(Node.NodeId)) return false;
        NodeIds.Add(Node.NodeId);
    }
    if (!NodeIds.Contains(StartNodeId)) return false;

    TSet<FString> ChoiceKeys;
    for (const FWMNarrativeChoiceDefinition& Choice : Choices)
    {
        if (!Choice.IsSane() || !NodeIds.Contains(Choice.FromNodeId) || !NodeIds.Contains(Choice.ToNodeId)) return false;
        const FString Key = Choice.FromNodeId.ToString() + TEXT("|") + Choice.ChoiceId.ToString();
        if (ChoiceKeys.Contains(Key)) return false;
        ChoiceKeys.Add(Key);
    }
    return true;
}

bool FWMEthicalOptionDefinition::IsSane() const
{
    if (OptionId.IsNone() || SupportedReasonIds.IsEmpty() || AffectedPerspectiveIds.IsEmpty() || TradeoffTags.IsEmpty() ||
        ConsequenceProfile.IsEmpty() || EvidenceEventId.IsNone() || !HasUniqueNames(SupportedReasonIds) ||
        !HasUniqueNames(AffectedPerspectiveIds) || !HasUniqueNames(TradeoffTags)) return false;
    for (const TPair<FName, float>& Pair : ConsequenceProfile)
    {
        if (Pair.Key.IsNone() || !FMath::IsFinite(Pair.Value) || Pair.Value < -1.0f || Pair.Value > 1.0f) return false;
    }
    return true;
}

const FWMEthicalOptionDefinition* FWMEthicalDilemmaDefinition::FindOption(const FName OptionId) const
{
    return Options.FindByPredicate([OptionId](const FWMEthicalOptionDefinition& Option) { return Option.OptionId == OptionId; });
}

bool FWMEthicalDilemmaDefinition::IsSane() const
{
    if (DilemmaId.IsNone() || PerspectiveIds.Num() < 2 || RequiredPerspectiveCount < 1 || RequiredPerspectiveCount > PerspectiveIds.Num() ||
        Options.Num() < 2 || !HasUniqueNames(PerspectiveIds)) return false;
    TSet<FName> OptionIds;
    for (const FWMEthicalOptionDefinition& Option : Options)
    {
        if (!Option.IsSane() || OptionIds.Contains(Option.OptionId) || !IsSubset(Option.AffectedPerspectiveIds, PerspectiveIds)) return false;
        OptionIds.Add(Option.OptionId);
    }
    return true;
}

bool FWMArgumentLinkDefinition::IsSane() const
{
    static const TSet<FName> AllowedRelations = { TEXT("supports"), TEXT("challenges"), TEXT("clarifies") };
    return !LinkId.IsNone() && !FromPropositionId.IsNone() && !ToPropositionId.IsNone() && AllowedRelations.Contains(RelationId);
}

bool FWMPhilosophyProblemDefinition::IsSane() const
{
    if (ProblemId.IsNone() || ClaimIds.IsEmpty() || AssumptionIds.IsEmpty() || CounterexampleIds.IsEmpty() || ReasonLinks.IsEmpty() ||
        EvidenceEventId.IsNone() || MinReasonLinks < 1 || MinAssumptions < 1 || MinCounterexamples < 1 || MinRevisions < 1 ||
        MinReasonLinks > ReasonLinks.Num() || MinAssumptions > AssumptionIds.Num() || MinCounterexamples > CounterexampleIds.Num() ||
        !HasUniqueNames(ClaimIds) || !HasUniqueNames(AssumptionIds) || !HasUniqueNames(CounterexampleIds)) return false;
    TSet<FName> LinkIds;
    for (const FWMArgumentLinkDefinition& Link : ReasonLinks)
    {
        if (!Link.IsSane() || LinkIds.Contains(Link.LinkId)) return false;
        LinkIds.Add(Link.LinkId);
    }
    return true;
}

const FWMCommunicationChallengeDefinition* FWMLanguageThoughtCatalog::FindCommunicationChallenge(const FName ChallengeId) const
{
    return CommunicationChallenges.Find(ChallengeId);
}

const FWMNarrativeStoryDefinition* FWMLanguageThoughtCatalog::FindNarrativeStory(const FName StoryId) const
{
    return NarrativeStories.Find(StoryId);
}

const FWMEthicalDilemmaDefinition* FWMLanguageThoughtCatalog::FindEthicalDilemma(const FName DilemmaId) const
{
    return EthicalDilemmas.Find(DilemmaId);
}

const FWMPhilosophyProblemDefinition* FWMLanguageThoughtCatalog::FindPhilosophyProblem(const FName ProblemId) const
{
    return PhilosophyProblems.Find(ProblemId);
}

bool FWMLanguageThoughtCatalog::IsSane() const
{
    if (SchemaVersion != 1 || PrivacyModel != StableIdsPrivacyModel || CommunicationChallenges.IsEmpty() || NarrativeStories.IsEmpty() ||
        EthicalDilemmas.IsEmpty() || PhilosophyProblems.IsEmpty()) return false;
    for (const TPair<FName, FWMCommunicationChallengeDefinition>& Pair : CommunicationChallenges)
        if (Pair.Key != Pair.Value.ChallengeId || !Pair.Value.IsSane()) return false;
    for (const TPair<FName, FWMNarrativeStoryDefinition>& Pair : NarrativeStories)
        if (Pair.Key != Pair.Value.StoryId || !Pair.Value.IsSane()) return false;
    for (const TPair<FName, FWMEthicalDilemmaDefinition>& Pair : EthicalDilemmas)
        if (Pair.Key != Pair.Value.DilemmaId || !Pair.Value.IsSane()) return false;
    for (const TPair<FName, FWMPhilosophyProblemDefinition>& Pair : PhilosophyProblems)
        if (Pair.Key != Pair.Value.ProblemId || !Pair.Value.IsSane()) return false;
    return true;
}

bool FWMLanguageThoughtCatalog::TryParseJson(const FString& Json, FWMLanguageThoughtCatalog& OutCatalog, FString& OutError)
{
    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        OutError = TEXT("Language/thought JSON is not a valid object.");
        return false;
    }

    FWMLanguageThoughtCatalog Candidate;
    FString PrivacyModelString;
    const TArray<TSharedPtr<FJsonValue>>* Challenges = nullptr;
    const TArray<TSharedPtr<FJsonValue>>* Stories = nullptr;
    const TArray<TSharedPtr<FJsonValue>>* Dilemmas = nullptr;
    const TArray<TSharedPtr<FJsonValue>>* Problems = nullptr;
    if (!Root->TryGetNumberField(TEXT("schemaVersion"), Candidate.SchemaVersion) ||
        !Root->TryGetBoolField(TEXT("prototypeOnly"), Candidate.bPrototypeOnly) ||
        !Root->TryGetStringField(TEXT("privacyModel"), PrivacyModelString) || PrivacyModelString.IsEmpty() ||
        !Root->TryGetArrayField(TEXT("communicationChallenges"), Challenges) || !Challenges ||
        !Root->TryGetArrayField(TEXT("narrativeStories"), Stories) || !Stories ||
        !Root->TryGetArrayField(TEXT("ethicalDilemmas"), Dilemmas) || !Dilemmas ||
        !Root->TryGetArrayField(TEXT("philosophyProblems"), Problems) || !Problems)
    {
        OutError = TEXT("Language/thought catalog header is invalid.");
        return false;
    }
    Candidate.PrivacyModel = FName(*PrivacyModelString);

    for (const TSharedPtr<FJsonValue>& Value : *Challenges)
    {
        const TSharedPtr<FJsonObject>* Object = nullptr;
        FWMCommunicationChallengeDefinition Challenge;
        if (!Value.IsValid() || !Value->TryGetObject(Object) || !Object || !Object->IsValid() || !ParseCommunicationChallenge(*Object, Challenge) ||
            Candidate.CommunicationChallenges.Contains(Challenge.ChallengeId))
        {
            OutError = TEXT("Communication challenge failed validation.");
            return false;
        }
        Candidate.CommunicationChallenges.Add(Challenge.ChallengeId, MoveTemp(Challenge));
    }

    for (const TSharedPtr<FJsonValue>& Value : *Stories)
    {
        const TSharedPtr<FJsonObject>* Object = nullptr;
        FWMNarrativeStoryDefinition Story;
        if (!Value.IsValid() || !Value->TryGetObject(Object) || !Object || !Object->IsValid() || !ParseNarrativeStory(*Object, Story) ||
            Candidate.NarrativeStories.Contains(Story.StoryId))
        {
            OutError = TEXT("Narrative story failed validation.");
            return false;
        }
        Candidate.NarrativeStories.Add(Story.StoryId, MoveTemp(Story));
    }

    for (const TSharedPtr<FJsonValue>& Value : *Dilemmas)
    {
        const TSharedPtr<FJsonObject>* Object = nullptr;
        FWMEthicalDilemmaDefinition Dilemma;
        if (!Value.IsValid() || !Value->TryGetObject(Object) || !Object || !Object->IsValid() || !ParseEthicalDilemma(*Object, Dilemma) ||
            Candidate.EthicalDilemmas.Contains(Dilemma.DilemmaId))
        {
            OutError = TEXT("Ethical dilemma failed validation.");
            return false;
        }
        Candidate.EthicalDilemmas.Add(Dilemma.DilemmaId, MoveTemp(Dilemma));
    }

    for (const TSharedPtr<FJsonValue>& Value : *Problems)
    {
        const TSharedPtr<FJsonObject>* Object = nullptr;
        FWMPhilosophyProblemDefinition Problem;
        if (!Value.IsValid() || !Value->TryGetObject(Object) || !Object || !Object->IsValid() || !ParsePhilosophyProblem(*Object, Problem) ||
            Candidate.PhilosophyProblems.Contains(Problem.ProblemId))
        {
            OutError = TEXT("Philosophy problem failed validation.");
            return false;
        }
        Candidate.PhilosophyProblems.Add(Problem.ProblemId, MoveTemp(Problem));
    }

    if (!Candidate.IsSane())
    {
        OutError = TEXT("Language/thought catalog failed semantic validation.");
        return false;
    }
    OutCatalog = MoveTemp(Candidate);
    OutError.Reset();
    return true;
}

bool FWMLanguageThoughtRuntime::EvaluateCommunication(
    const FWMCommunicationChallengeDefinition& Challenge,
    const FName ChoiceId,
    FWMThoughtEvidenceResult& OutResult)
{
    OutResult = FWMThoughtEvidenceResult();
    if (!Challenge.IsSane()) return false;
    const FWMCommunicationOptionDefinition* Option = Challenge.FindOption(ChoiceId);
    if (!Option || Option->LanguageId != Challenge.TargetLanguageId || Option->MeaningId != Challenge.RequiredMeaningId ||
        !Challenge.AcceptedRegisterIds.Contains(Option->RegisterId)) return false;
    OutResult.bAccepted = true;
    OutResult.PrimitiveId = CommunicatePrimitive;
    OutResult.EvidenceEventId = Option->EvidenceEventId;
    OutResult.NumericValue = 1.0f;
    return true;
}

bool FWMLanguageThoughtRuntime::TraverseNarrative(
    const FWMNarrativeStoryDefinition& Story,
    const FName CurrentNodeId,
    const FName ChoiceId,
    FWMThoughtEvidenceResult& OutResult)
{
    OutResult = FWMThoughtEvidenceResult();
    if (!Story.IsSane() || !Story.FindNode(CurrentNodeId)) return false;
    const FWMNarrativeChoiceDefinition* Choice = Story.FindChoice(CurrentNodeId, ChoiceId);
    if (!Choice || !Story.FindNode(Choice->ToNodeId)) return false;
    OutResult.bAccepted = true;
    OutResult.PrimitiveId = InterpretPrimitive;
    OutResult.EvidenceEventId = Choice->EvidenceEventId;
    OutResult.NextNodeId = Choice->ToNodeId;
    OutResult.NumericValue = 1.0f;
    return true;
}

bool FWMLanguageThoughtRuntime::EvaluateEthicalReasoning(
    const FWMEthicalDilemmaDefinition& Dilemma,
    const FName OptionId,
    const TArray<FName>& ReasonIds,
    const TArray<FName>& PerspectiveIds,
    const bool bAcknowledgedTradeoff,
    FWMThoughtEvidenceResult& OutResult)
{
    OutResult = FWMThoughtEvidenceResult();
    if (!Dilemma.IsSane() || !bAcknowledgedTradeoff) return false;
    const FWMEthicalOptionDefinition* Option = Dilemma.FindOption(OptionId);
    if (!Option || CountUniqueValid(ReasonIds, Option->SupportedReasonIds) < 1 ||
        CountUniqueValid(PerspectiveIds, Dilemma.PerspectiveIds) < Dilemma.RequiredPerspectiveCount) return false;
    OutResult.bAccepted = true;
    OutResult.PrimitiveId = EthicsPrimitive;
    OutResult.EvidenceEventId = Option->EvidenceEventId;
    OutResult.NumericValue = 1.0f;
    return true;
}

bool FWMLanguageThoughtRuntime::EvaluatePhilosophicalArgument(
    const FWMPhilosophyProblemDefinition& Problem,
    const FName ClaimId,
    const TArray<FName>& ReasonLinkIds,
    const TArray<FName>& AssumptionIds,
    const TArray<FName>& CounterexampleIds,
    const int32 RevisionCount,
    FWMThoughtEvidenceResult& OutResult)
{
    OutResult = FWMThoughtEvidenceResult();
    if (!Problem.IsSane() || !Problem.ClaimIds.Contains(ClaimId) || RevisionCount < Problem.MinRevisions) return false;

    TArray<FName> AllowedLinkIds;
    for (const FWMArgumentLinkDefinition& Link : Problem.ReasonLinks) AllowedLinkIds.Add(Link.LinkId);
    if (CountUniqueValid(ReasonLinkIds, AllowedLinkIds) < Problem.MinReasonLinks ||
        CountUniqueValid(AssumptionIds, Problem.AssumptionIds) < Problem.MinAssumptions ||
        CountUniqueValid(CounterexampleIds, Problem.CounterexampleIds) < Problem.MinCounterexamples) return false;

    bool bConnectsToClaim = false;
    for (const FName LinkId : ReasonLinkIds)
    {
        const FWMArgumentLinkDefinition* Link = Problem.ReasonLinks.FindByPredicate([LinkId](const FWMArgumentLinkDefinition& Candidate)
        {
            return Candidate.LinkId == LinkId;
        });
        if (Link && Link->ToPropositionId == ClaimId) bConnectsToClaim = true;
    }
    if (!bConnectsToClaim) return false;

    OutResult.bAccepted = true;
    OutResult.PrimitiveId = ArgumentPrimitive;
    OutResult.EvidenceEventId = Problem.EvidenceEventId;
    OutResult.NumericValue = 1.0f;
    return true;
}
