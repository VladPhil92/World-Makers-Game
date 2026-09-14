#include "Adventure/WMEclipseEngineExperience.h"

#include "Dom/JsonObject.h"
#include "Mission/WMMissionTypes.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
    const TSet<FName> AllowedProducerKinds = { TEXT("building"), TEXT("science"), TEXT("thought"), TEXT("world") };
    const TSet<FName> AllowedVerbs = {
        TEXT("observe"), TEXT("tune"), TEXT("build"), TEXT("engage"), TEXT("rotate"), TEXT("align"), TEXT("inspect"),
        TEXT("test"), TEXT("route"), TEXT("ignite"), TEXT("communicate"), TEXT("decode"), TEXT("awaken"),
        TEXT("interpret"), TEXT("reveal"), TEXT("rethink"), TEXT("synchronize")
    };

    bool ReadName(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field, FName& Out)
    {
        FString Value;
        if (!Object.IsValid() || !Object->TryGetStringField(Field, Value) || Value.IsEmpty()) return false;
        Out = FName(*Value);
        return !Out.IsNone();
    }

    bool ReadNameArray(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field, TArray<FName>& Out, bool bAllowEmpty = false)
    {
        const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
        if (!Object.IsValid() || !Object->TryGetArrayField(Field, Values) || !Values) return false;
        Out.Reset();
        TSet<FName> Seen;
        for (const TSharedPtr<FJsonValue>& Value : *Values)
        {
            if (!Value.IsValid() || Value->Type != EJson::String || Value->AsString().IsEmpty()) return false;
            const FName Name(*Value->AsString());
            if (Name.IsNone() || Seen.Contains(Name)) return false;
            Seen.Add(Name);
            Out.Add(Name);
        }
        return bAllowEmpty || !Out.IsEmpty();
    }

    bool ReadLocation(const TSharedPtr<FJsonObject>& Object, FVector& Out)
    {
        const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
        if (!Object.IsValid() || !Object->TryGetArrayField(TEXT("prototypeLocationCm"), Values) || !Values || Values->Num() != 3) return false;
        double X = 0.0, Y = 0.0, Z = 0.0;
        if (!(*Values)[0]->TryGetNumber(X) || !(*Values)[1]->TryGetNumber(Y) || !(*Values)[2]->TryGetNumber(Z)) return false;
        Out = FVector(static_cast<float>(X), static_cast<float>(Y), static_cast<float>(Z));
        return !Out.ContainsNaN() && Out.Size() <= 20000.0f;
    }

    bool ParseEvidence(const TSharedPtr<FJsonObject>& Object, FWMEclipseEvidenceRoute& Out)
    {
        return ReadName(Object, TEXT("objectiveId"), Out.ObjectiveId) &&
            ReadName(Object, TEXT("disciplineId"), Out.DisciplineId) &&
            ReadName(Object, TEXT("producerKind"), Out.ProducerKind) &&
            ReadName(Object, TEXT("producerRefId"), Out.ProducerRefId) &&
            ReadName(Object, TEXT("primitiveId"), Out.PrimitiveId) &&
            ReadName(Object, TEXT("evidenceEventId"), Out.EvidenceEventId) && Out.IsSane();
    }

    bool ParseWorldState(const TSharedPtr<FJsonObject>& Object, FWMEclipseWorldStateRoute& Out)
    {
        return ReadName(Object, TEXT("producerKind"), Out.ProducerKind) &&
            ReadName(Object, TEXT("producerRefId"), Out.ProducerRefId) &&
            ReadName(Object, TEXT("worldStateId"), Out.WorldStateId) && Out.IsSane();
    }

    bool ParseAction(const TSharedPtr<FJsonObject>& Object, FWMEclipseActionDefinition& Out)
    {
        if (!ReadName(Object, TEXT("actionId"), Out.ActionId) || !ReadName(Object, TEXT("interactionVerb"), Out.InteractionVerb) ||
            !ReadName(Object, TEXT("promptKey"), Out.PromptKey) || !ReadName(Object, TEXT("feedbackKey"), Out.FeedbackKey) ||
            !ReadName(Object, TEXT("firstPersonEventId"), Out.FirstPersonEventId) || !ReadName(Object, TEXT("firstPersonActionId"), Out.FirstPersonActionId) ||
            !ReadLocation(Object, Out.PrototypeLocationCm) || !ReadNameArray(Object, TEXT("hints"), Out.HintKeys)) return false;

        if (Object->HasTypedField<EJson::Object>(TEXT("evidence")))
        {
            Out.bHasEvidenceRoute = ParseEvidence(Object->GetObjectField(TEXT("evidence")), Out.Evidence);
            if (!Out.bHasEvidenceRoute) return false;
        }
        if (Object->HasTypedField<EJson::Object>(TEXT("worldState")))
        {
            Out.bHasWorldStateRoute = ParseWorldState(Object->GetObjectField(TEXT("worldState")), Out.WorldState);
            if (!Out.bHasWorldStateRoute) return false;
        }
        return Out.IsSane();
    }

    bool ParseChapter(const TSharedPtr<FJsonObject>& Object, FWMEclipseChapterExperience& Out)
    {
        if (!ReadName(Object, TEXT("chapterId"), Out.ChapterId) || !ReadName(Object, TEXT("fantasyGoalKey"), Out.FantasyGoalKey) ||
            !ReadName(Object, TEXT("tensionKey"), Out.TensionKey) || !ReadName(Object, TEXT("completionReactionKey"), Out.CompletionReactionKey)) return false;
        const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
        if (!Object->TryGetArrayField(TEXT("actions"), Values) || !Values || Values->IsEmpty()) return false;
        for (const TSharedPtr<FJsonValue>& Value : *Values)
        {
            const TSharedPtr<FJsonObject>* ActionObject = nullptr;
            if (!Value.IsValid() || !Value->TryGetObject(ActionObject) || !ActionObject || !ActionObject->IsValid()) return false;
            FWMEclipseActionDefinition Action;
            if (!ParseAction(*ActionObject, Action)) return false;
            Out.Actions.Add(MoveTemp(Action));
        }
        return Out.IsSane();
    }
}

bool FWMEclipseEvidenceRoute::IsSane() const
{
    return !ObjectiveId.IsNone() && !DisciplineId.IsNone() && AllowedProducerKinds.Contains(ProducerKind) &&
        !ProducerRefId.IsNone() && FWMMissionRuntimeDefinition::IsSupportedEvidencePrimitive(PrimitiveId) && !EvidenceEventId.IsNone();
}

bool FWMEclipseWorldStateRoute::IsSane() const
{
    return AllowedProducerKinds.Contains(ProducerKind) && !ProducerRefId.IsNone() && !WorldStateId.IsNone();
}

bool FWMEclipseActionDefinition::IsSane() const
{
    return !ActionId.IsNone() && AllowedVerbs.Contains(InteractionVerb) && !PromptKey.IsNone() && !FeedbackKey.IsNone() &&
        !FirstPersonEventId.IsNone() && !FirstPersonActionId.IsNone() && !PrototypeLocationCm.ContainsNaN() &&
        !HintKeys.IsEmpty() && HintKeys.Num() <= 3 && (bHasEvidenceRoute != bHasWorldStateRoute) &&
        (!bHasEvidenceRoute || Evidence.IsSane()) && (!bHasWorldStateRoute || WorldState.IsSane());
}

const FWMEclipseActionDefinition* FWMEclipseChapterExperience::FindAction(const FName ActionId) const
{
    return Actions.FindByPredicate([ActionId](const FWMEclipseActionDefinition& Action) { return Action.ActionId == ActionId; });
}

bool FWMEclipseChapterExperience::IsSane() const
{
    if (ChapterId.IsNone() || FantasyGoalKey.IsNone() || TensionKey.IsNone() || CompletionReactionKey.IsNone() || Actions.IsEmpty()) return false;
    TSet<FName> Ids;
    bool bHasWorldState = false;
    for (const FWMEclipseActionDefinition& Action : Actions)
    {
        if (!Action.IsSane() || Ids.Contains(Action.ActionId)) return false;
        Ids.Add(Action.ActionId);
        bHasWorldState |= Action.bHasWorldStateRoute;
    }
    return bHasWorldState;
}

const FWMEclipseChapterExperience* FWMEclipseExperienceCatalog::FindChapter(const FName ChapterId) const
{
    return Chapters.FindByPredicate([ChapterId](const FWMEclipseChapterExperience& Chapter) { return Chapter.ChapterId == ChapterId; });
}

bool FWMEclipseExperienceCatalog::IsSane() const
{
    if (SchemaVersion != 1 || ExperienceId != TEXT("experience.eclipse-engine-v1") || EpicId != TEXT("epic.eclipse-engine") || !bPrototypeOnly ||
        PlayerPromise != TEXT("repair-an-impossible-celestial-machine-through-discovery") || PresentationRule != TEXT("world-first-no-school-ui") ||
        RewardModel != TEXT("world-transformation-and-new-capability") || FailureModel != TEXT("reversible-experimentation-with-visible-consequence") ||
        HintModel != TEXT("player-requested-progressive-environmental-hints") || PrivacyModel != TEXT("stable-ids-no-child-free-text") || Chapters.Num() != 6) return false;

    TSet<FName> ChapterIds;
    TSet<FName> ActionIds;
    for (const FWMEclipseChapterExperience& Chapter : Chapters)
    {
        if (!Chapter.IsSane() || ChapterIds.Contains(Chapter.ChapterId)) return false;
        ChapterIds.Add(Chapter.ChapterId);
        for (const FWMEclipseActionDefinition& Action : Chapter.Actions)
        {
            if (ActionIds.Contains(Action.ActionId)) return false;
            ActionIds.Add(Action.ActionId);
        }
    }
    return ForbiddenPlayerFacingTerms.Num() >= 8;
}

bool FWMEclipseExperienceCatalog::TryParseJson(const FString& Json, FWMEclipseExperienceCatalog& OutCatalog, FString& OutError)
{
    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid()) { OutError = TEXT("Eclipse experience JSON is invalid."); return false; }

    FWMEclipseExperienceCatalog Candidate;
    int32 Version = 0;
    bool bPrototypeOnly = true;
    if (!Root->TryGetNumberField(TEXT("schemaVersion"), Version) || !Root->TryGetBoolField(TEXT("prototypeOnly"), bPrototypeOnly) ||
        !ReadName(Root, TEXT("experienceId"), Candidate.ExperienceId) || !ReadName(Root, TEXT("epicId"), Candidate.EpicId) ||
        !ReadName(Root, TEXT("playerPromise"), Candidate.PlayerPromise) || !ReadName(Root, TEXT("presentationRule"), Candidate.PresentationRule) ||
        !ReadName(Root, TEXT("rewardModel"), Candidate.RewardModel) || !ReadName(Root, TEXT("failureModel"), Candidate.FailureModel) ||
        !ReadName(Root, TEXT("hintModel"), Candidate.HintModel) || !ReadName(Root, TEXT("privacyModel"), Candidate.PrivacyModel) ||
        !ReadNameArray(Root, TEXT("forbiddenPlayerFacingTerms"), Candidate.ForbiddenPlayerFacingTerms))
    {
        OutError = TEXT("Eclipse experience header is invalid."); return false;
    }
    Candidate.SchemaVersion = Version;
    Candidate.bPrototypeOnly = bPrototypeOnly;

    const TArray<TSharedPtr<FJsonValue>>* Chapters = nullptr;
    if (!Root->TryGetArrayField(TEXT("chapters"), Chapters) || !Chapters || Chapters->IsEmpty()) { OutError = TEXT("Eclipse experience has no chapters."); return false; }
    for (const TSharedPtr<FJsonValue>& Value : *Chapters)
    {
        const TSharedPtr<FJsonObject>* ChapterObject = nullptr;
        if (!Value.IsValid() || !Value->TryGetObject(ChapterObject) || !ChapterObject || !ChapterObject->IsValid()) { OutError = TEXT("Eclipse chapter is invalid."); return false; }
        FWMEclipseChapterExperience Chapter;
        if (!ParseChapter(*ChapterObject, Chapter)) { OutError = TEXT("Eclipse chapter failed semantic validation."); return false; }
        Candidate.Chapters.Add(MoveTemp(Chapter));
    }

    if (!Candidate.IsSane()) { OutError = TEXT("Eclipse experience failed semantic validation."); return false; }
    OutCatalog = MoveTemp(Candidate);
    OutError.Reset();
    return true;
}

FName FWMEclipseHintRuntime::RequestHint(const FWMEclipseActionDefinition& Action)
{
    if (!Action.IsSane() || Action.HintKeys.IsEmpty()) return NAME_None;
    int32& Level = HintLevels.FindOrAdd(Action.ActionId);
    const int32 AttemptCount = Attempts.FindRef(Action.ActionId);
    const int32 MaxLevelFromAttempts = FMath::Clamp(AttemptCount, 0, Action.HintKeys.Num() - 1);
    Level = FMath::Min(FMath::Max(Level, MaxLevelFromAttempts), Action.HintKeys.Num() - 1);
    const FName Result = Action.HintKeys[Level];
    Level = FMath::Min(Level + 1, Action.HintKeys.Num() - 1);
    return Result;
}

void FWMEclipseHintRuntime::RecordAttempt(const FName ActionId)
{
    if (!ActionId.IsNone()) Attempts.FindOrAdd(ActionId) = FMath::Min(Attempts.FindRef(ActionId) + 1, 20);
}

int32 FWMEclipseHintRuntime::GetAttemptCount(const FName ActionId) const
{
    return Attempts.FindRef(ActionId);
}

void FWMEclipseHintRuntime::Reset()
{
    Attempts.Reset();
    HintLevels.Reset();
}
