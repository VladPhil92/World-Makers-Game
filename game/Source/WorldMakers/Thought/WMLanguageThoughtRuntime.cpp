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

    bool ReadName(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field, FName& OutValue)
    {
        FString Value;
        if (!Object.IsValid() || !Object->TryGetStringField(Field, Value) || Value.IsEmpty()) return false;
        OutValue = FName(*Value);
        return !OutValue.IsNone();
    }

    bool ReadNames(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field, TArray<FName>& OutValues)
    {
        const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
        if (!Object.IsValid() || !Object->TryGetArrayField(Field, Values) || !Values || Values->IsEmpty()) return false;
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

    bool IsSubset(const TArray<FName>& Values, const TArray<FName>& Allowed)
    {
        for (const FName Value : Values) if (!Allowed.Contains(Value)) return false;
        return true;
    }

    int32 CountUniqueAllowed(const TArray<FName>& Values, const TArray<FName>& Allowed)
    {
        TSet<FName> Seen;
        for (const FName Value : Values)
        {
            if (!Allowed.Contains(Value)) return -1;
            Seen.Add(Value);
        }
        return Seen.Num();
    }

    bool ParseCommunication(const TSharedPtr<FJsonObject>& Object, FWMCommunicationChallengeDefinition& Out)
    {
        if (!ReadName(Object, TEXT("challengeId"), Out.ChallengeId) ||
            !ReadName(Object, TEXT("targetLanguageId"), Out.TargetLanguageId) ||
            !ReadName(Object, TEXT("contextId"), Out.ContextId) ||
            !ReadName(Object, TEXT("requiredMeaningId"), Out.RequiredMeaningId) ||
            !ReadNames(Object, TEXT("acceptedRegisterIds"), Out.AcceptedRegisterIds)) return false;

        const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
        if (!Object->TryGetArrayField(TEXT("options"), Values) || !Values || Values->IsEmpty()) return false;
        for (const TSharedPtr<FJsonValue>& Value : *Values)
        {
            const TSharedPtr<FJsonObject>* Item = nullptr;
            if (!Value.IsValid() || !Value->TryGetObject(Item) || !Item || !Item->IsValid()) return false;
            FWMCommunicationOptionDefinition Option;
            if (!ReadName(*Item, TEXT("choiceId"), Option.ChoiceId) ||
                !ReadName(*Item, TEXT("languageId"), Option.LanguageId) ||
                !ReadName(*Item, TEXT("meaningId"), Option.MeaningId) ||
                !ReadName(*Item, TEXT("registerId"), Option.RegisterId) ||
                !ReadName(*Item, TEXT("syntaxPatternId"), Option.SyntaxPatternId) ||
                !ReadName(*Item, TEXT("evidenceEventId"), Option.EvidenceEventId)) return false;
            Out.Options.Add(MoveTemp(Option));
        }
        return Out.IsSane();
    }

    bool ParseStory(const TSharedPtr<FJsonObject>& Object, FWMNarrativeStoryDefinition& Out)
    {
        if (!ReadName(Object, TEXT("storyId"), Out.StoryId) ||
            !ReadName(Object, TEXT("traditionId"), Out.TraditionId) ||
            !ReadName(Object, TEXT("sourceClassId"), Out.SourceClassId) ||
            !ReadName(Object, TEXT("provenanceKey"), Out.ProvenanceKey) ||
            !ReadName(Object, TEXT("culturalReviewState"), Out.CulturalReviewState) ||
            !ReadName(Object, TEXT("startNodeId"), Out.StartNodeId)) return false;

        const TArray<TSharedPtr<FJsonValue>>* Nodes = nullptr;
        const TArray<TSharedPtr<FJsonValue>>* Choices = nullptr;
        if (!Object->TryGetArrayField(TEXT("nodes"), Nodes) || !Nodes || Nodes->IsEmpty() ||
            !Object->TryGetArrayField(TEXT("choices"), Choices) || !Choices || Choices->IsEmpty()) return false;

        for (const TSharedPtr<FJsonValue>& Value : *Nodes)
        {
            const TSharedPtr<FJsonObject>* Item = nullptr;
            if (!Value.IsValid() || !Value->TryGetObject(Item) || !Item || !Item->IsValid()) return false;
            FWMNarrativeNodeDefinition Node;
            if (!ReadName(*Item, TEXT("nodeId"), Node.NodeId) ||
                !ReadName(*Item, TEXT("passageKey"), Node.PassageKey) ||
                !ReadName(*Item, TEXT("pointOfViewId"), Node.PointOfViewId)) return false;
            Out.Nodes.Add(MoveTemp(Node));
        }

        for (const TSharedPtr<FJsonValue>& Value : *Choices)
        {
            const TSharedPtr<FJsonObject>* Item = nullptr;
            if (!Value.IsValid() || !Value->TryGetObject(Item) || !Item || !Item->IsValid()) return false;
            FWMNarrativeChoiceDefinition Choice;
            if (!ReadName(*Item, TEXT("choiceId"), Choice.ChoiceId) ||
                !ReadName(*Item, TEXT("fromNodeId"), Choice.FromNodeId) ||
                !ReadName(*Item, TEXT("toNodeId"), Choice.ToNodeId) ||
                !ReadName(*Item, TEXT("inferenceTag"), Choice.InferenceTag) ||
                !ReadName(*Item, TEXT("evidenceEventId"), Choice.EvidenceEventId)) return false;
            Out.Choices.Add(MoveTemp(Choice));
        }
        return Out.IsSane();
    }

    bool ParseDilemma(const TSharedPtr<FJsonObject>& Object, FWMEthicalDilemmaDefinition& Out)
    {
        if (!ReadName(Object, TEXT("dilemmaId"), Out.DilemmaId) ||
            !ReadNames(Object, TEXT("perspectiveIds"), Out.PerspectiveIds) ||
            !Object->TryGetNumberField(TEXT("requiredPerspectiveCount"), Out.RequiredPerspectiveCount)) return false;

        const TArray<TSharedPtr<FJsonValue>>* Options = nullptr;
        if (!Object->TryGetArrayField(TEXT("options"), Options) || !Options || Options->Num() < 2) return false;
        for (const TSharedPtr<FJsonValue>& Value : *Options)
        {
            const TSharedPtr<FJsonObject>* Item = nullptr;
            if (!Value.IsValid() || !Value->TryGetObject(Item) || !Item || !Item->IsValid()) return false;
            FWMEthicalOptionDefinition Option;
            if (!ReadName(*Item, TEXT("optionId"), Option.OptionId) ||
                !ReadNames(*Item, TEXT("supportedReasonIds"), Option.SupportedReasonIds) ||
                !ReadNames(*Item, TEXT("affectedPerspectiveIds"), Option.AffectedPerspectiveIds) ||
                !ReadNames(*Item, TEXT("tradeoffTags"), Option.TradeoffTags) ||
                !ReadName(*Item, TEXT("evidenceEventId"), Option.EvidenceEventId)) return false;

            const TSharedPtr<FJsonObject>* Consequences = nullptr;
            if (!(*Item)->TryGetObjectField(TEXT("consequenceProfile"), Consequences) || !Consequences || !Consequences->IsValid()) return false;
            for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : (*Consequences)->Values)
            {
                if (!Pair.Value.IsValid() || Pair.Value->Type != EJson::Number) return false;
                Option.ConsequenceProfile.Add(FName(*Pair.Key), static_cast<float>(Pair.Value->AsNumber()));
            }
            Out.Options.Add(MoveTemp(Option));
        }
        return Out.IsSane();
    }

    bool ParseProblem(const TSharedPtr<FJsonObject>& Object, FWMPhilosophyProblemDefinition& Out)
    {
        if (!ReadName(Object, TEXT("problemId"), Out.ProblemId) ||
            !ReadNames(Object, TEXT("claimIds"), Out.ClaimIds) ||
            !ReadNames(Object, TEXT("assumptionIds"), Out.AssumptionIds) ||
            !ReadNames(Object, TEXT("counterexampleIds"), Out.CounterexampleIds) ||
            !ReadName(Object, TEXT("evidenceEventId"), Out.EvidenceEventId) ||
            !Object->TryGetNumberField(TEXT("minReasonLinks"), Out.MinReasonLinks) ||
            !Object->TryGetNumberField(TEXT("minAssumptions"), Out.MinAssumptions) ||
            !Object->TryGetNumberField(TEXT("minCounterexamples"), Out.MinCounterexamples) ||
            !Object->TryGetNumberField(TEXT("minRevisions"), Out.MinRevisions)) return false;

        const TArray<TSharedPtr<FJsonValue>>* Links = nullptr;
        if (!Object->TryGetArrayField(TEXT("reasonLinks"), Links) || !Links || Links->IsEmpty()) return false;
        for (const TSharedPtr<FJsonValue>& Value : *Links)
        {
            const TSharedPtr<FJsonObject>* Item = nullptr;
            if (!Value.IsValid() || !Value->TryGetObject(Item) || !Item || !Item->IsValid()) return false;
            FWMArgumentLinkDefinition Link;
            if (!ReadName(*Item, TEXT("linkId"), Link.LinkId) ||
                !ReadName(*Item, TEXT("fromPropositionId"), Link.FromPropositionId) ||
                !ReadName(*Item, TEXT("toPropositionId"), Link.ToPropositionId) ||
                !ReadName(*Item, TEXT("relationId"), Link.RelationId)) return false;
            Out.ReasonLinks.Add(MoveTemp(Link));
        }
        return Out.IsSane();
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
    bool bHasPassingOption = false;
    for (const FWMCommunicationOptionDefinition& Option : Options)
    {
        if (!Option.IsSane() || ChoiceIds.Contains(Option.ChoiceId)) return false;
        ChoiceIds.Add(Option.ChoiceId);
        bHasPassingOption |= Option.LanguageId == TargetLanguageId && Option.MeaningId == RequiredMeaningId && AcceptedRegisterIds.Contains(Option.RegisterId);
    }
    return bHasPassingOption;
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
    const TSet<FName> SourceClasses = { FName(TEXT("retelling")), FName(TEXT("adaptation")), FName(TEXT("historical-source")) };
    const TSet<FName> ReviewStates = { FName(TEXT("draft")), FName(TEXT("approved")), FName(TEXT("changes-requested")) };
    if (StoryId.IsNone() || TraditionId.IsNone() || ProvenanceKey.IsNone() || !SourceClasses.Contains(SourceClassId) ||
        !ReviewStates.Contains(CulturalReviewState) || StartNodeId.IsNone() || Nodes.Num() < 2 || Choices.IsEmpty()) return false;

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
    const TSet<FName> Relations = { FName(TEXT("supports")), FName(TEXT("challenges")), FName(TEXT("clarifies")) };
    return !LinkId.IsNone() && !FromPropositionId.IsNone() && !ToPropositionId.IsNone() && Relations.Contains(RelationId);
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

const FWMCommunicationChallengeDefinition* FWMLanguageThoughtCatalog::FindCommunicationChallenge(const FName ChallengeId) const { return CommunicationChallenges.Find(ChallengeId); }
const FWMNarrativeStoryDefinition* FWMLanguageThoughtCatalog::FindNarrativeStory(const FName StoryId) const { return NarrativeStories.Find(StoryId); }
const FWMEthicalDilemmaDefinition* FWMLanguageThoughtCatalog::FindEthicalDilemma(const FName DilemmaId) const { return EthicalDilemmas.Find(DilemmaId); }
const FWMPhilosophyProblemDefinition* FWMLanguageThoughtCatalog::FindPhilosophyProblem(const FName ProblemId) const { return PhilosophyProblems.Find(ProblemId); }

bool FWMLanguageThoughtCatalog::IsSane() const
{
    if (SchemaVersion != 1 || PrivacyModel != StableIdsPrivacyModel || CommunicationChallenges.IsEmpty() || NarrativeStories.IsEmpty() ||
        EthicalDilemmas.IsEmpty() || PhilosophyProblems.IsEmpty()) return false;
    for (const TPair<FName, FWMCommunicationChallengeDefinition>& Pair : CommunicationChallenges) if (Pair.Key != Pair.Value.ChallengeId || !Pair.Value.IsSane()) return false;
    for (const TPair<FName, FWMNarrativeStoryDefinition>& Pair : NarrativeStories) if (Pair.Key != Pair.Value.StoryId || !Pair.Value.IsSane()) return false;
    for (const TPair<FName, FWMEthicalDilemmaDefinition>& Pair : EthicalDilemmas) if (Pair.Key != Pair.Value.DilemmaId || !Pair.Value.IsSane()) return false;
    for (const TPair<FName, FWMPhilosophyProblemDefinition>& Pair : PhilosophyProblems) if (Pair.Key != Pair.Value.ProblemId || !Pair.Value.IsSane()) return false;
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
    FString Privacy;
    const TArray<TSharedPtr<FJsonValue>>* Challenges = nullptr;
    const TArray<TSharedPtr<FJsonValue>>* Stories = nullptr;
    const TArray<TSharedPtr<FJsonValue>>* Dilemmas = nullptr;
    const TArray<TSharedPtr<FJsonValue>>* Problems = nullptr;
    if (!Root->TryGetNumberField(TEXT("schemaVersion"), Candidate.SchemaVersion) ||
        !Root->TryGetBoolField(TEXT("prototypeOnly"), Candidate.bPrototypeOnly) ||
        !Root->TryGetStringField(TEXT("privacyModel"), Privacy) || Privacy.IsEmpty() ||
        !Root->TryGetArrayField(TEXT("communicationChallenges"), Challenges) || !Challenges ||
        !Root->TryGetArrayField(TEXT("narrativeStories"), Stories) || !Stories ||
        !Root->TryGetArrayField(TEXT("ethicalDilemmas"), Dilemmas) || !Dilemmas ||
        !Root->TryGetArrayField(TEXT("philosophyProblems"), Problems) || !Problems)
    {
        OutError = TEXT("Language/thought catalog header is invalid.");
        return false;
    }
    Candidate.PrivacyModel = FName(*Privacy);

    for (const TSharedPtr<FJsonValue>& Value : *Challenges)
    {
        const TSharedPtr<FJsonObject>* Object = nullptr;
        FWMCommunicationChallengeDefinition Item;
        if (!Value.IsValid() || !Value->TryGetObject(Object) || !Object || !Object->IsValid() || !ParseCommunication(*Object, Item) || Candidate.CommunicationChallenges.Contains(Item.ChallengeId))
        { OutError = TEXT("Communication challenge failed validation."); return false; }
        Candidate.CommunicationChallenges.Add(Item.ChallengeId, MoveTemp(Item));
    }
    for (const TSharedPtr<FJsonValue>& Value : *Stories)
    {
        const TSharedPtr<FJsonObject>* Object = nullptr;
        FWMNarrativeStoryDefinition Item;
        if (!Value.IsValid() || !Value->TryGetObject(Object) || !Object || !Object->IsValid() || !ParseStory(*Object, Item) || Candidate.NarrativeStories.Contains(Item.StoryId))
        { OutError = TEXT("Narrative story failed validation."); return false; }
        Candidate.NarrativeStories.Add(Item.StoryId, MoveTemp(Item));
    }
    for (const TSharedPtr<FJsonValue>& Value : *Dilemmas)
    {
        const TSharedPtr<FJsonObject>* Object = nullptr;
        FWMEthicalDilemmaDefinition Item;
        if (!Value.IsValid() || !Value->TryGetObject(Object) || !Object || !Object->IsValid() || !ParseDilemma(*Object, Item) || Candidate.EthicalDilemmas.Contains(Item.DilemmaId))
        { OutError = TEXT("Ethical dilemma failed validation."); return false; }
        Candidate.EthicalDilemmas.Add(Item.DilemmaId, MoveTemp(Item));
    }
    for (const TSharedPtr<FJsonValue>& Value : *Problems)
    {
        const TSharedPtr<FJsonObject>* Object = nullptr;
        FWMPhilosophyProblemDefinition Item;
        if (!Value.IsValid() || !Value->TryGetObject(Object) || !Object || !Object->IsValid() || !ParseProblem(*Object, Item) || Candidate.PhilosophyProblems.Contains(Item.ProblemId))
        { OutError = TEXT("Philosophy problem failed validation."); return false; }
        Candidate.PhilosophyProblems.Add(Item.ProblemId, MoveTemp(Item));
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

bool FWMLanguageThoughtRuntime::EvaluateCommunication(const FWMCommunicationChallengeDefinition& Challenge, const FName ChoiceId, FWMThoughtEvidenceResult& OutResult)
{
    OutResult = FWMThoughtEvidenceResult();
    if (!Challenge.IsSane()) return false;
    const FWMCommunicationOptionDefinition* Option = Challenge.FindOption(ChoiceId);
    if (!Option || Option->LanguageId != Challenge.TargetLanguageId || Option->MeaningId != Challenge.RequiredMeaningId || !Challenge.AcceptedRegisterIds.Contains(Option->RegisterId)) return false;
    OutResult.bAccepted = true;
    OutResult.PrimitiveId = CommunicatePrimitive;
    OutResult.EvidenceEventId = Option->EvidenceEventId;
    return true;
}

bool FWMLanguageThoughtRuntime::TraverseNarrative(const FWMNarrativeStoryDefinition& Story, const FName CurrentNodeId, const FName ChoiceId, FWMThoughtEvidenceResult& OutResult)
{
    OutResult = FWMThoughtEvidenceResult();
    if (!Story.IsSane() || !Story.FindNode(CurrentNodeId)) return false;
    const FWMNarrativeChoiceDefinition* Choice = Story.FindChoice(CurrentNodeId, ChoiceId);
    if (!Choice || !Story.FindNode(Choice->ToNodeId)) return false;
    OutResult.bAccepted = true;
    OutResult.PrimitiveId = InterpretPrimitive;
    OutResult.EvidenceEventId = Choice->EvidenceEventId;
    OutResult.NextNodeId = Choice->ToNodeId;
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
    if (!Option || CountUniqueAllowed(ReasonIds, Option->SupportedReasonIds) < 1 || CountUniqueAllowed(PerspectiveIds, Dilemma.PerspectiveIds) < Dilemma.RequiredPerspectiveCount) return false;
    OutResult.bAccepted = true;
    OutResult.PrimitiveId = EthicsPrimitive;
    OutResult.EvidenceEventId = Option->EvidenceEventId;
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
    TArray<FName> AllowedLinks;
    for (const FWMArgumentLinkDefinition& Link : Problem.ReasonLinks) AllowedLinks.Add(Link.LinkId);
    if (CountUniqueAllowed(ReasonLinkIds, AllowedLinks) < Problem.MinReasonLinks ||
        CountUniqueAllowed(AssumptionIds, Problem.AssumptionIds) < Problem.MinAssumptions ||
        CountUniqueAllowed(CounterexampleIds, Problem.CounterexampleIds) < Problem.MinCounterexamples) return false;

    bool bReasonTargetsClaim = false;
    for (const FName LinkId : ReasonLinkIds)
    {
        const FWMArgumentLinkDefinition* Link = Problem.ReasonLinks.FindByPredicate([LinkId](const FWMArgumentLinkDefinition& Candidate) { return Candidate.LinkId == LinkId; });
        if (Link && Link->ToPropositionId == ClaimId) bReasonTargetsClaim = true;
    }
    if (!bReasonTargetsClaim) return false;

    OutResult.bAccepted = true;
    OutResult.PrimitiveId = ArgumentPrimitive;
    OutResult.EvidenceEventId = Problem.EvidenceEventId;
    return true;
}
