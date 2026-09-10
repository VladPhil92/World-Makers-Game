#pragma once

#include "Adventure/WMAdventureRuntime.h"
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WMAdventureRuntimeSubsystem.generated.h"

UCLASS()
class WORLDMAKERS_API UWMAdventureRuntimeSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "World Makers|Adventure")
    bool ReloadAdventurePack();

    UFUNCTION(BlueprintPure, Category = "World Makers|Adventure")
    bool IsAdventurePackLoaded() const { return bPackLoaded; }

    UFUNCTION(BlueprintPure, Category = "World Makers|Adventure")
    TArray<FName> GetAdventureIds() const;

    UFUNCTION(BlueprintCallable, Category = "World Makers|Adventure")
    bool ActivateAdventure(FName AdventureId);

    /**
     * Trusted evidence boundary for ordered adventures. Evidence is accepted only when the
     * producer identity, primitive and event exactly match the current beat. The underlying
     * M5.2 mission runtime remains the authoritative learning-progress/reward ledger.
     */
    UFUNCTION(BlueprintCallable, Category = "World Makers|Adventure")
    bool RecordAdventureEvidence(
        FName ProducerKind,
        FName ProducerRefId,
        FName PrimitiveId,
        FName EvidenceEventId,
        float NumericValue = 1.0f);

    UFUNCTION(BlueprintPure, Category = "World Makers|Adventure")
    bool IsAdventureActive() const { return Progress.IsActive(); }

    UFUNCTION(BlueprintPure, Category = "World Makers|Adventure")
    FName GetActiveAdventureId() const { return Progress.BuildReadModel().AdventureId; }

    UFUNCTION(BlueprintPure, Category = "World Makers|Adventure")
    FName GetCurrentBeatId() const { return Progress.BuildReadModel().CurrentBeatId; }

    UFUNCTION(BlueprintPure, Category = "World Makers|Adventure")
    FWMAdventureProgressReadModel GetAdventureProgress() const { return Progress.BuildReadModel(); }

    const FWMFantasticAdventurePack& GetPack() const { return Pack; }

private:
    bool bPackLoaded = false;
    FWMFantasticAdventurePack Pack;
    FWMAdventureProgressModel Progress;
};
