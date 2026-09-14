#pragma once

#include "Adventure/WMEpicJourneySaveGame.h"
#include "Adventure/WMEpicRuntime.h"
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WMEpicRuntimeSubsystem.generated.h"

UCLASS()
class WORLDMAKERS_API UWMEpicRuntimeSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "World Makers|Epic")
    bool ReloadEpicCatalog();

    UFUNCTION(BlueprintPure, Category = "World Makers|Epic")
    bool IsEpicCatalogLoaded() const { return bCatalogLoaded; }

    UFUNCTION(BlueprintPure, Category = "World Makers|Epic")
    TArray<FName> GetEpicIds() const;

    /** Starts an epic from chapter zero and replaces any prior checkpoint for it. */
    UFUNCTION(BlueprintCallable, Category = "World Makers|Epic")
    bool ActivateEpic(FName EpicId);

    /** Restarts the saved current chapter from a clean authoritative state. */
    UFUNCTION(BlueprintCallable, Category = "World Makers|Epic")
    bool ResumeEpic(FName EpicId);

    /** Resume when possible; otherwise start fresh. */
    UFUNCTION(BlueprintCallable, Category = "World Makers|Epic")
    bool ActivateOrResumeEpic(FName EpicId);

    /**
     * Imports a server-authoritative resumable chapter without allowing local regression.
     * Completed or equally/newer local progress wins; no partial evidence is imported.
     */
    UFUNCTION(BlueprintCallable, Category = "World Makers|Epic")
    bool ApplyExternalResumeCheckpoint(FName EpicId, FName ChapterId, int32 ChapterIndex, int32 ChapterCount);

    UFUNCTION(BlueprintPure, Category = "World Makers|Epic")
    bool HasResumableEpicCheckpoint(FName EpicId) const;

    UFUNCTION(BlueprintPure, Category = "World Makers|Epic")
    FName GetEpicCheckpointChapterId(FName EpicId) const;

    UFUNCTION(BlueprintCallable, Category = "World Makers|Epic")
    bool ClearEpicCheckpoint(FName EpicId);

    /** Retry the coalesced checkpoint for the epic authorized by the current native sync token. */
    bool FlushPendingEpicCheckpointSyncs(FName EpicId);

    /** Remove an outbox item only when the server confirms equal/newer progress. */
    bool AcknowledgeEpicCheckpointSync(const FWMEpicCheckpoint& AcknowledgedCheckpoint);

    /** Trusted pedagogical evidence boundary. Objective + discipline + producer + primitive + event must all match. */
    UFUNCTION(BlueprintCallable, Category = "World Makers|Epic")
    bool RecordEpicEvidence(
        FName ObjectiveId,
        FName DisciplineId,
        FName ProducerKind,
        FName ProducerRefId,
        FName PrimitiveId,
        FName EvidenceEventId,
        float NumericValue = 1.0f);

    /** Trusted causal-world boundary. A client completion flag is never accepted as a substitute. */
    UFUNCTION(BlueprintCallable, Category = "World Makers|Epic")
    bool RecordEpicWorldState(FName ProducerKind, FName ProducerRefId, FName WorldStateId);

    /** Safe retry point if the next mission could not be activated after a chapter became ready. */
    UFUNCTION(BlueprintCallable, Category = "World Makers|Epic")
    bool AdvanceEpicIfReady();

    UFUNCTION(BlueprintPure, Category = "World Makers|Epic")
    bool IsEpicActive() const { return Progress.IsActive(); }

    UFUNCTION(BlueprintPure, Category = "World Makers|Epic")
    FName GetActiveEpicId() const { return Progress.BuildReadModel().EpicId; }

    UFUNCTION(BlueprintPure, Category = "World Makers|Epic")
    FName GetCurrentEpicChapterId() const { return Progress.BuildReadModel().CurrentChapterId; }

    UFUNCTION(BlueprintPure, Category = "World Makers|Epic")
    FWMEpicProgressReadModel GetEpicProgress() const { return Progress.BuildReadModel(); }

    const FWMEpicCatalog& GetCatalog() const { return Catalog; }
    const TArray<FWMEpicCheckpoint>& GetCheckpointsForTests() const { return Checkpoints; }
    const TArray<FWMEpicCheckpoint>& GetPendingSyncCheckpointsForTests() const { return PendingSyncCheckpoints; }

private:
    bool LoadEpicCheckpoints();
    bool SaveEpicCheckpoints() const;
    bool SaveCurrentCheckpoint();
    int32 FindCheckpointIndex(FName EpicId) const;
    int32 FindPendingSyncCheckpointIndex(FName EpicId) const;
    bool QueuePendingSyncCheckpoint(const FWMEpicCheckpoint& Checkpoint);
    bool IsCheckpointValidForCatalog(const FWMEpicCheckpoint& Checkpoint) const;
    static bool IsAtLeastAsAdvanced(const FWMEpicCheckpoint& Candidate, const FWMEpicCheckpoint& Baseline);

    bool bCatalogLoaded = false;
    FWMEpicCatalog Catalog;
    FWMEpicProgressModel Progress;

    UPROPERTY(Transient)
    TArray<FWMEpicCheckpoint> Checkpoints;

    UPROPERTY(Transient)
    TArray<FWMEpicCheckpoint> PendingSyncCheckpoints;
};