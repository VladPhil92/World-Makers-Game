#include "Adventure/WMAdventureRuntime.h"

#include "Dom/JsonObject.h"
#include "Mission/WMMissionTypes.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
    const TSet<FName> FirstClassDisciplines = {
        TEXT("mathematics"), TEXT("geometry"), TEXT("english-language"), TEXT("spanish-language"),
        TEXT("literature"), TEXT("biology"), TEXT("chemistry"), TEXT("physics"), TEXT("ecology"),
        TEXT("ethics"), TEXT("philosophy-for-children")
    };
    const TSet<FName> AllowedSecondaryDisciplines = {
        TEXT("mathematics"), TEXT("geometry"), TEXT("english-language"), TEXT("spanish-language"),
        TEXT("literature"), TEXT("biology"), TEXT("chemistry"), TEXT("physics"), TEXT("ecology"),
        TEXT("ethics"), TEXT("philosophy-for-children"), TEXT("history-culture")
    };
    const TSet<FName> AllowedProducerKinds = { TEXT("building"), TEXT("science"), TEXT("thought"), TEXT("world") };
    const TSet<FName> AllowedAgeBands = { TEXT("4-6"), TEXT("7-8"), TEXT("9-10") };

    bool NameField(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field, FName& Out)
    {
        FString Value;
        if (!Object.IsValid() || !Object->TryGetStringField(Field, Value) || Value.IsEmpty()) return false;
        Out = FName(*Value);
        return !Out.IsNone();
    }

    bool NameArray(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field, TArray<FName>& Out, const bool bAllowEmpty)
    {
        const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
        if (!Object.IsValid() || !Object->TryGetArrayField(Field, Values) || !Values) return false;
        Out.Reset();
        TSet<FName> Seen;
        for (const TSharedPtr<FJsonValue>& Value : *Values)
        {
            if (!Value.IsValid() || Value->Type != EJson::String || Value->AsString().IsEmpty()) return false;
            const FName Id(*Value->AsString());
            if (Id.IsNone() || Seen.Contains(Id)) return false;
            Seen.Add(Id);
            Out.Add(Id);
        }
        return bAllowEmpty || !Out.IsEmpty();
    }

    bool ParseBeat(const TSharedPtr<FJsonObject>& Object, FWMAdventureBeatDefinition& Out)
    {
        int32 Count = 0;
        if (!NameField(Object, TEXT("beatId"), Out.BeatId) ||
            !NameField(Object, TEXT("primitiveId"), Out.PrimitiveId) ||
            !NameField(Object, TEXT("evidenceEventId"), Out.EvidenceEventId) ||
            !NameField(Object, TEXT("producerKind"), Out.ProducerKind) ||
            !NameField(Object, TEXT("producerRefId"), Out.ProducerRefId) ||
            !NameField(Object, TEXT("promptKey"), Out.PromptKey) ||
            !NameField(Object, TEXT("formalizationKey"), Out.FormalizationKey) ||
            !Object->TryGetNumberField(TEXT("requiredCount"), Count)) return false;
        Out.RequiredCount = Count;
        return Out.IsSane();
    }

    bool ParseAdventure(const TSharedPtr<FJsonObject>& Object, FWMFantasticAdventureDefinition& Out)
    {
        if (!NameField(Object, TEXT("adventureId"), Out.AdventureId) ||
            !NameField(Object, TEXT("missionId"), Out.MissionId) ||
            !NameField(Object, TEXT("titleKey"), Out.TitleKey) ||
            !NameField(Object, TEXT("premiseKey"), Out.PremiseKey) ||
            !NameField(Object, TEXT("primaryDiscipline"), Out.PrimaryDiscipline) ||
            !NameField(Object, TEXT("ageBand"), Out.AgeBand) ||
            !NameArray(Object, TEXT("secondaryDisciplines"), Out.SecondaryDisciplines, true)) return false;

        const TArray<TSharedPtr<FJsonValue>>* Beats = nullptr;
        if (!Object->TryGetArrayField(TEXT("beats"), Beats) || !Beats || Beats->IsEmpty()) return false;
        for (const TSharedPtr<FJsonValue>& Value : *Beats)
        {
            const TSharedPtr<FJsonObject>* BeatObject = nullptr;
            if (!Value.IsValid() || !Value->TryGetObject(BeatObject) || !BeatObject || !BeatObject->IsValid()) return false;
            FWMAdventureBeatDefinition Beat;
            if (!ParseBeat(*BeatObject, Beat)) return false;
            Out.Beats.Add(MoveTemp(Beat));
        }
        return Out.IsSane();
    }
}

bool FWMAdventureBeatDefinition::IsSane() const
{
    return !BeatId.IsNone() && FWMMissionRuntimeDefinition::IsSupportedEvidencePrimitive(PrimitiveId) &&
        !EvidenceEventId.IsNone() && AllowedProducerKinds.Contains(ProducerKind) && !ProducerRefId.IsNone() &&
        !PromptKey.IsNone() && !FormalizationKey.IsNone() && RequiredCount >= 1 && RequiredCount <= 20;
}

const FWMAdventureBeatDefinition* FWMFantasticAdventureDefinition::FindBeat(const FName BeatId) const
{
    return Beats.FindByPredicate([BeatId](const FWMAdventureBeatDefinition& Beat) { return Beat.BeatId == BeatId; });
}

bool FWMFantasticAdventureDefinition::IsSane() const
{
    if (AdventureId.IsNone() || MissionId.IsNone() || TitleKey.IsNone() || PremiseKey.IsNone() ||
        !FirstClassDisciplines.Contains(PrimaryDiscipline) || !AllowedAgeBands.Contains(AgeBand) || Beats.IsEmpty()) return false;

    TSet<FName> Secondary;
    for (const FName Discipline : SecondaryDisciplines)
    {
        if (!AllowedSecondaryDisciplines.Contains(Discipline) || Discipline == PrimaryDiscipline || Secondary.Contains(Discipline)) return false;
        Secondary.Add(Discipline);
    }

    TSet<FName> BeatIds;
    TSet<FName> EventIds;
    for (const FWMAdventureBeatDefinition& Beat : Beats)
    {
        if (!Beat.IsSane() || BeatIds.Contains(Beat.BeatId) || EventIds.Contains(Beat.EvidenceEventId)) return false;
        BeatIds.Add(Beat.BeatId);
        EventIds.Add(Beat.EvidenceEventId);
    }
    return true;
}

const FWMFantasticAdventureDefinition* FWMFantasticAdventurePack::FindAdventure(const FName AdventureId) const
{
    return Adventures.FindByPredicate([AdventureId](const FWMFantasticAdventureDefinition& Adventure)
    {
        return Adventure.AdventureId == AdventureId;
    });
}

bool FWMFantasticAdventurePack::IsSane() const
{
    if (SchemaVersion != 1 || PackId != FName(TEXT("adventure-pack.first-fantastic-v1")) || !bPrototypeOnly ||
        DesignPrinciple != FName(TEXT("learning-is-structurally-necessary-to-play")) ||
        ProgressionModel != FName(TEXT("ordered-beats")) || PrivacyModel != FName(TEXT("stable-ids-no-child-free-text")) ||
        Adventures.Num() != 11) return false;

    TSet<FName> AdventureIds;
    TSet<FName> MissionIds;
    TSet<FName> PrimaryDisciplines;
    for (const FWMFantasticAdventureDefinition& Adventure : Adventures)
    {
        if (!Adventure.IsSane() || AdventureIds.Contains(Adventure.AdventureId) || MissionIds.Contains(Adventure.MissionId) ||
            PrimaryDisciplines.Contains(Adventure.PrimaryDiscipline)) return false;
        AdventureIds.Add(Adventure.AdventureId);
        MissionIds.Add(Adventure.MissionId);
        PrimaryDisciplines.Add(Adventure.PrimaryDiscipline);
    }
    return PrimaryDisciplines.Num() == FirstClassDisciplines.Num();
}

bool FWMFantasticAdventurePack::TryParseJson(const FString& Json, FWMFantasticAdventurePack& OutPack, FString& OutError)
{
    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        OutError = TEXT("Fantastic adventure pack JSON is invalid.");
        return false;
    }

    FWMFantasticAdventurePack Candidate;
    int32 Version = 0;
    bool bPrototype = true;
    if (!Root->TryGetNumberField(TEXT("schemaVersion"), Version) || !Root->TryGetBoolField(TEXT("prototypeOnly"), bPrototype) ||
        !NameField(Root, TEXT("packId"), Candidate.PackId) || !NameField(Root, TEXT("designPrinciple"), Candidate.DesignPrinciple) ||
        !NameField(Root, TEXT("progressionModel"), Candidate.ProgressionModel) || !NameField(Root, TEXT("privacyModel"), Candidate.PrivacyModel))
    {
        OutError = TEXT("Fantastic adventure pack header is invalid.");
        return false;
    }
    Candidate.SchemaVersion = Version;
    Candidate.bPrototypeOnly = bPrototype;

    const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
    if (!Root->TryGetArrayField(TEXT("adventures"), Values) || !Values || Values->IsEmpty())
    {
        OutError = TEXT("Fantastic adventure pack has no adventures.");
        return false;
    }
    for (const TSharedPtr<FJsonValue>& Value : *Values)
    {
        const TSharedPtr<FJsonObject>* Object = nullptr;
        if (!Value.IsValid() || !Value->TryGetObject(Object) || !Object || !Object->IsValid())
        {
            OutError = TEXT("Fantastic adventure entry is invalid.");
            return false;
        }
        FWMFantasticAdventureDefinition Adventure;
        if (!ParseAdventure(*Object, Adventure))
        {
            OutError = TEXT("Fantastic adventure failed semantic validation.");
            return false;
        }
        Candidate.Adventures.Add(MoveTemp(Adventure));
    }

    if (!Candidate.IsSane())
    {
        OutError = TEXT("Fantastic adventure pack failed semantic validation.");
        return false;
    }
    OutPack = MoveTemp(Candidate);
    OutError.Reset();
    return true;
}

bool FWMAdventureProgressModel::Begin(const FWMFantasticAdventureDefinition& InDefinition)
{
    Reset();
    if (!InDefinition.IsSane()) return false;
    Definition = InDefinition;
    bActive = true;
    return true;
}

const FWMAdventureBeatDefinition* FWMAdventureProgressModel::GetCurrentBeat() const
{
    return bActive && !bCompleted && Definition.Beats.IsValidIndex(CurrentBeatIndex) ? &Definition.Beats[CurrentBeatIndex] : nullptr;
}

bool FWMAdventureProgressModel::CanAcceptEvidence(
    const FName ProducerKind, const FName ProducerRefId, const FName PrimitiveId, const FName EvidenceEventId) const
{
    const FWMAdventureBeatDefinition* Beat = GetCurrentBeat();
    return Beat && Beat->ProducerKind == ProducerKind && Beat->ProducerRefId == ProducerRefId &&
        Beat->PrimitiveId == PrimitiveId && Beat->EvidenceEventId == EvidenceEventId && CurrentBeatEvidenceCount < Beat->RequiredCount;
}

bool FWMAdventureProgressModel::CommitEvidence(
    const FName ProducerKind, const FName ProducerRefId, const FName PrimitiveId, const FName EvidenceEventId)
{
    if (!CanAcceptEvidence(ProducerKind, ProducerRefId, PrimitiveId, EvidenceEventId)) return false;
    const int32 RequiredCount = Definition.Beats[CurrentBeatIndex].RequiredCount;
    ++CurrentBeatEvidenceCount;
    if (CurrentBeatEvidenceCount >= RequiredCount)
    {
        ++CurrentBeatIndex;
        CurrentBeatEvidenceCount = 0;
        if (CurrentBeatIndex >= Definition.Beats.Num())
        {
            bCompleted = true;
            bActive = false;
        }
    }
    return true;
}

float FWMAdventureProgressModel::GetProgressFraction() const
{
    if (bCompleted) return 1.0f;
    if (!bActive || Definition.Beats.IsEmpty()) return 0.0f;
    int32 Total = 0;
    int32 Complete = 0;
    for (int32 Index = 0; Index < Definition.Beats.Num(); ++Index)
    {
        const int32 Units = Definition.Beats[Index].RequiredCount;
        Total += Units;
        if (Index < CurrentBeatIndex) Complete += Units;
        else if (Index == CurrentBeatIndex) Complete += CurrentBeatEvidenceCount;
    }
    return Total > 0 ? static_cast<float>(Complete) / static_cast<float>(Total) : 0.0f;
}

FWMAdventureProgressReadModel FWMAdventureProgressModel::BuildReadModel() const
{
    FWMAdventureProgressReadModel Result;
    Result.AdventureId = Definition.AdventureId;
    Result.CurrentBeatIndex = CurrentBeatIndex;
    Result.BeatCount = Definition.Beats.Num();
    Result.CurrentBeatEvidenceCount = CurrentBeatEvidenceCount;
    Result.ProgressFraction = GetProgressFraction();
    Result.bCompleted = bCompleted;
    if (const FWMAdventureBeatDefinition* Beat = GetCurrentBeat()) Result.CurrentBeatId = Beat->BeatId;
    return Result;
}

void FWMAdventureProgressModel::Reset()
{
    Definition = FWMFantasticAdventureDefinition();
    CurrentBeatIndex = 0;
    CurrentBeatEvidenceCount = 0;
    bActive = false;
    bCompleted = false;
}
