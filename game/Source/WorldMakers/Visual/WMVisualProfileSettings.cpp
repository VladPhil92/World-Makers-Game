#include "Visual/WMVisualProfileSettings.h"

bool FWMAtmosphereLook::IsSane() const
{
    return FMath::IsFinite(SunIntensity) && SunIntensity >= 0.0f && SunIntensity <= 20.0f &&
        FMath::IsFinite(SkyLightIntensity) && SkyLightIntensity >= 0.0f && SkyLightIntensity <= 10.0f &&
        FMath::IsFinite(FogDensity) && FogDensity >= 0.0f && FogDensity <= 0.2f &&
        FMath::IsFinite(FogHeightFalloff) && FogHeightFalloff >= 0.01f && FogHeightFalloff <= 2.0f &&
        FMath::IsFinite(CameraFOVDegrees) && CameraFOVDegrees >= 30.0f && CameraFOVDegrees <= 120.0f;
}

bool FWMStylizedSurfaceResponse::IsSane() const
{
    const auto InUnitRange = [](const float Value)
    {
        return FMath::IsFinite(Value) && Value >= 0.0f && Value <= 1.0f;
    };

    return InUnitRange(GroundRoughness) &&
        InUnitRange(TerrainRoughness) &&
        InUnitRange(BarkRoughness) &&
        InUnitRange(FoliageRoughness) &&
        InUnitRange(StoneRoughness) &&
        InUnitRange(WaterRoughness) &&
        InUnitRange(BuildRoughness) &&
        FMath::IsFinite(PreviewEmissiveStrength) && PreviewEmissiveStrength >= 0.0f && PreviewEmissiveStrength <= 4.0f &&
        InUnitRange(FoliageWindResponse) &&
        InUnitRange(WaterOpacityIntent) &&
        FMath::IsFinite(MagicalEmissiveStrength) && MagicalEmissiveStrength >= 0.0f && MagicalEmissiveStrength <= 4.0f;
}

bool FWMStylizedSurfaceLook::IsSane() const
{
    return BaseColor.R >= 0.0f && BaseColor.R <= 1.0f &&
        BaseColor.G >= 0.0f && BaseColor.G <= 1.0f &&
        BaseColor.B >= 0.0f && BaseColor.B <= 1.0f &&
        BaseColor.A >= 0.0f && BaseColor.A <= 1.0f &&
        FMath::IsFinite(Roughness) && Roughness >= 0.0f && Roughness <= 1.0f &&
        FMath::IsFinite(Metallic) && Metallic >= 0.0f && Metallic <= 1.0f &&
        FMath::IsFinite(EmissiveStrength) && EmissiveStrength >= 0.0f && EmissiveStrength <= 4.0f &&
        FMath::IsFinite(WindResponse) && WindResponse >= 0.0f && WindResponse <= 1.0f &&
        FMath::IsFinite(OpacityIntent) && OpacityIntent >= 0.0f && OpacityIntent <= 1.0f;
}

bool FWMVisualBudget::IsSane() const
{
    return TargetFPS >= 30 &&
        TreeClusters >= 0 && TreeClusters <= 128 &&
        RockClusters >= 0 && RockClusters <= 128 &&
        TerrainMounds >= 0 && TerrainMounds <= 32 &&
        WaterMarkers >= 0 && WaterMarkers <= 64 &&
        MaxShadowCastingFoliage >= 0 && MaxShadowCastingFoliage <= TreeClusters &&
        MaxMaterialSlotsPerMesh >= 1 && MaxMaterialSlotsPerMesh <= 4 &&
        (MaxTextureEdgePx == 512 || MaxTextureEdgePx == 1024 || MaxTextureEdgePx == 2048 || MaxTextureEdgePx == 4096) &&
        MaxSampledTexturesPerMaterial >= 1 && MaxSampledTexturesPerMaterial <= 16;
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
