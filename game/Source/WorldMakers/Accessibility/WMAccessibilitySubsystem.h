#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "WMAccessibilitySubsystem.generated.h"

USTRUCT(BlueprintType)
struct FWMAccessibilitySettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World Makers|Accessibility")
    bool bSubtitlesEnabled = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World Makers|Accessibility")
    bool bReducedMotion = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World Makers|Accessibility")
    bool bHighContrastUI = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World Makers|Accessibility")
    bool bHoldToInteract = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World Makers|Accessibility")
    float TextScale = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World Makers|Accessibility")
    float CameraShakeScale = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World Makers|Accessibility")
    float UIAnimationScale = 1.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWMAccessibilitySettingsChanged, const FWMAccessibilitySettings&, Settings);

/** Central, asset-independent accessibility state shared by UI, camera and VFX. */
UCLASS()
class WORLDMAKERS_API UWMAccessibilitySubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable, Category="World Makers|Accessibility")
    FWMAccessibilitySettingsChanged OnAccessibilitySettingsChanged;

    UFUNCTION(BlueprintCallable, Category="World Makers|Accessibility")
    void ApplyAccessibilitySettings(const FWMAccessibilitySettings& NewSettings);

    UFUNCTION(BlueprintPure, Category="World Makers|Accessibility")
    FWMAccessibilitySettings GetAccessibilitySettings() const { return Settings; }

    UFUNCTION(BlueprintPure, Category="World Makers|Accessibility")
    float GetEffectiveMotionScale() const;

private:
    UPROPERTY()
    FWMAccessibilitySettings Settings;
};
