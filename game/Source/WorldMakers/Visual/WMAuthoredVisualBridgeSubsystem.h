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
class WORLDMAKERS_API UWMAuthoredVisualBridgeSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;

    UFUNCTION(BlueprintCallable, Category = "World Makers|Visual|Authored")
    void RefreshAuthoredVisuals();

    UFUNCTION(BlueprintPure, Category = "World Makers|Visual|Authored")
    bool IsAuthoredEnvironmentActive() const { return bAuthoredEnvironmentActive; }

    UFUNCTION(BlueprintPure, Category = "World Makers|Visual|Authored")
    bool IsAuthoredAvatarActive() const { return bAuthoredAvatarActive; }

private:
    UWMAuthoredAssetSubsystem* GetAssetSubsystem() const;
    bool TryApplyAuthoredAvatar(AWMPlayerCharacter* Character, UWMAuthoredAssetSubsystem* Assets);
    bool TryApplyAuthoredEnvironment(AWMCaribbeanRainforestPrototype* Biome, UWMAuthoredAssetSubsystem* Assets);
    UHierarchicalInstancedStaticMeshComponent* CreateRenderFamily(
        AWMCaribbeanRainforestPrototype* Biome,
        FName ComponentName,
        UStaticMesh* Mesh) const;

    bool bAuthoredEnvironmentActive = false;
    bool bAuthoredAvatarActive = false;
};
