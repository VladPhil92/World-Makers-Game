#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Visual/WMPresentationRuntime.h"
#include "Visual/WMReferenceVisualPolishRuntime.h"
#include "WMPresentationSubsystem.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UWMFirstPersonInteractionComponent;
class UWMPresentationOverlayWidget;

UCLASS()
class WORLDMAKERS_API UWMPresentationSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override { return true; }

    bool PulseCameraMode(EWMPresentationCameraMode Mode, float DurationSeconds = 0.8f, FName CueId = NAME_None);

    /** Presentation-only override used by first-person interaction viewmodels. */
    bool SetFirstPersonInteractionMode(EWMFirstPersonVisualMode Mode, bool bEnabled);

    UFUNCTION(BlueprintCallable, Category = "World Makers|Presentation|First Person")
    bool SetFirstPersonInteractionModeById(FName ModeId, bool bEnabled);

    UFUNCTION(BlueprintPure, Category = "World Makers|Presentation|First Person")
    bool IsFirstPersonInteractionModeActive() const { return bFirstPersonInteractionActive; }

    UFUNCTION(BlueprintPure, Category = "World Makers|Presentation|First Person")
    FName GetFirstPersonInteractionModeId() const
    {
        return FWMReferenceVisualPolishRuntime::FirstPersonModeToId(FirstPersonMode);
    }

    UFUNCTION(BlueprintCallable, Category = "World Makers|Presentation|Accessibility")
    void SetReducedMotion(bool bEnabled);

    UFUNCTION(BlueprintPure, Category = "World Makers|Presentation|Accessibility")
    bool IsReducedMotion() const { return bReducedMotion; }

    EWMPresentationCameraMode GetActiveCameraMode() const { return ActiveProfile.Mode; }

private:
    UFUNCTION()
    void HandleVFXAccepted(FName EventId, FVector LocationCm, float Intensity);

    void EnsurePresentationTargets();
    void UpdateMissionReveal();
    void ApplyCameraProfile(float DeltaTime);
    FName ResolveCueForEvent(FName EventId) const;
    FName ResolveFirstPersonActionForEvent(FName EventId) const;
    bool TryParseFirstPersonModeId(FName ModeId, EWMFirstPersonVisualMode& OutMode) const;

    UPROPERTY(Transient)
    TObjectPtr<USpringArmComponent> CameraBoom;

    UPROPERTY(Transient)
    TObjectPtr<UCameraComponent> Camera;

    UPROPERTY(Transient)
    TObjectPtr<UWMPresentationOverlayWidget> Overlay;

    UPROPERTY(Transient)
    TObjectPtr<UWMFirstPersonInteractionComponent> FirstPersonInteraction;

    FWMPresentationCameraProfile ActiveProfile;
    FWMFirstPersonVisualProfile FirstPersonProfile;
    EWMFirstPersonVisualMode FirstPersonMode = EWMFirstPersonVisualMode::Explore;
    float PulseRemainingSeconds = 0.0f;
    FName LastMissionId = NAME_None;
    bool bReducedMotion = false;
    bool bFirstPersonInteractionActive = false;
};
