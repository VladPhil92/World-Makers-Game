#include "Accessibility/WMAccessibilitySubsystem.h"

void UWMAccessibilitySubsystem::ApplyAccessibilitySettings(const FWMAccessibilitySettings& NewSettings)
{
    Settings = NewSettings;
    Settings.TextScale = FMath::Clamp(Settings.TextScale, 0.8f, 1.6f);
    Settings.CameraShakeScale = FMath::Clamp(Settings.CameraShakeScale, 0.0f, 1.0f);
    Settings.UIAnimationScale = FMath::Clamp(Settings.UIAnimationScale, 0.0f, 1.0f);

    if (Settings.bReducedMotion)
    {
        Settings.CameraShakeScale = 0.0f;
        Settings.UIAnimationScale = FMath::Min(Settings.UIAnimationScale, 0.25f);
    }

    OnAccessibilitySettingsChanged.Broadcast(Settings);
}

float UWMAccessibilitySubsystem::GetEffectiveMotionScale() const
{
    return Settings.bReducedMotion ? 0.0f : Settings.UIAnimationScale;
}
