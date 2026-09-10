#pragma once

#include "CoreMinimal.h"
#include "WMAdventureRuntime.generated.h"

USTRUCT(BlueprintType)
struct WORLDMAKERS_API FWMAdventureBeatDefinition
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Adventure")
    FName BeatId;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Adventure")
    FName PrimitiveId;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Adventure")
    FName EvidenceEventId;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Adventure")
    FName ProducerKind;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Adventure")
    FName ProducerRefId;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Adventure")
    FName PromptKey;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Adventure")
    FName FormalizationKey;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Adventure")
    int32 RequiredCount = 1;

    bool IsSane() const;
};

USTRUCT(BlueprintType)
struct WORLDMAKERS_API FWMFantasticAdventureDefinition
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Adventure")
    FName AdventureId;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Adventure")
    FName MissionId;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Adventure")
    FName TitleKey;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Adventure")
    FName PremiseKey;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Adventure")
    FName PrimaryDiscipline;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Adventure")
    TArray<FName> SecondaryDisciplines;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Adventure")
    FName AgeBand;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Adventure")
    TArray<FWMAdventureBeatDefinition> Beats;

    bool IsSane() const;
    const FWMAdventureBeatDefinition* FindBeat(FName BeatId) const;
};

USTRUCT(BlueprintType)
struct WORLDMAKERS_API FWMFantasticAdventurePack
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Adventure")
    int32 SchemaVersion = 1;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Adventure")
    FName PackId;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Adventure")
    bool bPrototypeOnly = true;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Adventure")
    FName DesignPrinciple;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Adventure")
    FName ProgressionModel;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Adventure")
    FName PrivacyModel;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Adventure")
    TArray<FWMFantasticAdventureDefinition> Adventures;

    bool IsSane() const;
    const FWMFantasticAdventureDefinition* FindAdventure(FName AdventureId) const;
    static bool TryParseJson(const FString& Json, FWMFantasticAdventurePack& OutPack, FString& OutError);
};

USTRUCT(BlueprintType)
struct WORLDMAKERS_API FWMAdventureProgressReadModel
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Adventure")
    FName AdventureId;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Adventure")
    FName CurrentBeatId;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Adventure")
    int32 CurrentBeatIndex = 0;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Adventure")
    int32 BeatCount = 0;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Adventure")
    int32 CurrentBeatEvidenceCount = 0;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Adventure")
    float ProgressFraction = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Adventure")
    bool bCompleted = false;
};

/** Pure ordered-beat progress model. It stores stable IDs and bounded counters only. */
struct WORLDMAKERS_API FWMAdventureProgressModel
{
    bool Begin(const FWMFantasticAdventureDefinition& InDefinition);
    bool CanAcceptEvidence(FName ProducerKind, FName ProducerRefId, FName PrimitiveId, FName EvidenceEventId) const;
    bool CommitEvidence(FName ProducerKind, FName ProducerRefId, FName PrimitiveId, FName EvidenceEventId);
    const FWMAdventureBeatDefinition* GetCurrentBeat() const;
    float GetProgressFraction() const;
    bool IsActive() const { return bActive; }
    bool IsCompleted() const { return bCompleted; }
    FWMAdventureProgressReadModel BuildReadModel() const;
    void Reset();

private:
    FWMFantasticAdventureDefinition Definition;
    int32 CurrentBeatIndex = 0;
    int32 CurrentBeatEvidenceCount = 0;
    bool bActive = false;
    bool bCompleted = false;
};
