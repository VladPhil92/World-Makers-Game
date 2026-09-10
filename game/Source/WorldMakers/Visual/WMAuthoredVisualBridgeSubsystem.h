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
    virtual bool IsTickable() const override { return RetryRemainingSeconds > 0.0f; }

    UFUNCTION(BlueprintCallable, Category = "World Makers|Visual|Authored")
    void RefreshAuthoredVisuals();

    UFUNCTION(BlueprintPure, Category = "World Makers|Visual|Authored")
    bool IsAuthoredEnvironmentActive() const { return bAuthoredEnvironmentActive; }

    /** True when a conforming authored Skeletal Mesh has been loaded into the production mesh slot. V4 still decides whether it is visible. */
    UFUNCTION(BlueprintPure, Category = "World Makers|Visual|Authored")
    bool IsAuthoredAvatarReady() const { return bAuthoredAvatarReady; }

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

    float RetryRemainingSeconds = 0.0f;
    float RetryAccumulatorSeconds = 0.0f;
    bool bAuthoredEnvironmentActive = false;
    bool bAuthoredAvatarReady = false;
};
