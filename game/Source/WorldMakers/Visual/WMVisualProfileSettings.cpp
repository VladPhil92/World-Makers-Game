#include "Visual/WMVisualProfileSettings.h"

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
