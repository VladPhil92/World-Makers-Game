#pragma once

#include "CoreMinimal.h"
#include "Building/WMBuildCatalogSettings.h"
#include "GameFramework/SaveGame.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "WMBuildUnlockSubsystem.generated.h"

/** Pure deterministic creative-unlock model. Stable reward IDs only. */
struct WORLDMAKERS_API FWMBuildUnlockModel
{
    bool Grant(FName RewardId);
    bool Has(FName RewardId) const;
    bool IsPieceUnlocked(const FWMBuildPieceSpec& Spec) const;
    void Restore(const TArray<FName>& RewardIds);
    TArray<FName> Export() const;

private:
    TSet<FName> GrantedRewardIds;
};

UCLASS()
class WORLDMAKERS_API UWMBuildUnlockSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    static constexpr int32 CurrentFormatVersion = 1;

    UPROPERTY(SaveGame)
    int32 FormatVersion = CurrentFormatVersion;

    UPROPERTY(SaveGame)
    TArray<FName> GrantedRewardIds;
};

UCLASS()
class WORLDMAKERS_API UWMBuildUnlockSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintPure, Category = "World Makers|Building|Unlocks")
    bool IsRewardGranted(FName RewardId) const { return Model.Has(RewardId); }

    UFUNCTION(BlueprintPure, Category = "World Makers|Building|Unlocks")
    bool IsPieceUnlocked(FName PieceId) const;

    UFUNCTION(BlueprintPure, Category = "World Makers|Building|Unlocks")
    bool IsKnownUnlockReward(FName RewardId) const;

    /** Idempotent: true means the known reward is granted after this call. */
    UFUNCTION(BlueprintCallable, Category = "World Makers|Building|Unlocks")
    bool EnsureRewardGranted(FName RewardId);

    UFUNCTION(BlueprintPure, Category = "World Makers|Building|Unlocks")
    TArray<FName> GetGrantedRewardIds() const { return Model.Export(); }

    UFUNCTION(BlueprintPure, Category = "World Makers|Building|Unlocks")
    TArray<FName> GetUnlockedPieceIds() const;

    UFUNCTION(BlueprintCallable, Category = "World Makers|Building|Unlocks")
    bool LoadProgress();

    UFUNCTION(BlueprintCallable, Category = "World Makers|Building|Unlocks")
    bool SaveProgress() const;

private:
    FWMBuildUnlockModel Model;
};
