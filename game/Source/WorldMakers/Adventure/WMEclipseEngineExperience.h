#pragma once

#include "CoreMinimal.h"
#include "WMEclipseEngineExperience.generated.h"

USTRUCT(BlueprintType)
struct WORLDMAKERS_API FWMEclipseEvidenceRoute
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Eclipse") FName ObjectiveId;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Eclipse") FName DisciplineId;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Eclipse") FName ProducerKind;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Eclipse") FName ProducerRefId;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Eclipse") FName PrimitiveId;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Eclipse") FName EvidenceEventId;

    bool IsSane() const;
};

USTRUCT(BlueprintType)
struct WORLDMAKERS_API FWMEclipseWorldStateRoute
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Eclipse") FName ProducerKind;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Eclipse") FName ProducerRefId;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Eclipse") FName WorldStateId;

    bool IsSane() const;
};

USTRUCT(BlueprintType)
struct WORLDMAKERS_API FWMEclipseActionDefinition
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Eclipse") FName ActionId;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Eclipse") FName InteractionVerb;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Eclipse") FName PromptKey;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Eclipse") FName FeedbackKey;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Eclipse") FName FirstPersonEventId;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Eclipse") FName FirstPersonActionId;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Eclipse") FVector PrototypeLocationCm = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Eclipse") TArray<FName> HintKeys;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Eclipse") bool bHasEvidenceRoute = false;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Eclipse") bool bHasWorldStateRoute = false;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Eclipse") FWMEclipseEvidenceRoute Evidence;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Eclipse") FWMEclipseWorldStateRoute WorldState;

    bool IsSane() const;
};

USTRUCT(BlueprintType)
struct WORLDMAKERS_API FWMEclipseChapterExperience
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Eclipse") FName ChapterId;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Eclipse") FName FantasyGoalKey;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Eclipse") FName TensionKey;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Eclipse") FName CompletionReactionKey;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Eclipse") TArray<FWMEclipseActionDefinition> Actions;

    bool IsSane() const;
    const FWMEclipseActionDefinition* FindAction(FName ActionId) const;
};

USTRUCT(BlueprintType)
struct WORLDMAKERS_API FWMEclipseExperienceCatalog
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Eclipse") int32 SchemaVersion = 1;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Eclipse") FName ExperienceId;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Eclipse") FName EpicId;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Eclipse") bool bPrototypeOnly = true;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Eclipse") FName PlayerPromise;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Eclipse") FName PresentationRule;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Eclipse") FName RewardModel;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Eclipse") FName FailureModel;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Eclipse") FName HintModel;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Eclipse") FName PrivacyModel;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Eclipse") TArray<FName> ForbiddenPlayerFacingTerms;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Eclipse") TArray<FWMEclipseChapterExperience> Chapters;

    bool IsSane() const;
    const FWMEclipseChapterExperience* FindChapter(FName ChapterId) const;
    static bool TryParseJson(const FString& Json, FWMEclipseExperienceCatalog& OutCatalog, FString& OutError);
};

/** Session-only hint state. Nothing here is persisted as child profiling or behavior telemetry. */
struct WORLDMAKERS_API FWMEclipseHintRuntime
{
    FName RequestHint(const FWMEclipseActionDefinition& Action);
    void RecordAttempt(FName ActionId);
    int32 GetAttemptCount(FName ActionId) const;
    void Reset();

private:
    TMap<FName, int32> Attempts;
    TMap<FName, int32> HintLevels;
};
