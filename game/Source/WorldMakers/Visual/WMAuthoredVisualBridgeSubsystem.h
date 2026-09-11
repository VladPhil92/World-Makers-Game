#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WMAuthoredVisualBridgeSubsystem.generated.h"

class AWMCaribbeanRainforestPrototype;
class AWMPlayerCharacter;
class UHierarchicalInstancedStaticMeshComponent;
class UStaticMesh;
class UWMAuthoredAssetSubsystem;

UCLASS()
class WORLDMAKERS_API UWMAuthoredVisualBridgeSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override
    {
        return RetryRemainingSeconds > 0.0f || (!TakeoverReportPath.IsEmpty() && !bTakeoverReportWritten);
    }

    UFUNCTION(BlueprintCallable, Category = "World Makers|Visual|Authored")
    void RefreshAuthoredVisuals();

    UFUNCTION(BlueprintPure, Category = "World Makers|Visual|Authored")
    bool IsAuthoredEnvironmentActive() const { return bAuthoredEnvironmentActive; }

    UFUNCTION(BlueprintPure, Category = "World Makers|Visual|Authored")
    bool IsAuthoredAvatarReady() const { return bAuthoredAvatarReady; }

    UFUNCTION(BlueprintPure, Category = "World Makers|Visual|Authored")
    bool IsAuthoredAvatarActive() const { return bAuthoredAvatarActive; }

    UFUNCTION(BlueprintPure, Category = "World Makers|Visual|Authored")
    bool IsAuthoredAnimationBlueprintActive() const { return bAuthoredAnimationBlueprintActive; }

    UFUNCTION(BlueprintPure, Category = "World Makers|Visual|Authored")
    bool IsAuthoredPresentationReady() const;

    UFUNCTION(BlueprintPure, Category = "World Makers|Visual|Authored")
    bool IsFullAuthoredTakeoverActive() const;

private:
    UWMAuthoredAssetSubsystem* GetAssetSubsystem() const;
    bool TryPrepareAuthoredAvatar(AWMPlayerCharacter* Character, UWMAuthoredAssetSubsystem* Assets);
    bool TryApplyAuthoredEnvironment(AWMCaribbeanRainforestPrototype* Biome, UWMAuthoredAssetSubsystem* Assets);
    UHierarchicalInstancedStaticMeshComponent* CreateRenderFamily(
        AWMCaribbeanRainforestPrototype* Biome,
        FName ComponentName,
        UStaticMesh* Mesh) const;
    void SetAuthoredEnvironmentVisible(AWMCaribbeanRainforestPrototype* Biome, bool bVisible) const;
    void SetProceduralEnvironmentVisible(AWMCaribbeanRainforestPrototype* Biome, bool bVisible) const;
    void TryWriteN2TakeoverReport(bool bForce);

    float RetryRemainingSeconds = 0.0f;
    float RetryAccumulatorSeconds = 0.0f;
    bool bAuthoredEnvironmentActive = false;
    bool bAuthoredAvatarReady = false;
    bool bAuthoredAvatarActive = false;
    bool bAuthoredAnimationBlueprintActive = false;
    FString TakeoverReportPath;
    FString TakeoverBuildCommit;
    bool bTakeoverReportWritten = false;
};
