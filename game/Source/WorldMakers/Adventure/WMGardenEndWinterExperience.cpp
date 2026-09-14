#include "Adventure/WMGardenEndWinterExperience.h"

#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
    const TSet<FName> GardenDisciplines = {TEXT("biology"),TEXT("chemistry"),TEXT("ecology"),TEXT("mathematics"),TEXT("ethics"),TEXT("philosophy-for-children")};
    const TSet<FName> GardenVerbs = {TEXT("tune"),TEXT("route"),TEXT("awaken"),TEXT("test"),TEXT("balance"),TEXT("release"),TEXT("observe"),TEXT("deliberate"),TEXT("rethink")};

    bool ReadName(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field, FName& Out)
    {
        FString Value;
        if (!Object.IsValid() || !Object->TryGetStringField(Field, Value) || Value.IsEmpty()) return false;
        Out = FName(*Value);
        return !Out.IsNone();
    }

    bool ReadNames(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field, TArray<FName>& Out)
    {
        const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
        if (!Object.IsValid() || !Object->TryGetArrayField(Field, Values) || !Values || Values->IsEmpty()) return false;
        TSet<FName> Seen;
        for (const TSharedPtr<FJsonValue>& Value : *Values)
        {
            if (!Value.IsValid() || Value->Type != EJson::String || Value->AsString().IsEmpty()) return false;
            FName Name(*Value->AsString());
            if (Name.IsNone() || Seen.Contains(Name)) return false;
            Seen.Add(Name); Out.Add(Name);
        }
        return true;
    }

    bool ReadLocation(const TSharedPtr<FJsonObject>& Object, FVector& Out)
    {
        const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
        if (!Object.IsValid() || !Object->TryGetArrayField(TEXT("prototypeLocationCm"), Values) || !Values || Values->Num()!=3) return false;
        double X=0,Y=0,Z=0;
        if (!(*Values)[0]->TryGetNumber(X) || !(*Values)[1]->TryGetNumber(Y) || !(*Values)[2]->TryGetNumber(Z)) return false;
        Out=FVector(static_cast<float>(X),static_cast<float>(Y),static_cast<float>(Z));
        return !Out.ContainsNaN() && Out.Size() <= 20000.0f;
    }

    bool ParseEvidence(const TSharedPtr<FJsonObject>& O, FWMEclipseEvidenceRoute& Out)
    {
        return ReadName(O,TEXT("objectiveId"),Out.ObjectiveId) && ReadName(O,TEXT("disciplineId"),Out.DisciplineId) &&
            ReadName(O,TEXT("producerKind"),Out.ProducerKind) && ReadName(O,TEXT("producerRefId"),Out.ProducerRefId) &&
            ReadName(O,TEXT("primitiveId"),Out.PrimitiveId) && ReadName(O,TEXT("evidenceEventId"),Out.EvidenceEventId) && Out.IsSane();
    }

    bool ParseState(const TSharedPtr<FJsonObject>& O, FWMEclipseWorldStateRoute& Out)
    {
        return ReadName(O,TEXT("producerKind"),Out.ProducerKind) && ReadName(O,TEXT("producerRefId"),Out.ProducerRefId) &&
            ReadName(O,TEXT("worldStateId"),Out.WorldStateId) && Out.IsSane();
    }

    bool ParseMastery(const TSharedPtr<FJsonObject>& O, FWMGardenMasteryGateRoute& Out)
    {
        return ReadName(O,TEXT("gateId"),Out.GateId) && ReadName(O,TEXT("disciplineId"),Out.DisciplineId) && Out.IsSane();
    }

    bool ParseAction(const TSharedPtr<FJsonObject>& O, FWMGardenActionDefinition& Out)
    {
        if (!ReadName(O,TEXT("actionId"),Out.ActionId) || !ReadName(O,TEXT("interactionVerb"),Out.InteractionVerb) ||
            !ReadName(O,TEXT("promptKey"),Out.PromptKey) || !ReadName(O,TEXT("feedbackKey"),Out.FeedbackKey) ||
            !ReadName(O,TEXT("firstPersonEventId"),Out.FirstPersonEventId) || !ReadName(O,TEXT("firstPersonActionId"),Out.FirstPersonActionId) ||
            !ReadLocation(O,Out.PrototypeLocationCm) || !ReadNames(O,TEXT("hints"),Out.HintKeys)) return false;
        if (O->HasTypedField<EJson::Object>(TEXT("evidence"))) Out.bHasEvidenceRoute = ParseEvidence(O->GetObjectField(TEXT("evidence")),Out.Evidence);
        if (O->HasTypedField<EJson::Object>(TEXT("worldState"))) Out.bHasWorldStateRoute = ParseState(O->GetObjectField(TEXT("worldState")),Out.WorldState);
        if (O->HasTypedField<EJson::Object>(TEXT("masteryGate"))) Out.bHasMasteryGate = ParseMastery(O->GetObjectField(TEXT("masteryGate")),Out.MasteryGate);
        return Out.IsSane();
    }

    bool ParseChapter(const TSharedPtr<FJsonObject>& O, FWMGardenChapterExperience& Out)
    {
        if (!ReadName(O,TEXT("chapterId"),Out.ChapterId) || !ReadName(O,TEXT("fantasyGoalKey"),Out.FantasyGoalKey) ||
            !ReadName(O,TEXT("tensionKey"),Out.TensionKey) || !ReadName(O,TEXT("completionReactionKey"),Out.CompletionReactionKey)) return false;
        const TArray<TSharedPtr<FJsonValue>>* Values=nullptr;
        if (!O->TryGetArrayField(TEXT("actions"),Values) || !Values || Values->IsEmpty()) return false;
        for (const TSharedPtr<FJsonValue>& Value:*Values)
        {
            const TSharedPtr<FJsonObject>* AO=nullptr; FWMGardenActionDefinition Action;
            if (!Value.IsValid() || !Value->TryGetObject(AO) || !AO || !AO->IsValid() || !ParseAction(*AO,Action)) return false;
            Out.Actions.Add(MoveTemp(Action));
        }
        return Out.IsSane();
    }
}

bool FWMGardenMasteryGateRoute::IsSane() const
{
    return !GateId.IsNone() && GateId.ToString().StartsWith(TEXT("mastery.garden.")) && GardenDisciplines.Contains(DisciplineId);
}

bool FWMGardenActionDefinition::IsSane() const
{
    const int32 RouteCount=(bHasEvidenceRoute?1:0)+(bHasWorldStateRoute?1:0)+(bHasMasteryGate?1:0);
    return !ActionId.IsNone() && GardenVerbs.Contains(InteractionVerb) && !PromptKey.IsNone() && !FeedbackKey.IsNone() &&
        !FirstPersonEventId.IsNone() && !FirstPersonActionId.IsNone() && !PrototypeLocationCm.ContainsNaN() &&
        !HintKeys.IsEmpty() && HintKeys.Num()<=3 && RouteCount==1 &&
        (!bHasEvidenceRoute || Evidence.IsSane()) && (!bHasWorldStateRoute || WorldState.IsSane()) && (!bHasMasteryGate || MasteryGate.IsSane());
}

const FWMGardenActionDefinition* FWMGardenChapterExperience::FindAction(const FName ActionId) const
{
    return Actions.FindByPredicate([ActionId](const FWMGardenActionDefinition& A){return A.ActionId==ActionId;});
}

TArray<FName> FWMGardenChapterExperience::GetRequiredMasteryGateIds() const
{
    TArray<FName> Result;
    for (const FWMGardenActionDefinition& A:Actions) if (A.bHasMasteryGate) Result.Add(A.MasteryGate.GateId);
    return Result;
}

bool FWMGardenChapterExperience::IsSane() const
{
    if (ChapterId.IsNone() || FantasyGoalKey.IsNone() || TensionKey.IsNone() || CompletionReactionKey.IsNone() || Actions.Num()<2) return false;
    TSet<FName> IDs; int32 StateCount=0;
    for (const FWMGardenActionDefinition& A:Actions)
    {
        if (!A.IsSane() || IDs.Contains(A.ActionId)) return false;
        IDs.Add(A.ActionId); StateCount += A.bHasWorldStateRoute ? 1 : 0;
    }
    return StateCount==1 && Actions.Last().bHasWorldStateRoute;
}

const FWMGardenChapterExperience* FWMGardenExperienceCatalog::FindChapter(const FName ChapterId) const
{
    return Chapters.FindByPredicate([ChapterId](const FWMGardenChapterExperience& C){return C.ChapterId==ChapterId;});
}

bool FWMGardenExperienceCatalog::IsSane() const
{
    if (SchemaVersion!=1 || ExperienceId!=TEXT("experience.garden-end-winter-v1") || EpicId!=TEXT("epic.garden-end-winter") || !bPrototypeOnly ||
        PlayerPromise!=TEXT("bring-a-frozen-living-garden-back-into-motion") || PresentationRule!=TEXT("world-first-no-school-ui") ||
        RewardModel!=TEXT("living-world-transformation-and-new-access") || FailureModel!=TEXT("reversible-ecosystem-experimentation") ||
        HintModel!=TEXT("player-requested-progressive-environmental-hints") || PrivacyModel!=TEXT("stable-ids-no-child-free-text") || Chapters.Num()!=4) return false;
    TSet<FName> ChapterIDs, ActionIDs; TSet<FName> MasteryDisciplines;
    for (const FWMGardenChapterExperience& C:Chapters)
    {
        if (!C.IsSane() || ChapterIDs.Contains(C.ChapterId)) return false;
        ChapterIDs.Add(C.ChapterId);
        for (const FWMGardenActionDefinition& A:C.Actions)
        {
            if (ActionIDs.Contains(A.ActionId)) return false;
            ActionIDs.Add(A.ActionId);
            if (A.bHasMasteryGate) MasteryDisciplines.Add(A.MasteryGate.DisciplineId);
        }
    }
    return MasteryDisciplines.Contains(TEXT("mathematics")) && MasteryDisciplines.Contains(TEXT("philosophy-for-children"));
}

bool FWMGardenExperienceCatalog::TryParseJson(const FString& Json, FWMGardenExperienceCatalog& OutCatalog, FString& OutError)
{
    TSharedPtr<FJsonObject> Root; const TSharedRef<TJsonReader<>> Reader=TJsonReaderFactory<>::Create(Json);
    if (!FJsonSerializer::Deserialize(Reader,Root) || !Root.IsValid()){OutError=TEXT("Garden experience JSON invalid.");return false;}
    FWMGardenExperienceCatalog C; bool bPrototype=true;
    if (!Root->TryGetNumberField(TEXT("schemaVersion"),C.SchemaVersion) || !Root->TryGetBoolField(TEXT("prototypeOnly"),bPrototype) ||
        !ReadName(Root,TEXT("experienceId"),C.ExperienceId) || !ReadName(Root,TEXT("epicId"),C.EpicId) ||
        !ReadName(Root,TEXT("playerPromise"),C.PlayerPromise) || !ReadName(Root,TEXT("presentationRule"),C.PresentationRule) ||
        !ReadName(Root,TEXT("rewardModel"),C.RewardModel) || !ReadName(Root,TEXT("failureModel"),C.FailureModel) ||
        !ReadName(Root,TEXT("hintModel"),C.HintModel) || !ReadName(Root,TEXT("privacyModel"),C.PrivacyModel)) {OutError=TEXT("Garden header invalid.");return false;}
    C.bPrototypeOnly=bPrototype;
    const TArray<TSharedPtr<FJsonValue>>* Values=nullptr;
    if (!Root->TryGetArrayField(TEXT("chapters"),Values) || !Values) {OutError=TEXT("Garden chapters missing.");return false;}
    for (const TSharedPtr<FJsonValue>& Value:*Values)
    {
        const TSharedPtr<FJsonObject>* CO=nullptr; FWMGardenChapterExperience Chapter;
        if (!Value.IsValid() || !Value->TryGetObject(CO) || !CO || !CO->IsValid() || !ParseChapter(*CO,Chapter)){OutError=TEXT("Garden chapter invalid.");return false;}
        C.Chapters.Add(MoveTemp(Chapter));
    }
    if (!C.IsSane()){OutError=TEXT("Garden semantic validation failed.");return false;}
    OutCatalog=MoveTemp(C); OutError.Reset(); return true;
}

FName FWMGardenHintRuntime::RequestHint(const FWMGardenActionDefinition& Action)
{
    if (!Action.IsSane() || Action.HintKeys.IsEmpty()) return NAME_None;
    int32& Level=HintLevels.FindOrAdd(Action.ActionId);
    Level=FMath::Clamp(FMath::Max(Level,Attempts.FindRef(Action.ActionId)),0,Action.HintKeys.Num()-1);
    const FName Result=Action.HintKeys[Level]; Level=FMath::Min(Level+1,Action.HintKeys.Num()-1); return Result;
}
void FWMGardenHintRuntime::RecordAttempt(const FName ActionId){if(!ActionId.IsNone())Attempts.FindOrAdd(ActionId)=FMath::Min(Attempts.FindRef(ActionId)+1,20);}
void FWMGardenHintRuntime::Reset(){Attempts.Reset();HintLevels.Reset();}
