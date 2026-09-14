#pragma once

#include "CoreMinimal.h"
#include "WMEpicRuntime.generated.h"

USTRUCT(BlueprintType)
struct WORLDMAKERS_API FWMEpicEvidenceRequirement
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Epic") FName RequirementId;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Epic") FName ObjectiveId;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Epic") FName DisciplineId;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Epic") FName PrimitiveId;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Epic") FName EvidenceEventId;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Epic") FName ProducerKind;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Epic") FName ProducerRefId;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Epic") int32 RequiredCount = 1;

    bool IsSane() const;
};

USTRUCT(BlueprintType)
struct WORLDMAKERS_API FWMEpicWorldStateRequirement
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Epic") FName WorldStateId;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Epic") FName ProducerKind;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Epic") FName ProducerRefId;

    bool IsSane() const;
};

USTRUCT(BlueprintType)
struct WORLDMAKERS_API FWMEpicChapterDefinition
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Epic") FName ChapterId;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Epic") FName MissionId;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Epic") FName TitleKey;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Epic") FName PromptKey;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Epic") TArray<FWMEpicEvidenceRequirement> EvidenceRequirements;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Epic") TArray<FWMEpicWorldStateRequirement> WorldStateRequirements;

    bool IsSane() const;
};

USTRUCT(BlueprintType)
struct WORLDMAKERS_API FWMEpicDefinition
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Epic") FName EpicId;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Epic") FName TitleKey;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Epic") FName PremiseKey;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Epic") FName AgeBand;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Epic") TArray<FName> Disciplines;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Epic") TArray<FWMEpicChapterDefinition> Chapters;

    bool IsSane() const;
    const FWMEpicChapterDefinition* FindChapter(FName ChapterId) const;
};

USTRUCT(BlueprintType)
struct WORLDMAKERS_API FWMEpicCatalog
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Epic") int32 SchemaVersion = 1;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Epic") FName CatalogId;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Epic") bool bPrototypeOnly = true;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Epic") FName DesignPrinciple;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Epic") FName ProgressionModel;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Epic") FName PrivacyModel;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Epic") TArray<FWMEpicDefinition> Epics;

    bool IsSane() const;
    const FWMEpicDefinition* FindEpic(FName EpicId) const;
    static bool TryParseJson(const FString& Json, FWMEpicCatalog& OutCatalog, FString& OutError);
};

USTRUCT(BlueprintType)
struct WORLDMAKERS_API FWMEpicProgressReadModel
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Epic") FName EpicId;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Epic") FName CurrentChapterId;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Epic") int32 CurrentChapterIndex = 0;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Epic") int32 ChapterCount = 0;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Epic") int32 CurrentEvidenceUnits = 0;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Epic") int32 CurrentWorldStateCount = 0;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Epic") float ProgressFraction = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Epic") bool bCurrentChapterReady = false;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Epic") bool bCompleted = false;
};

/**
 * Pure authoritative epic progress model. It stores only stable IDs and bounded counters.
 * A chapter can advance only when every pedagogical evidence requirement and every trusted
 * world-state predicate for that chapter has been satisfied.
 */
struct WORLDMAKERS_API FWMEpicProgressModel
{
    bool Begin(const FWMEpicDefinition& InDefinition);

    /**
     * M5.6D chapter checkpoint restore. Partial evidence/world-state is intentionally dropped:
     * a resumed journey restarts the saved chapter from a clean authoritative state.
     */
    bool ResumeAtChapter(const FWMEpicDefinition& InDefinition, const int32 InChapterIndex)
    {
        Reset();
        if (!InDefinition.IsSane() || !InDefinition.Chapters.IsValidIndex(InChapterIndex)) return false;
        Definition = InDefinition;
        CurrentChapterIndex = InChapterIndex;
        bActive = true;
        return true;
    }

    bool CanAcceptEvidence(FName ObjectiveId, FName DisciplineId, FName ProducerKind, FName ProducerRefId, FName PrimitiveId, FName EvidenceEventId) const;
    bool CommitEvidence(FName ObjectiveId, FName DisciplineId, FName ProducerKind, FName ProducerRefId, FName PrimitiveId, FName EvidenceEventId);
    bool CanAcceptWorldState(FName ProducerKind, FName ProducerRefId, FName WorldStateId) const;
    bool CommitWorldState(FName ProducerKind, FName ProducerRefId, FName WorldStateId);
    bool IsCurrentChapterReadyToAdvance() const;
    bool AdvanceChapter();
    const FWMEpicChapterDefinition* GetCurrentChapter() const;
    const FWMEpicChapterDefinition* GetNextChapter() const;
    float GetProgressFraction() const;
    bool IsActive() const { return bActive; }
    bool IsCompleted() const { return bCompleted; }
    FWMEpicProgressReadModel BuildReadModel() const;
    void Reset();

private:
    const FWMEpicEvidenceRequirement* FindMatchingEvidence(FName ObjectiveId, FName DisciplineId, FName ProducerKind, FName ProducerRefId, FName PrimitiveId, FName EvidenceEventId) const;
    int32 GetCurrentEvidenceUnits() const;

    FWMEpicDefinition Definition;
    int32 CurrentChapterIndex = 0;
    TMap<FName, int32> CurrentEvidenceCounts;
    TSet<FName> CurrentWorldStates;
    bool bActive = false;
    bool bCompleted = false;
};
