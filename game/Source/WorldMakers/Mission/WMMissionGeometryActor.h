#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WMMissionGeometryActor.generated.h"

class UBoxComponent;
class USceneComponent;
class UStaticMeshComponent;

UCLASS()
class WORLDMAKERS_API AWMMissionGeometryActor : public AActor
{
    GENERATED_BODY()

public:
    AWMMissionGeometryActor();

    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    static const FName PrototypeMissionGeometryTag;

    UFUNCTION(BlueprintPure, Category = "World Makers|Missions|Geometry")
    FVector GetMeasureStartWorld() const;

    UFUNCTION(BlueprintPure, Category = "World Makers|Missions|Geometry")
    FVector GetMeasureEndWorld() const;

    UFUNCTION(BlueprintPure, Category = "World Makers|Missions|Geometry")
    float GetMeasuredSpanCm() const;

    UFUNCTION(BlueprintPure, Category = "World Makers|Missions|Geometry")
    FTransform GetBuildZoneTransform() const;

    UFUNCTION(BlueprintPure, Category = "World Makers|Missions|Geometry")
    FVector GetBuildZoneHalfExtent() const;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Makers|Missions|Geometry", meta = (ClampMin = "1.0"))
    float AnchorSpanCm = 300.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Makers|Missions|Geometry")
    FVector BuildZoneHalfExtent = FVector(250.0f, 300.0f, 250.0f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Makers|Missions|Geometry")
    float AnchorHeightCm = 60.0f;

private:
    void ApplyPrototypeLayout();

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UBoxComponent> BuildZone;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> MeasureStart;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> MeasureEnd;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> MeasureStartMarker;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> MeasureEndMarker;
};
