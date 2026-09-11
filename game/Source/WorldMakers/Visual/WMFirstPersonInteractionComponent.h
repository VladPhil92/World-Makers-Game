#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Visual/WMFirstPersonInteractionRuntime.h"
#include "Visual/WMReferenceVisualPolishRuntime.h"
#include "WMFirstPersonInteractionComponent.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UWMFirstPersonContextWidget;
class AWMPlayerCharacter;

UCLASS(ClassGroup = (WorldMakers), meta = (BlueprintSpawnableComponent))
class WORLDMAKERS_API UWMFirstPersonInteractionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UWMFirstPersonInteractionComponent();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintCallable, Category = "World Makers|First Person")
    bool PulseSemanticEvent(FName EventId, FName ActionId, float DurationSeconds = 0.85f);

    UFUNCTION(BlueprintCallable, Category = "World Makers|First Person")
    bool SetPersistentModeById(FName ModeId, bool bEnabled);

    UFUNCTION(BlueprintCallable, Category = "World Makers|First Person")
    void ExitFirstPersonInteraction();

    UFUNCTION(BlueprintPure, Category = "World Makers|First Person")
    bool IsFirstPersonInteractionActive() const { return bModeActive; }

    UFUNCTION(BlueprintPure, Category = "World Makers|First Person")
    FName GetActiveModeId() const { return FWMReferenceVisualPolishRuntime::FirstPersonModeToId(ActiveMode); }

    UFUNCTION(BlueprintPure, Category = "World Makers|First Person")
    FName GetActiveActionId() const { return Runtime.GetActionId(); }

private:
    void EnsureViewModel();
    void EnsureContextWidget();
    void SetViewModelVisible(bool bVisible);
    bool ActivateMode(EWMFirstPersonVisualMode Mode, EWMFirstPersonInteractionAction Action, float DurationSeconds, bool bPersistent);
    bool TryParseMode(FName ModeId, EWMFirstPersonVisualMode& OutMode) const;
    void ApplyViewModelPose(const FWMFirstPersonInteractionPose& Pose);
    bool ResolveReducedMotion() const;

    UPROPERTY(Transient)
    TObjectPtr<AWMPlayerCharacter> CharacterOwner;

    UPROPERTY(Transient)
    TObjectPtr<USceneComponent> ViewModelRoot;

    UPROPERTY(Transient)
    TObjectPtr<UStaticMeshComponent> LeftHandProxy;

    UPROPERTY(Transient)
    TObjectPtr<UStaticMeshComponent> RightHandProxy;

    UPROPERTY(Transient)
    TObjectPtr<UStaticMeshComponent> ToolProxy;

    UPROPERTY(Transient)
    TObjectPtr<UStaticMeshComponent> WristDeviceProxy;

    UPROPERTY(Transient)
    TObjectPtr<UWMFirstPersonContextWidget> ContextWidget;

    FWMFirstPersonInteractionRuntime Runtime;
    FWMFirstPersonVisualProfile ActiveProfile;
    EWMFirstPersonVisualMode ActiveMode = EWMFirstPersonVisualMode::Explore;
    float ModeRemainingSeconds = 0.0f;
    bool bModeActive = false;
    bool bPersistentMode = false;
};
