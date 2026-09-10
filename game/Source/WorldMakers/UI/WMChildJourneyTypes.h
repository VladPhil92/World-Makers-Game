#pragma once

#include "CoreMinimal.h"
#include "Mission/WMMissionTypes.h"
#include "WMChildJourneyTypes.generated.h"

UENUM(BlueprintType)
enum class EWMChildAdventureKind : uint8
{
    Mission,
    Discovery,
    EcosystemCare,
    CreativeUnlock
};

UENUM(BlueprintType)
enum class EWMChildAdventureState : uint8
{
    Hidden,
    Ready,
    InProgress,
    Complete
};

USTRUCT(BlueprintType)
struct WORLDMAKERS_API FWMChildAdventureCard
{
    GENERATED_BODY()

    /** Stable UI projection ID. Never shown directly to the child. */
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Child Journey")
    FName CardId;

    /** Stable source-domain ID used for traceability. Never used as display copy. */
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Child Journey")
    FName SourceId;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Child Journey")
    EWMChildAdventureKind Kind = EWMChildAdventureKind::Mission;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Child Journey")
    EWMChildAdventureState State = EWMChildAdventureState::Hidden;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Child Journey")
    int32 CurrentUnits = 0;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Child Journey")
    int32 TotalUnits = 1;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Child Journey")
    float ProgressFraction = 0.0f;

    /** Only mission cards may request activation, and only when the mission domain says they are available. */
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Child Journey")
    bool bSelectable = false;

    bool IsSane() const;
};

USTRUCT(BlueprintType)
struct WORLDMAKERS_API FWMChildJourneySnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Child Journey")
    FName BiomeId;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Child Journey")
    FName ZoneId;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Child Journey")
    FName EcosystemReactionId;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Child Journey")
    TArray<FWMChildAdventureCard> Cards;

    int32 GetVisibleCardCount() const;
    int32 GetCompletedCardCount() const;
};

/** Pure mapping rules keep the child-facing projection deterministic and time-independent. */
struct WORLDMAKERS_API FWMChildJourneyRules
{
    static EWMChildAdventureState FromMissionState(EWMJourneyMissionState State);
    static EWMChildAdventureState FromCountProgress(int32 CurrentUnits, int32 TotalUnits);
    static EWMChildAdventureState FromInterventionState(bool bCompleted, bool bPrerequisitesSatisfied, int32 CurrentUnits, int32 TotalUnits);
    static EWMChildAdventureState FromUnlockState(bool bGranted, bool bPrerequisiteVisible);
};
