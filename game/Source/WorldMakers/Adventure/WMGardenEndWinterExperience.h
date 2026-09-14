#pragma once

#include "Adventure/WMEclipseEngineExperience.h"
#include "CoreMinimal.h"
#include "WMGardenEndWinterExperience.generated.h"

USTRUCT(BlueprintType)
struct WORLDMAKERS_API FWMGardenMasteryGateRoute
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Garden") FName GateId;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Garden") FName DisciplineId;
    bool IsSane() const;
};

USTRUCT(BlueprintType)
struct WORLDMAKERS_API FWMGardenActionDefinition
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Garden") FName ActionId;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Garden") FName InteractionVerb;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Garden") FName PromptKey;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Garden") FName FeedbackKey;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Garden") FName FirstPersonEventId;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Garden") FName FirstPersonActionId;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Garden") FVector PrototypeLocationCm = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Garden") TArray<FName> HintKeys;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Garden") bool bHasEvidenceRoute = false;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Garden") bool bHasWorldStateRoute = false;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Garden") bool bHasMasteryGate = false;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Garden") FWMEclipseEvidenceRoute Evidence;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Garden") FWMEclipseWorldStateRoute WorldState;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Garden") FWMGardenMasteryGateRoute MasteryGate;
    bool IsSane() const;
};

USTRUCT(BlueprintType)
struct WORLDMAKERS_API FWMGardenChapterExperience
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Garden") FName ChapterId;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Garden") FName FantasyGoalKey;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Garden") FName TensionKey;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Garden") FName CompletionReactionKey;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Garden") TArray<FWMGardenActionDefinition> Actions;
    bool IsSane() const;
    const FWMGardenActionDefinition* FindAction(FName ActionId) const;
    TArray<FName> GetRequiredMasteryGateIds() const;
};

USTRUCT(BlueprintType)
struct WORLDMAKERS_API FWMGardenExperienceCatalog
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Garden") int32 SchemaVersion = 1;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Garden") FName ExperienceId;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Garden") FName EpicId;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Garden") bool bPrototypeOnly = true;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Garden") FName PlayerPromise;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Garden") FName PresentationRule;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Garden") FName RewardModel;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Garden") FName FailureModel;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Garden") FName HintModel;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Garden") FName PrivacyModel;
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Garden") TArray<FWMGardenChapterExperience> Chapters;
    bool IsSane() const;
    const FWMGardenChapterExperience* FindChapter(FName ChapterId) const;
    static bool TryParseJson(const FString& Json, FWMGardenExperienceCatalog& OutCatalog, FString& OutError);
};

struct WORLDMAKERS_API FWMGardenHintRuntime
{
    FName RequestHint(const FWMGardenActionDefinition& Action);
    void RecordAttempt(FName ActionId);
    void Reset();
private:
    TMap<FName,int32> Attempts;
    TMap<FName,int32> HintLevels;
};
