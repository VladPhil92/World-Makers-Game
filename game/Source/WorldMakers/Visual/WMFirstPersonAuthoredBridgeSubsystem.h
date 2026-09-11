#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Visual/WMFirstPersonAuthoredRuntime.h"
#include "WMFirstPersonAuthoredBridgeSubsystem.generated.h"

class AWMPlayerCharacter;
class UAnimationAsset;
class USkeletalMesh;
class USkeletalMeshComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UWMFirstPersonInteractionComponent;

/**
 * Optional production-art bridge for the source-proxy first-person kit.
 * It is presentation-only and remains fail-closed unless the full authored set exists and
 * -WMEnableFirstPersonAuthored is supplied for an explicit native review/build.
 */
UCLASS()
class WORLDMAKERS_API UWMFirstPersonAuthoredBridgeSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override { return true; }

    UFUNCTION(BlueprintPure, Category = "World Makers|First Person|Authored")
    bool IsAuthoredTakeoverReady() const;

    UFUNCTION(BlueprintPure, Category = "World Makers|First Person|Authored")
    bool IsAuthoredTakeoverActive() const { return bTakeoverActive; }

private:
    void EnsureTargets();
    void TryLoadAuthoredAssets();
    void ApplyAuthoredTakeover();
    void ApplyProxyFallback();
    void PlayActionIfChanged(FName ActionId);
    UStaticMeshComponent* FindStaticMeshComponent(FName ComponentName) const;
    UStaticMesh* ResolveAuthoredTool(FName ModeId) const;

    UPROPERTY(Transient)
    TObjectPtr<AWMPlayerCharacter> Character;

    UPROPERTY(Transient)
    TObjectPtr<UWMFirstPersonInteractionComponent> Interaction;

    UPROPERTY(Transient)
    TObjectPtr<USkeletalMeshComponent> AuthoredArmsComponent;

    UPROPERTY(Transient)
    TObjectPtr<UStaticMeshComponent> LeftHandProxy;

    UPROPERTY(Transient)
    TObjectPtr<UStaticMeshComponent> RightHandProxy;

    UPROPERTY(Transient)
    TObjectPtr<UStaticMeshComponent> ToolProxy;

    UPROPERTY(Transient)
    TObjectPtr<UStaticMeshComponent> WristProxy;

    UPROPERTY(Transient)
    TObjectPtr<USkeletalMesh> ArmsAsset;

    UPROPERTY(Transient)
    TObjectPtr<UStaticMesh> ScannerAsset;

    UPROPERTY(Transient)
    TObjectPtr<UStaticMesh> BuildToolAsset;

    UPROPERTY(Transient)
    TObjectPtr<UStaticMesh> MeasureToolAsset;

    UPROPERTY(Transient)
    TObjectPtr<UStaticMesh> WristAsset;

    UPROPERTY(Transient)
    TObjectPtr<UStaticMesh> ProxyToolFallback;

    UPROPERTY(Transient)
    TObjectPtr<UStaticMesh> ProxyWristFallback;

    UPROPERTY(Transient)
    TMap<FName, TObjectPtr<UAnimationAsset>> Animations;

    FWMFirstPersonAuthoredAvailability Availability;
    FName LastPlayedActionId = NAME_None;
    bool bLoadAttempted = false;
    bool bExplicitTakeoverEnabled = false;
    bool bTakeoverActive = false;
};
