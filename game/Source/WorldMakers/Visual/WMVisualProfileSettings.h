#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "WMVisualProfileSettings.generated.h"

UENUM(BlueprintType)
enum class EWMVisualQualityTier : uint8
{
    Low,
    Mid,
    High
};

UENUM(BlueprintType)
enum class EWMStylizedSurfaceRole : uint8
{
    GroundEarth,
    Terrain,
    Bark,
    Foliage,
    Stone,
    Water,
    BuildNeutral,
    BuildEco,
    PreviewValid,
    PreviewInvalid,
    MagicalAccent
};

USTRUCT(BlueprintType)
struct FWMVisualPalette
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FLinearColor CanopyDeep = FLinearColor(0.094f, 0.310f, 0.227f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FLinearColor LeafBright = FLinearColor(0.243f, 0.541f, 0.341f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FLinearColor TrunkWarm = FLinearColor(0.502f, 0.310f, 0.180f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FLinearColor Earth = FLinearColor(0.718f, 0.431f, 0.294f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FLinearColor Water = FLinearColor(0.184f, 0.561f, 0.616f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FLinearColor Stone = FLinearColor(0.435f, 0.482f, 0.447f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FLinearColor Sky = FLinearColor(0.545f, 0.780f, 0.847f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FLinearColor Sunlight = FLinearColor(1.0f, 0.890f, 0.690f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FLinearColor BuildNeutral = FLinearColor(0.745f, 0.604f, 0.420f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FLinearColor BuildEco = FLinearColor(0.357f, 0.620f, 0.369f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FLinearColor PreviewValid = FLinearColor(0.388f, 0.831f, 0.773f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FLinearColor PreviewInvalid = FLinearColor(0.941f, 0.643f, 0.235f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FLinearColor MagicalAccent = FLinearColor(0.576f, 0.443f, 0.878f, 1.0f);
};

/**
 * Source-controlled look-development controls that can be applied even while the scene still uses proxy meshes.
 * Values are intentionally renderer-agnostic and conservative enough to scale down for tablet profiles.
 */
USTRUCT(BlueprintType)
struct FWMAtmosphereLook
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "20.0"))
    float SunIntensity = 4.5f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "10.0"))
    float SkyLightIntensity = 0.85f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "0.2"))
    float FogDensity = 0.012f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.01", ClampMax = "2.0"))
    float FogHeightFalloff = 0.22f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "30.0", ClampMax = "120.0"))
    float CameraFOVDegrees = 72.0f;

    bool IsSane() const;
};

/** Material response shared by the proxy fallback and future authored World Makers master materials. */
USTRUCT(BlueprintType)
struct FWMStylizedSurfaceResponse
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float GroundRoughness = 0.88f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float TerrainRoughness = 0.82f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float BarkRoughness = 0.78f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float FoliageRoughness = 0.68f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float StoneRoughness = 0.74f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float WaterRoughness = 0.18f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float BuildRoughness = 0.62f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "4.0"))
    float PreviewEmissiveStrength = 0.70f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float FoliageWindResponse = 0.72f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float WaterOpacityIntent = 0.72f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "4.0"))
    float MagicalEmissiveStrength = 1.35f;

    bool IsSane() const;
};

USTRUCT(BlueprintType)
struct FWMStylizedSurfaceLook
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FLinearColor BaseColor = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Roughness = 0.7f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Metallic = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "4.0"))
    float EmissiveStrength = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float WindResponse = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float OpacityIntent = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bTwoSidedIntent = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bTranslucentIntent = false;

    bool IsSane() const;
};

USTRUCT(BlueprintType)
struct FWMVisualBudget
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 TargetFPS = 30;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 TreeClusters = 24;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 RockClusters = 12;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 TerrainMounds = 6;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 WaterMarkers = 8;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 MaxShadowCastingFoliage = 8;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 MaxMaterialSlotsPerMesh = 2;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 MaxTextureEdgePx = 2048;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 MaxSampledTexturesPerMaterial = 8;

    bool IsSane() const;
};

UCLASS(Config = Game, DefaultConfig)
class WORLDMAKERS_API UWMVisualProfileSettings : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "World Makers|Visual")
    FString ProfileName = TEXT("CaribbeanRainforestPrototype");

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "World Makers|Visual")
    EWMVisualQualityTier DefaultQualityTier = EWMVisualQualityTier::Mid;

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "World Makers|Visual")
    FWMVisualPalette Palette;

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "World Makers|Visual")
    FWMAtmosphereLook Atmosphere;

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "World Makers|Visual")
    FWMStylizedSurfaceResponse SurfaceResponse;

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "World Makers|Visual")
    FWMVisualBudget LowBudget;

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "World Makers|Visual")
    FWMVisualBudget MidBudget;

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "World Makers|Visual")
    FWMVisualBudget HighBudget;

    const FWMVisualBudget& GetBudget(EWMVisualQualityTier Tier) const;
};
