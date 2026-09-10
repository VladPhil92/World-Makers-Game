#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Visual/WMVisualProfileSettings.h"
#include "WMCaribbeanRainforestPrototype.generated.h"

class UDirectionalLightComponent;
class UHierarchicalInstancedStaticMeshComponent;
class USceneComponent;

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

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "World Makers|Visual")
    EWMVisualQualityTier QualityTier = EWMVisualQualityTier::Mid;

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "World Makers|Visual", meta = (ClampMin = "1200.0", ClampMax = "6000.0"))
    float BiomeRadiusCm = 2400.0f;

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "World Makers|Visual", meta = (ClampMin = "400.0", ClampMax = "1600.0"))
    float BuildClearingRadiusCm = 750.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Visual")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Visual")
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> GroundTiles;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Visual")
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> TerrainMounds;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Visual")
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> TreeTrunks;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Visual")
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> TreeCanopies;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Visual")
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Rocks;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Visual")
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> WaterEdgeMarkers;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Visual")
    TObjectPtr<UDirectionalLightComponent> PrototypeSun;

    static const FName PrototypeBiomeTag;

private:
    void RebuildPrototype();
    FVector RandomRingPoint(FRandomStream& Random, float MinRadius, float MaxRadius) const;
};
