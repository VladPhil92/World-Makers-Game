#include "Adventure/WMEpicRuntime.h"

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
    const TSet<FName> AllowedDisciplines = {
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

    bool ParseEvidence(const TSharedPtr<FJsonObject>& Object, FWMEpicEvidenceRequirement& Out)
    {
        int32 Count = 0;
        if (!NameField(Object, TEXT("requirementId"), Out.RequirementId) ||
            !NameField(Object, TEXT("objectiveId"), Out.ObjectiveId) ||
            !NameField(Object, TEXT("disciplineId"), Out.DisciplineId) ||
            !NameField(Object, TEXT("primitiveId"), Out.PrimitiveId) ||
            !NameField(Object, TEXT("evidenceEventId"), Out.EvidenceEventId) ||
            !NameField(Object, TEXT("producerKind"), Out.ProducerKind) ||
            !NameField(Object, TEXT("producerRefId"), Out.ProducerRefId) ||
            !Object->TryGetNumberField(TEXT("requiredCount"), Count)) return false;
        Out.RequiredCount = Count;
        return Out.IsSane();
    }

    bool ParseWorldState(const TSharedPtr<FJsonObject>& Object, FWMEpicWorldStateRequirement& Out)
    {
        return NameField(Object, TEXT("worldStateId"), Out.WorldStateId) &&
            NameField(Object, TEXT("producerKind"), Out.ProducerKind) &&
            NameField(Object, TEXT("producerRefId"), Out.ProducerRefId) && Out.IsSane();
    }

    bool ParseChapter(const TSharedPtr<FJsonObject>& Object, FWMEpicChapterDefinition& Out)
    {
        if (!NameField(Object, TEXT("chapterId"), Out.ChapterId) ||
            !NameField(Object, TEXT("missionId"), Out.MissionId) ||
            !NameField(Object, TEXT("titleKey"), Out.TitleKey) ||
            !NameField(Object, TEXT("promptKey"), Out.PromptKey)) return false;

        const TArray<TSharedPtr<FJsonValue>>* Evidence = nullptr;
        if (!Object->TryGetArrayField(TEXT("evidenceRequirements"), Evidence) || !Evidence || Evidence->IsEmpty()) return false;
        for (const TSharedPtr<FJsonValue>& Value : *Evidence)
        {
            const TSharedPtr<FJsonObject>* Item = nullptr;
            if (!Value.IsValid() || !Value->TryGetObject(Item) || !Item || !Item->IsValid()) return false;
            FWMEpicEvidenceRequirement Requirement;
            if (!ParseEvidence(*Item, Requirement)) return false;
            Out.EvidenceRequirements.Add(MoveTemp(Requirement));
        }

        const TArray<TSharedPtr<FJsonValue>>* WorldStates = nullptr;
        if (!Object->TryGetArrayField(TEXT("worldStateRequirements"), WorldStates) || !WorldStates || WorldStates->IsEmpty()) return false;
        for (const TSharedPtr<FJsonValue>& Value : *WorldStates)
        {
            const TSharedPtr<FJsonObject>* Item = nullptr;
            if (!Value.IsValid() || !Value->TryGetObject(Item) || !Item || !Item->IsValid()) return false;
            FWMEpicWorldStateRequirement Requirement;
            if (!ParseWorldState(*Item, Requirement)) return false;
            Out.WorldStateRequirements.Add(MoveTemp(Requirement));
        }
        return Out.IsSane();
    }

    bool ParseEpic(const TSharedPtr<FJsonObject>& Object, FWMEpicDefinition& Out)
    {
        if (!NameField(Object, TEXT("epicId"), Out.EpicId) || !NameField(Object, TEXT("titleKey"), Out.TitleKey) ||
            !NameField(Object, TEXT("premiseKey"), Out.PremiseKey) || !NameField(Object, TEXT("ageBand"), Out.AgeBand) ||
            !NameArray(Object, TEXT("disciplines"), Out.Disciplines, false)) return false;

        const TArray<TSharedPtr<FJsonValue>>* Chapters = nullptr;
        if (!Object->TryGetArrayField(TEXT("chapters"), Chapters) || !Chapters || Chapters->IsEmpty()) return false;
        for (const TSharedPtr<FJsonValue>& Value : *Chapters)
        {
            const TSharedPtr<FJsonObject>* Item = nullptr;
            if (!Value.IsValid() || !Value->TryGetObject(Item) || !Item || !Item->IsValid()) return false;
            FWMEpicChapterDefinition Chapter;
            if (!ParseChapter(*Item, Chapter)) return false;
            Out.Chapters.Add(MoveTemp(Chapter));
        }
        return Out.IsSane();
    }
}

bool FWMEpicEvidenceRequirement::IsSane() const
{
    return !RequirementId.IsNone() && !ObjectiveId.IsNone() && FirstClassDisciplines.Contains(DisciplineId) &&
        FWMMissionRuntimeDefinition::IsSupportedEvidencePrimitive(PrimitiveId) && !EvidenceEventId.IsNone() &&
        AllowedProducerKinds.Contains(ProducerKind) && !ProducerRefId.IsNone() && RequiredCount >= 1 && RequiredCount <= 20;
}

bool FWMEpicWorldStateRequirement::IsSane() const
{
    return !WorldStateId.IsNone() && AllowedProducerKinds.Contains(ProducerKind) && !ProducerRefId.IsNone();
}

bool FWMEpicChapterDefinition::IsSane() const
{
    if (ChapterId.IsNone() || MissionId.IsNone() || TitleKey.IsNone() || PromptKey.IsNone() ||
        EvidenceRequirements.IsEmpty() || WorldStateRequirements.IsEmpty()) return false;

    TSet<FName> RequirementIds;
    TSet<FName> WorldStateIds;
    for (const FWMEpicEvidenceRequirement& Requirement : EvidenceRequirements)
    {
        if (!Requirement.IsSane() || RequirementIds.Contains(Requirement.RequirementId)) return false;
        RequirementIds.Add(Requirement.RequirementId);
    }
    for (const FWMEpicWorldStateRequirement& Requirement : WorldStateRequirements)
    {
        if (!Requirement.IsSane() || WorldStateIds.Contains(Requirement.WorldStateId)) return false;
        WorldStateIds.Add(Requirement.WorldStateId);
    }
    return true;
}

const FWMEpicChapterDefinition* FWMEpicDefinition::FindChapter(const FName ChapterId) const
{
    return Chapters.FindByPredicate([ChapterId](const FWMEpicChapterDefinition& Chapter) { return Chapter.ChapterId == ChapterId; });
}

bool FWMEpicDefinition::IsSane() const
{
    if (EpicId.IsNone() || TitleKey.IsNone() || PremiseKey.IsNone() || !AllowedAgeBands.Contains(AgeBand) ||
        Disciplines.Num() < 4 || Chapters.Num() < 2) return false;

    TSet<FName> DisciplineSet;
    int32 FirstClassCount = 0;
    for (const FName Discipline : Disciplines)
    {
        if (!AllowedDisciplines.Contains(Discipline) || DisciplineSet.Contains(Discipline)) return false;
        DisciplineSet.Add(Discipline);
        if (FirstClassDisciplines.Contains(Discipline)) ++FirstClassCount;
    }
    if (FirstClassCount < 4) return false;

    TSet<FName> ChapterIds;
    TSet<FName> MissionIds;
    for (const FWMEpicChapterDefinition& Chapter : Chapters)
    {
        if (!Chapter.IsSane() || ChapterIds.Contains(Chapter.ChapterId) || MissionIds.Contains(Chapter.MissionId)) return false;
        ChapterIds.Add(Chapter.ChapterId);
        MissionIds.Add(Chapter.MissionId);
        for (const FWMEpicEvidenceRequirement& Requirement : Chapter.EvidenceRequirements)
        {
            if (!DisciplineSet.Contains(Requirement.DisciplineId)) return false;
        }
    }
    return true;
}

const FWMEpicDefinition* FWMEpicCatalog::FindEpic(const FName EpicId) const
{
    return Epics.FindByPredicate([EpicId](const FWMEpicDefinition& Epic) { return Epic.EpicId == EpicId; });
}

bool FWMEpicCatalog::IsSane() const
{
    if (SchemaVersion != 1 || CatalogId != FName(TEXT("epic-catalog.cross-disciplinary-v1")) || !bPrototypeOnly ||
        DesignPrinciple != FName(TEXT("learning-is-structurally-necessary-to-play")) ||
        ProgressionModel != FName(TEXT("ordered-chapters-world-state-gated")) ||
        PrivacyModel != FName(TEXT("stable-ids-no-child-free-text")) || Epics.Num() < 2) return false;

    TSet<FName> EpicIds;
    for (const FWMEpicDefinition& Epic : Epics)
    {
        if (!Epic.IsSane() || EpicIds.Contains(Epic.EpicId)) return false;
        EpicIds.Add(Epic.EpicId);
    }
    return true;
}

bool FWMEpicCatalog::TryParseJson(const FString& Json, FWMEpicCatalog& OutCatalog, FString& OutError)
{
    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        OutError = TEXT("Epic catalog JSON is invalid.");
        return false;
    }

    FWMEpicCatalog Candidate;
    int32 Version = 0;
    bool bPrototype = true;
    if (!Root->TryGetNumberField(TEXT("schemaVersion"), Version) || !Root->TryGetBoolField(TEXT("prototypeOnly"), bPrototype) ||
        !NameField(Root, TEXT("catalogId"), Candidate.CatalogId) || !NameField(Root, TEXT("designPrinciple"), Candidate.DesignPrinciple) ||
        !NameField(Root, TEXT("progressionModel"), Candidate.ProgressionModel) || !NameField(Root, TEXT("privacyModel"), Candidate.PrivacyModel))
    {
        OutError = TEXT("Epic catalog header is invalid.");
        return false;
    }
    Candidate.SchemaVersion = Version;
    Candidate.bPrototypeOnly = bPrototype;

    const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
    if (!Root->TryGetArrayField(TEXT("epics"), Values) || !Values || Values->IsEmpty())
    {
        OutError = TEXT("Epic catalog has no epics.");
        return false;
    }
    for (const TSharedPtr<FJsonValue>& Value : *Values)
    {
        const TSharedPtr<FJsonObject>* Item = nullptr;
        if (!Value.IsValid() || !Value->TryGetObject(Item) || !Item || !Item->IsValid())
        {
            OutError = TEXT("Epic catalog entry is invalid.");
            return false;
        }
        FWMEpicDefinition Epic;
        if (!ParseEpic(*Item, Epic))
        {
            OutError = TEXT("Epic definition failed semantic validation.");
            return false;
        }
        Candidate.Epics.Add(MoveTemp(Epic));
    }

    if (!Candidate.IsSane())
    {
        OutError = TEXT("Epic catalog failed semantic validation.");
        return false;
    }
    OutCatalog = MoveTemp(Candidate);
    OutError.Reset();
    return true;
}

bool FWMEpicProgressModel::Begin(const FWMEpicDefinition& InDefinition)
{
    Reset();
    if (!InDefinition.IsSane()) return false;
    Definition = InDefinition;
    bActive = true;
    return true;
}

const FWMEpicChapterDefinition* FWMEpicProgressModel::GetCurrentChapter() const
{
    return bActive && !bCompleted && Definition.Chapters.IsValidIndex(CurrentChapterIndex) ? &Definition.Chapters[CurrentChapterIndex] : nullptr;
}

const FWMEpicChapterDefinition* FWMEpicProgressModel::GetNextChapter() const
{
    const int32 NextIndex = CurrentChapterIndex + 1;
    return bActive && !bCompleted && Definition.Chapters.IsValidIndex(NextIndex) ? &Definition.Chapters[NextIndex] : nullptr;
}

const FWMEpicEvidenceRequirement* FWMEpicProgressModel::FindMatchingEvidence(
    const FName ObjectiveId, const FName DisciplineId, const FName ProducerKind, const FName ProducerRefId,
    const FName PrimitiveId, const FName EvidenceEventId) const
{
    const FWMEpicChapterDefinition* Chapter = GetCurrentChapter();
    if (!Chapter) return nullptr;
    return Chapter->EvidenceRequirements.FindByPredicate([&](const FWMEpicEvidenceRequirement& Requirement)
    {
        return Requirement.ObjectiveId == ObjectiveId && Requirement.DisciplineId == DisciplineId &&
            Requirement.ProducerKind == ProducerKind && Requirement.ProducerRefId == ProducerRefId &&
            Requirement.PrimitiveId == PrimitiveId && Requirement.EvidenceEventId == EvidenceEventId;
    });
}

bool FWMEpicProgressModel::CanAcceptEvidence(
    const FName ObjectiveId, const FName DisciplineId, const FName ProducerKind, const FName ProducerRefId,
    const FName PrimitiveId, const FName EvidenceEventId) const
{
    const FWMEpicEvidenceRequirement* Requirement = FindMatchingEvidence(
        ObjectiveId, DisciplineId, ProducerKind, ProducerRefId, PrimitiveId, EvidenceEventId);
    return Requirement && CurrentEvidenceCounts.FindRef(Requirement->RequirementId) < Requirement->RequiredCount;
}

bool FWMEpicProgressModel::CommitEvidence(
    const FName ObjectiveId, const FName DisciplineId, const FName ProducerKind, const FName ProducerRefId,
    const FName PrimitiveId, const FName EvidenceEventId)
{
    const FWMEpicEvidenceRequirement* Requirement = FindMatchingEvidence(
        ObjectiveId, DisciplineId, ProducerKind, ProducerRefId, PrimitiveId, EvidenceEventId);
    if (!Requirement || CurrentEvidenceCounts.FindRef(Requirement->RequirementId) >= Requirement->RequiredCount) return false;
    CurrentEvidenceCounts.FindOrAdd(Requirement->RequirementId) += 1;
    return true;
}

bool FWMEpicProgressModel::CanAcceptWorldState(const FName ProducerKind, const FName ProducerRefId, const FName WorldStateId) const
{
    const FWMEpicChapterDefinition* Chapter = GetCurrentChapter();
    if (!Chapter || CurrentWorldStates.Contains(WorldStateId)) return false;
    return Chapter->WorldStateRequirements.ContainsByPredicate([&](const FWMEpicWorldStateRequirement& Requirement)
    {
        return Requirement.WorldStateId == WorldStateId && Requirement.ProducerKind == ProducerKind && Requirement.ProducerRefId == ProducerRefId;
    });
}

bool FWMEpicProgressModel::CommitWorldState(const FName ProducerKind, const FName ProducerRefId, const FName WorldStateId)
{
    if (!CanAcceptWorldState(ProducerKind, ProducerRefId, WorldStateId)) return false;
    CurrentWorldStates.Add(WorldStateId);
    return true;
}

bool FWMEpicProgressModel::IsCurrentChapterReadyToAdvance() const
{
    const FWMEpicChapterDefinition* Chapter = GetCurrentChapter();
    if (!Chapter) return false;
    for (const FWMEpicEvidenceRequirement& Requirement : Chapter->EvidenceRequirements)
    {
        if (CurrentEvidenceCounts.FindRef(Requirement.RequirementId) < Requirement.RequiredCount) return false;
    }
    for (const FWMEpicWorldStateRequirement& Requirement : Chapter->WorldStateRequirements)
    {
        if (!CurrentWorldStates.Contains(Requirement.WorldStateId)) return false;
    }
    return true;
}

bool FWMEpicProgressModel::AdvanceChapter()
{
    if (!IsCurrentChapterReadyToAdvance()) return false;
    ++CurrentChapterIndex;
    CurrentEvidenceCounts.Reset();
    CurrentWorldStates.Reset();
    if (CurrentChapterIndex >= Definition.Chapters.Num())
    {
        bCompleted = true;
        bActive = false;
    }
    return true;
}

int32 FWMEpicProgressModel::GetCurrentEvidenceUnits() const
{
    int32 Units = 0;
    for (const TPair<FName, int32>& Pair : CurrentEvidenceCounts) Units += Pair.Value;
    return Units;
}

float FWMEpicProgressModel::GetProgressFraction() const
{
    if (bCompleted) return 1.0f;
    if (!bActive || Definition.Chapters.IsEmpty()) return 0.0f;

    int32 TotalUnits = 0;
    int32 CompleteUnits = 0;
    for (int32 Index = 0; Index < Definition.Chapters.Num(); ++Index)
    {
        const FWMEpicChapterDefinition& Chapter = Definition.Chapters[Index];
        int32 ChapterUnits = Chapter.WorldStateRequirements.Num();
        for (const FWMEpicEvidenceRequirement& Requirement : Chapter.EvidenceRequirements) ChapterUnits += Requirement.RequiredCount;
        TotalUnits += ChapterUnits;
        if (Index < CurrentChapterIndex) CompleteUnits += ChapterUnits;
        else if (Index == CurrentChapterIndex) CompleteUnits += GetCurrentEvidenceUnits() + CurrentWorldStates.Num();
    }
    return TotalUnits > 0 ? static_cast<float>(CompleteUnits) / static_cast<float>(TotalUnits) : 0.0f;
}

FWMEpicProgressReadModel FWMEpicProgressModel::BuildReadModel() const
{
    FWMEpicProgressReadModel Result;
    Result.EpicId = Definition.EpicId;
    Result.CurrentChapterIndex = CurrentChapterIndex;
    Result.ChapterCount = Definition.Chapters.Num();
    Result.CurrentEvidenceUnits = GetCurrentEvidenceUnits();
    Result.CurrentWorldStateCount = CurrentWorldStates.Num();
    Result.ProgressFraction = GetProgressFraction();
    Result.bCurrentChapterReady = IsCurrentChapterReadyToAdvance();
    Result.bCompleted = bCompleted;
    if (const FWMEpicChapterDefinition* Chapter = GetCurrentChapter()) Result.CurrentChapterId = Chapter->ChapterId;
    return Result;
}

void FWMEpicProgressModel::Reset()
{
    Definition = FWMEpicDefinition();
    CurrentChapterIndex = 0;
    CurrentEvidenceCounts.Reset();
    CurrentWorldStates.Reset();
    bActive = false;
    bCompleted = false;
}
