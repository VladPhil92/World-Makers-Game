#include "Visual/WMVisualProfileSettings.h"

bool FWMAtmosphereLook::IsSane() const
{
    return FMath::IsFinite(SunIntensity) && SunIntensity >= 0.0f && SunIntensity <= 20.0f &&
        FMath::IsFinite(SkyLightIntensity) && SkyLightIntensity >= 0.0f && SkyLightIntensity <= 10.0f &&
        FMath::IsFinite(FogDensity) && FogDensity >= 0.0f && FogDensity <= 0.2f &&
        FMath::IsFinite(FogHeightFalloff) && FogHeightFalloff >= 0.01f && FogHeightFalloff <= 2.0f &&
        FMath::IsFinite(CameraFOVDegrees) && CameraFOVDegrees >= 30.0f && CameraFOVDegrees <= 120.0f;
}

bool FWMVisualBudget::IsSane() const
{
    return TargetFPS >= 30 &&
        TreeClusters >= 0 && TreeClusters <= 128 &&
        RockClusters >= 0 && RockClusters <= 128 &&
        TerrainMounds >= 0 && TerrainMounds <= 32 &&
        WaterMarkers >= 0 && WaterMarkers <= 64 &&
        MaxShadowCastingFoliage >= 0 && MaxShadowCastingFoliage <= TreeClusters;
}

const FWMVisualBudget& UWMVisualProfileSettings::GetBudget(const EWMVisualQualityTier Tier) const
{
    switch (Tier)
    {
    case EWMVisualQualityTier::Low:
        return LowBudget;
    case EWMVisualQualityTier::High:
        return HighBudget;
    case EWMVisualQualityTier::Mid:
    default:
        return MidBudget;
    }
}
