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
    FWMVisualBudget LowBudget;

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "World Makers|Visual")
    FWMVisualBudget MidBudget;

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "World Makers|Visual")
    FWMVisualBudget HighBudget;

    const FWMVisualBudget& GetBudget(EWMVisualQualityTier Tier) const;
};
