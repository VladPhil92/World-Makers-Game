#pragma once

#include "CoreMinimal.h"
#include "WMMissionTypes.generated.h"

UENUM(BlueprintType)
enum class EWMMissionRuntimeState : uint8
{
    Inactive,
    Active,
    Completed
};

USTRUCT(BlueprintType)
struct WORLDMAKERS_API FWMLearningEvidenceRecord
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Makers|Learning")
    FName MissionId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Makers|Learning")
    FName EventId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Makers|Learning")
    FName ObjectiveId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Makers|Learning")
    float NumericValue = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Makers|Learning")
    int32 Sequence = 0;
};

USTRUCT(BlueprintType)
struct WORLDMAKERS_API FWMMissionRuntimeDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Makers|Missions")
    FName MissionId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Makers|Missions")
    TArray<FName> LearningObjectiveIds;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Makers|Missions")
    TArray<FName> RequiredEvidenceEventIds;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Makers|Missions")
    float TargetSpanCm = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Makers|Missions")
    float ToleranceCm = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Makers|Missions")
    TArray<FName> RewardIds;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Makers|Missions")
    bool bPrototypeOnly = true;

    bool IsSane() const;
    static bool TryParseJson(const FString& Json, FWMMissionRuntimeDefinition& OutDefinition, FString& OutError);
};

/** Pure deterministic progress model used by runtime and automation tests. */
struct WORLDMAKERS_API FWMMissionProgressModel
{
    FWMMissionRuntimeDefinition Definition;
    EWMMissionRuntimeState State = EWMMissionRuntimeState::Inactive;
    bool bMeasurementEvidence = false;
    bool bStructureFitEvidence = false;
    float LastMeasuredSpanCm = 0.0f;
    float LastObservedStructureSpanCm = 0.0f;
    int32 NextEvidenceSequence = 1;
    TArray<FWMLearningEvidenceRecord> Evidence;
    TArray<FName> EarnedRewardIds;

    bool Begin(const FWMMissionRuntimeDefinition& InDefinition);
    bool RecordMeasurement(float MeasuredSpanCm);
    bool RecordStructureSpan(float StructureSpanCm);
    float GetProgressFraction() const;
};
