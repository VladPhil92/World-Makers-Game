#pragma once

#include "CoreMinimal.h"
#include "Mission/WMMissionTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "WMMissionRuntimeSubsystem.generated.h"

UCLASS()
class WORLDMAKERS_API UWMMissionRuntimeSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "World Makers|Missions")
    bool ReloadAndActivatePrototypeMission();

    UFUNCTION(BlueprintCallable, Category = "World Makers|Missions")
    bool RecordMeasurement(float MeasuredSpanCm);

    UFUNCTION(BlueprintCallable, Category = "World Makers|Missions")
    bool RecordStructureSpan(float StructureSpanCm);

    UFUNCTION(BlueprintPure, Category = "World Makers|Missions")
    EWMMissionRuntimeState GetMissionState() const { return Progress.State; }

    UFUNCTION(BlueprintPure, Category = "World Makers|Missions")
    bool HasMeasurementEvidence() const { return Progress.bMeasurementEvidence; }

    UFUNCTION(BlueprintPure, Category = "World Makers|Missions")
    bool HasStructureFitEvidence() const { return Progress.bStructureFitEvidence; }

    UFUNCTION(BlueprintPure, Category = "World Makers|Missions")
    float GetTargetSpanCm() const { return Progress.Definition.TargetSpanCm; }

    UFUNCTION(BlueprintPure, Category = "World Makers|Missions")
    float GetToleranceCm() const { return Progress.Definition.ToleranceCm; }

    UFUNCTION(BlueprintPure, Category = "World Makers|Missions")
    float GetProgressFraction() const { return Progress.GetProgressFraction(); }

    UFUNCTION(BlueprintPure, Category = "World Makers|Missions")
    TArray<FName> GetEarnedRewardIds() const { return Progress.EarnedRewardIds; }

    const TArray<FWMLearningEvidenceRecord>& GetEvidence() const { return Progress.Evidence; }

private:
    FWMMissionProgressModel Progress;
};
