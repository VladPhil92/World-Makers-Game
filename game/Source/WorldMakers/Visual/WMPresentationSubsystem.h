#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Visual/WMPresentationRuntime.h"
#include "WMPresentationSubsystem.generated.h"

class UCameraComponent;
class USpringArmComponent;
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

    UFUNCTION(BlueprintCallable, Category = "World Makers|Presentation")
    bool PulseCameraMode(EWMPresentationCameraMode Mode, float DurationSeconds = 0.8f, FName CueId = NAME_None);

    UFUNCTION(BlueprintCallable, Category = "World Makers|Presentation|Accessibility")
    void SetReducedMotion(bool bEnabled);

    UFUNCTION(BlueprintPure, Category = "World Makers|Presentation|Accessibility")
    bool IsReducedMotion() const { return bReducedMotion; }

    UFUNCTION(BlueprintPure, Category = "World Makers|Presentation")
    EWMPresentationCameraMode GetActiveCameraMode() const { return ActiveProfile.Mode; }

private:
    UFUNCTION()
    void HandleVFXAccepted(FName EventId, FVector LocationCm, float Intensity);

    void EnsurePresentationTargets();
    void UpdateMissionReveal();
    void ApplyCameraProfile(float DeltaTime);
    FName ResolveCueForEvent(FName EventId) const;

    UPROPERTY(Transient)
    TObjectPtr<USpringArmComponent> CameraBoom;

    UPROPERTY(Transient)
    TObjectPtr<UCameraComponent> Camera;

    UPROPERTY(Transient)
    TObjectPtr<UWMPresentationOverlayWidget> Overlay;

    FWMPresentationCameraProfile ActiveProfile;
    float PulseRemainingSeconds = 0.0f;
    FName LastMissionId = NAME_None;
    bool bReducedMotion = false;
};
