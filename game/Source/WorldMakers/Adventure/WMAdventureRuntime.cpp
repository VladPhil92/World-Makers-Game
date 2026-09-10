#include "Adventure/WMAdventureRuntime.h"

#include "Dom/JsonObject.h"
#include "Mission/WMMissionTypes.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
    const TSet<FName> AllowedProducerKinds = {
        TEXT("building"), TEXT("science"), TEXT("thought"), TEXT("world")
    };

    const TSet<FName> AllowedDisciplines = {
        TEXT("mathematics"), TEXT("geometry"), TEXT("english-language"), TEXT("spanish-language"),
        TEXT("literature"), TEXT("biology"), TEXT("chemistry"), TEXT("physics"), TEXT("ecology"),
        TEXT("ethics"), TEXT("philosophy-for-children")
    };

    const TSet<FName> AllowedAgeBands = { TEXT("4-6"), TEXT("7-8"), TEXT("9-10") };

    bool ReadRequiredName(const TSharedPtr<FJsonObject>& Object, const TCHAR* FieldName, FName& OutValue)
    {
        FString Value;
        if (!Object.IsValid() || !Object->TryGetStringField(FieldName, Value) || Value.IsEmpty())
        {
            return false;
        }
        OutValue = FName(*Value);
        return !OutValue.IsNone();
    }

    bool ReadNameArray(const TSharedPtr<FJsonObject>& Object, const TCHAR* FieldName, TArray<FName>& OutValues, const bool bAllowEmpty)
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
            if (!Value.IsValid() || Value->Type != EJson::String || Value->AsString().IsEmpty())
            {
                return false;
            }
            const FName Name(*Value->AsString());
            if (Name.IsNone() || Seen.Contains(Name))
            {
                return false;
            }
            Seen.Add(Name);
            OutValues.Add(Name);
        }
        return bAllowEmpty || !OutValues.IsEmpty();
    }

    bool ParseBeat(const TSharedPtr<FJsonObject>& Object, FWMAdventureBeatDefinition& OutBeat)
    {
        int32 RequiredCount = 0;
        if (!ReadRequiredName(Object, TEXT("beatId"), OutBeat.BeatId) ||
            !ReadRequiredName(Object, TEXT("primitiveId"), OutBeat.PrimitiveId) ||
            !ReadRequiredName(Object, TEXT("evidenceEventId"), OutBeat.EvidenceEventId) ||
            !ReadRequiredName(Object, TEXT("producerKind"), OutBeat.ProducerKind) ||
            !ReadRequiredName(Object, TEXT("producerRefId"), OutBeat.ProducerRefId) ||
            !ReadRequiredName(Object, TEXT("promptKey"), OutBeat.PromptKey) ||
            !ReadRequiredName(Object, TEXT("formalizationKey"), OutBeat.FormalizationKey) ||
            !Object->TryGetNumberField(TEXT("requiredCount"), RequiredCount))
        {
            return false;
        }
        OutBeat.RequiredCount = RequiredCount;
        return OutBeat.IsSane();
    }

    bool ParseAdventure(const TSharedPtr<FJsonObject>& Object, FWMFantasticAdventureDefinition& OutAdventure)
    {
        if (!ReadRequiredName(Object, TEXT("adventureId"), OutAdventure.AdventureId) ||
            !ReadRequiredName(Object, TEXT("missionId"), OutAdventure.MissionId) ||
            !ReadRequiredName(Object, TEXT("titleKey"), OutAdventure.TitleKey) ||
            !ReadRequiredName(Object, TEXT("premiseKey"), OutAdventure.PremiseKey) ||
            !ReadRequiredName(Object, TEXT("primaryDiscipline"), OutAdventure.PrimaryDiscipline) ||
            !ReadRequiredName(Object, TEXT("ageBand"), OutAdventure.AgeBand) ||
            !ReadNameArray(Object, TEXT("secondaryDisciplines"), OutAdventure.SecondaryDisciplines, true))
        {
            return false;
        }

        const TArray<TSharedPtr<FJsonValue>>* Beats = nullptr;
        if (!Object->TryGetArrayField(TEXT("beats"), Beats) || !Beats || Beats->IsEmpty())
        {
            return false;
        }

        for (const TSharedPtr<FJsonValue>& Value : *Beats)
        {
            const TSharedPtr<FJsonObject>* BeatObject = nullptr;
            if (!Value.IsValid() || !Value->TryGetObject(BeatObject) || !BeatObject || !BeatObject->IsValid())
            {
                return false;
            }
            FWMAdventureBeatDefinition Beat;
            if (!ParseBeat(*BeatObject, Beat))
            {
                return false;
            }
            OutAdventure.Beats.Add(MoveTemp(Beat));
        }
        return OutAdventure.IsSane();
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
        !AllowedDisciplines.Contains(PrimaryDiscipline) || !AllowedAgeBands.Contains(AgeBand) || Beats.IsEmpty())
    {
        return false;
    }

    TSet<FName> SecondarySet;
    for (const FName Discipline : SecondaryDisciplines)
    {
        if (!AllowedDisciplines.Contains(Discipline) || Discipline == PrimaryDiscipline || SecondarySet.Contains(Discipline))
        {
            return false;
        }
        SecondarySet.Add(Discipline);
    }

    TSet<FName> BeatIds;
    TSet<FName> EvidenceEventIds;
    for (const FWMAdventureBeatDefinition& Beat : Beats)
    {
        if (!Beat.IsSane() || BeatIds.Contains(Beat.BeatId) || EvidenceEventIds.Contains(Beat.EvidenceEventId))
        {
            return false;
        }
        BeatIds.Add(Beat.BeatId);
        EvidenceEventIds.Add(Beat.EvidenceEventId);
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
        ProgressionModel != FName(TEXT("ordered-beats")) ||
        PrivacyModel != FName(TEXT("stable-ids-no-child-free-text")) || Adventures.Num() != 11)
    {
        return false;
    }

    TSet<FName> AdventureIds;
    TSet<FName> MissionIds;
    TSet<FName> PrimaryDisciplines;
    for (const FWMFantasticAdventureDefinition& Adventure : Adventures)
    {
        if (!Adventure.IsSane() || AdventureIds.Contains(Adventure.AdventureId) || MissionIds.Contains(Adventure.MissionId) ||
            PrimaryDisciplines.Contains(Adventure.PrimaryDiscipline))
        {
            return false;
        }
        AdventureIds.Add(Adventure.AdventureId);
        MissionIds.Add(Adventure.MissionId);
        PrimaryDisciplines.Add(Adventure.PrimaryDiscipline);
    }
    return PrimaryDisciplines.Num() == AllowedDisciplines.Num();
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

    int32 SchemaVersion = 0;
    bool bPrototypeOnly = true;
    FWMFantasticAdventurePack Candidate;
    if (!Root->TryGetNumberField(TEXT("schemaVersion"), SchemaVersion) ||
        !Root->TryGetBoolField(TEXT("prototypeOnly"), bPrototypeOnly) ||
        !ReadRequiredName(Root, TEXT("packId"), Candidate.PackId) ||
        !ReadRequiredName(Root, TEXT("designPrinciple"), Candidate.DesignPrinciple) ||
        !ReadRequiredName(Root, TEXT("progressionModel"), Candidate.ProgressionModel) ||
        !ReadRequiredName(Root, TEXT("privacyModel"), Candidate.PrivacyModel))
    {
        OutError = TEXT("Fantastic adventure pack header is invalid.");
        return false;
    }
    Candidate.SchemaVersion = SchemaVersion;
    Candidate.bPrototypeOnly = bPrototypeOnly;

    const TArray<TSharedPtr<FJsonValue>>* Adventures = nullptr;
    if (!Root->TryGetArrayField(TEXT("adventures"), Adventures) || !Adventures || Adventures->IsEmpty())
    {
        OutError = TEXT("Fantastic adventure pack has no adventures.");
        return false;
    }

    for (const TSharedPtr<FJsonValue>& Value : *Adventures)
    {
        const TSharedPtr<FJsonObject>* AdventureObject = nullptr;
        if (!Value.IsValid() || !Value->TryGetObject(AdventureObject) || !AdventureObject || !AdventureObject->IsValid())
        {
            OutError = TEXT("Fantastic adventure entry is invalid.");
            return false;
        }
        FWMFantasticAdventureDefinition Adventure;
        if (!ParseAdventure(*AdventureObject, Adventure))
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
    if (!InDefinition.IsSane())
    {
        return false;
    }
    Definition = InDefinition;
    bActive = true;
    return true;
}

const FWMAdventureBeatDefinition* FWMAdventureProgressModel::GetCurrentBeat() const
{
    if (!bActive || bCompleted || !Definition.Beats.IsValidIndex(CurrentBeatIndex))
    {
        return nullptr;
    }
    return &Definition.Beats[CurrentBeatIndex];
}

bool FWMAdventureProgressModel::CanAcceptEvidence(
    const FName ProducerKind,
    const FName ProducerRefId,
    const FName PrimitiveId,
    const FName EvidenceEventId) const
{
    const FWMAdventureBeatDefinition* Beat = GetCurrentBeat();
    return Beat && Beat->ProducerKind == ProducerKind && Beat->ProducerRefId == ProducerRefId &&
        Beat->PrimitiveId == PrimitiveId && Beat->EvidenceEventId == EvidenceEventId &&
        CurrentBeatEvidenceCount < Beat->RequiredCount;
}

bool FWMAdventureProgressModel::CommitEvidence(
    const FName ProducerKind,
    const FName ProducerRefId,
    const FName PrimitiveId,
    const FName EvidenceEventId)
{
    if (!CanAcceptEvidence(ProducerKind, ProducerRefId, PrimitiveId, EvidenceEventId))
    {
        return false;
    }

    const FWMAdventureBeatDefinition* Beat = GetCurrentBeat();
    ++CurrentBeatEvidenceCount;
    if (Beat && CurrentBeatEvidenceCount >= Beat->RequiredCount)
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

    int32 TotalUnits = 0;
    int32 CompletedUnits = 0;
    for (int32 Index = 0; Index < Definition.Beats.Num(); ++Index)
    {
        const int32 Units = Definition.Beats[Index].RequiredCount;
        TotalUnits += Units;
        if (Index < CurrentBeatIndex)
        {
            CompletedUnits += Units;
        }
        else if (Index == CurrentBeatIndex)
        {
            CompletedUnits += CurrentBeatEvidenceCount;
        }
    }
    return TotalUnits > 0 ? static_cast<float>(CompletedUnits) / static_cast<float>(TotalUnits) : 0.0f;
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
    if (const FWMAdventureBeatDefinition* Beat = GetCurrentBeat())
    {
        Result.CurrentBeatId = Beat->BeatId;
    }
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
