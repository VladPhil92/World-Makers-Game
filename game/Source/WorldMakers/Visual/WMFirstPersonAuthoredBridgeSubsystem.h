#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Visual/WMFirstPersonAuthoredRuntime.h"
#include "Visual/WMFirstPersonNativeActivationRuntime.h"
#include "WMFirstPersonAuthoredBridgeSubsystem.generated.h"

class AWMPlayerCharacter;
class UAnimationAsset;
class USkeletalMesh;
class USkeletalMeshComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UWMFirstPersonInteractionComponent;

/**
 * Production-art bridge for the source-proxy first-person kit.
 * Review mode is explicitly opt-in with -WMEnableFirstPersonAuthored.
 * Production takeover requires an activated manifest plus build-time provenance binding the
 * packaged build to the source commit that actually received native review.
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

    UFUNCTION(BlueprintPure, Category = "World Makers|First Person|Authored")
    bool IsProductionActivationApproved() const { return bProductionActivationApproved; }

    UFUNCTION(BlueprintPure, Category = "World Makers|First Person|Authored")
    bool IsReviewTakeoverEnabled() const { return bReviewTakeoverEnabled; }

private:
    void EnsureTargets();
    void TryLoadAuthoredAssets();
    void ApplyAuthoredTakeover();
    void ApplyProxyFallback();
    void PlayActionIfChanged(FName ActionId);
    void WriteTakeoverReport() const;
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
    FWMFirstPersonNativeActivationState ActivationState;
    FWMFirstPersonBuildProvenance BuildProvenance;
    FString RequestedBuildCommitSha;
    FString TakeoverReportPath;
    FName LastPlayedActionId = NAME_None;
    bool bLoadAttempted = false;
    bool bReviewTakeoverEnabled = false;
    bool bProductionActivationApproved = false;
    bool bTakeoverActive = false;
    bool bCertificationMode = false;
};
