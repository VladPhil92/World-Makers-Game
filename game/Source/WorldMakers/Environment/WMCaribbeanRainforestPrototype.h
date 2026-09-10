#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Visual/WMVisualProfileSettings.h"
#include "WMCaribbeanRainforestPrototype.generated.h"

class UDirectionalLightComponent;
class UExponentialHeightFogComponent;
class UHierarchicalInstancedStaticMeshComponent;
class UProceduralMeshComponent;
class USceneComponent;
class USkyAtmosphereComponent;
class USkyLightComponent;

UCLASS(Config = Game)
class WORLDMAKERS_API AWMCaribbeanRainforestPrototype : public AActor
{
    GENERATED_BODY()

public:
    AWMCaribbeanRainforestPrototype();
    virtual void OnConstruction(const FTransform& Transform) override;

    /** Rebuild the authored prototype from the current visual profile budget. Safe for performance-tier changes. */
    UFUNCTION(BlueprintCallable, Category = "World Makers|Visual")
    void RefreshFromVisualProfile();

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "World Makers|Visual")
    int32 Seed = 17062026;

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "World Makers|Visual")
    bool bUseProfileDefaultQuality = true;

    /** V3 default render path. Legacy Engine primitives remain available as collision/fallback proxies. */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "World Makers|Visual")
    bool bUseProceduralEnvironmentArt = true;

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "World Makers|Visual")
    EWMVisualQualityTier QualityTier = EWMVisualQualityTier::Mid;

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "World Makers|Visual", meta = (ClampMin = "1200.0", ClampMax = "6000.0"))
    float BiomeRadiusCm = 2400.0f;

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "World Makers|Visual", meta = (ClampMin = "400.0", ClampMax = "1600.0"))
    float BuildClearingRadiusCm = 750.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Visual")
    TObjectPtr<USceneComponent> SceneRoot;

    // Legacy collision/fallback proxies. Hidden while the V3 procedural art path renders the scene.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Visual|Fallback")
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> GroundTiles;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Visual|Fallback")
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> TerrainMounds;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Visual|Fallback")
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> TreeTrunks;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Visual|Fallback")
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> TreeCanopies;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Visual|Fallback")
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Rocks;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Visual|Fallback")
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> WaterEdgeMarkers;

    // V3 render-only procedural art families.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Visual|Environment Art")
    TObjectPtr<UProceduralMeshComponent> GroundArt;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Visual|Environment Art")
    TObjectPtr<UProceduralMeshComponent> TerrainArt;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Visual|Environment Art")
    TObjectPtr<UProceduralMeshComponent> BarkAndRootsArt;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Visual|Environment Art")
    TObjectPtr<UProceduralMeshComponent> FoliageArt;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Visual|Environment Art")
    TObjectPtr<UProceduralMeshComponent> StoneArt;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Visual|Environment Art")
    TObjectPtr<UProceduralMeshComponent> WaterArt;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Visual")
    TObjectPtr<UDirectionalLightComponent> PrototypeSun;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Visual")
    TObjectPtr<USkyLightComponent> PrototypeSkyLight;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Visual")
    TObjectPtr<USkyAtmosphereComponent> PrototypeSkyAtmosphere;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Visual")
    TObjectPtr<UExponentialHeightFogComponent> PrototypeHeightFog;

    static const FName PrototypeBiomeTag;

private:
    void RebuildPrototype();
    void ApplyLookDevelopmentProfile();
    void ApplySurfaceLanguage();
    void SetEnvironmentArtPathEnabled(bool bEnabled);
    FVector RandomRingPoint(FRandomStream& Random, float MinRadius, float MaxRadius) const;
};
