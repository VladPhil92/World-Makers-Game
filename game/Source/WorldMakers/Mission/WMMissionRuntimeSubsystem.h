#pragma once

#include "CoreMinimal.h"
#include "Mission/WMMissionTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "WMMissionRuntimeSubsystem.generated.h"

class AWMMissionGeometryActor;

UCLASS()
class WORLDMAKERS_API UWMMissionRuntimeSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    /** Backward-compatible bootstrap for the default prototype mission. */
    UFUNCTION(BlueprintCallable, Category = "World Makers|Missions")
    bool ReloadAndActivatePrototypeMission();

    UFUNCTION(BlueprintCallable, Category = "World Makers|Missions")
    bool ReloadMissionCatalog();

    UFUNCTION(BlueprintCallable, Category = "World Makers|Missions")
    bool ActivateMission(FName MissionId);

    UFUNCTION(BlueprintCallable, Category = "World Makers|Missions")
    bool CycleMission(int32 Direction = 1);

    /** Legacy M2.2 catalog ordering. Includes locked definitions; use journey read model for state. */
    UFUNCTION(BlueprintPure, Category = "World Makers|Missions")
    TArray<FName> GetAvailableMissionIds() const { return AvailableMissionIds; }

    UFUNCTION(BlueprintPure, Category = "World Makers|Missions")
    TArray<FName> GetActivatableMissionIds() const;

    UFUNCTION(BlueprintPure, Category = "World Makers|Missions")
    int32 GetMissionCount() const { return AvailableMissionIds.Num(); }

    UFUNCTION(BlueprintPure, Category = "World Makers|Missions")
    int32 GetCompletedMissionCount() const { return Journey.CompletedMissionIds.Num(); }

    UFUNCTION(BlueprintPure, Category = "World Makers|Missions")
    FName GetActiveMissionId() const { return Progress.State == EWMMissionRuntimeState::Inactive ? NAME_None : Progress.Definition.MissionId; }

    UFUNCTION(BlueprintPure, Category = "World Makers|Missions")
    FName GetActiveEvaluator() const { return Progress.State == EWMMissionRuntimeState::Inactive ? NAME_None : Progress.Definition.Evaluator; }

    UFUNCTION(BlueprintPure, Category = "World Makers|Learning Journey")
    EWMJourneyMissionState GetJourneyMissionState(FName MissionId) const;

    UFUNCTION(BlueprintPure, Category = "World Makers|Learning Journey")
    TArray<FWMJourneyMissionReadModel> GetJourneyReadModel() const;

    UFUNCTION(BlueprintPure, Category = "World Makers|Learning Journey")
    TArray<FName> GetGrantedRewardIds() const;

    UFUNCTION(BlueprintCallable, Category = "World Makers|Learning Journey")
    bool LoadJourneyProgress();

    UFUNCTION(BlueprintCallable, Category = "World Makers|Learning Journey")
    bool SaveJourneyProgress() const;

    UFUNCTION(BlueprintCallable, Category = "World Makers|Missions|Mathematics")
    bool RecordMeasurement(float MeasuredSpanCm);

    /** Legacy M2.2 catalog helper retained for compatibility; child UI uses the interactive measurement component. */
    UFUNCTION(BlueprintCallable, Category = "World Makers|Missions|Mathematics")
    bool RecordActiveGeometryMeasurement();

    UFUNCTION(BlueprintCallable, Category = "World Makers|Missions|Mathematics")
    bool RecordStructureSpan(float StructureSpanCm);

    /** Mission-side M3.3 entry point for stable biome observation evidence. */
    UFUNCTION(BlueprintCallable, Category = "World Makers|Missions|Science")
    bool RecordObservationEvidence(FName ObservationId);

    UFUNCTION(BlueprintPure, Category = "World Makers|Missions|Science")
    bool IsObservationRequired(FName ObservationId) const;

    UFUNCTION(BlueprintPure, Category = "World Makers|Missions|Science")
    TArray<FName> GetRequiredObservationIds() const;

    UFUNCTION(BlueprintPure, Category = "World Makers|Missions|Science")
    int32 GetRecordedObservationCount() const { return Progress.GetRecordedObservationCount(); }

    /**
     * M5.2 generic mission evidence entry point. World systems submit only stable primitive/event IDs
     * plus an optional numeric value; no free text or subject-specific payload is retained here.
     */
    UFUNCTION(BlueprintCallable, Category = "World Makers|Missions|Composable")
    bool RecordComposableEvidence(FName PrimitiveId, FName EvidenceEventId, float NumericValue = 1.0f);

    UFUNCTION(BlueprintPure, Category = "World Makers|Missions|Composable")
    bool IsComposableEvidenceRequired(FName PrimitiveId, FName EvidenceEventId) const;

    UFUNCTION(BlueprintPure, Category = "World Makers|Missions|Composable")
    TArray<FName> GetRequiredEvidencePrimitiveIds() const;

    UFUNCTION(BlueprintPure, Category = "World Makers|Missions|Composable")
    int32 GetRecordedComposableEvidenceCount(FName EvidenceEventId) const { return Progress.GetRecordedComposableEvidenceCount(EvidenceEventId); }

    void RegisterMissionGeometry(AWMMissionGeometryActor* GeometryActor);
    void UnregisterMissionGeometry(AWMMissionGeometryActor* GeometryActor);

    UFUNCTION(BlueprintPure, Category = "World Makers|Missions")
    AWMMissionGeometryActor* GetActiveMissionGeometry() const { return ActiveGeometry.Get(); }

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
    float GetLastMeasuredSpanCm() const { return LastMeasuredSpanCm; }

    UFUNCTION(BlueprintPure, Category = "World Makers|Missions")
    float GetProgressFraction() const { return Progress.GetProgressFraction(); }

    /** New rewards from the current completion only; lifetime grants are exposed by GetGrantedRewardIds. */
    UFUNCTION(BlueprintPure, Category = "World Makers|Missions")
    TArray<FName> GetEarnedRewardIds() const { return Progress.EarnedRewardIds; }

    const TArray<FWMLearningEvidenceRecord>& GetEvidence() const { return Progress.Evidence; }

private:
    bool ValidateCatalogDependencies() const;
    bool ActivateBestStartupMission();
    void FinalizeMissionCompletion();

    FWMMissionProgressModel Progress;
    FWMMissionJourneyModel Journey;
    TWeakObjectPtr<AWMMissionGeometryActor> ActiveGeometry;
    TMap<FName, FWMMissionRuntimeDefinition> MissionCatalog;
    TArray<FName> AvailableMissionIds;
    FName LastSavedActiveMissionId;
    float LastMeasuredSpanCm = 0.0f;
};
