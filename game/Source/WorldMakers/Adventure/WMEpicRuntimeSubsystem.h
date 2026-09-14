#pragma once

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

    UFUNCTION(BlueprintCallable, Category = "World Makers|Epic")
    bool ActivateEpic(FName EpicId);

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

private:
    bool bCatalogLoaded = false;
    FWMEpicCatalog Catalog;
    FWMEpicProgressModel Progress;
};
