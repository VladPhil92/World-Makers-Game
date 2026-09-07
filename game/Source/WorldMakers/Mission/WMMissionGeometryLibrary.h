#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WMMissionGeometryLibrary.generated.h"

USTRUCT(BlueprintType)
struct WORLDMAKERS_API FWMMissionPieceGeometrySample
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Makers|Missions|Geometry")
    FVector Center = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Makers|Missions|Geometry")
    FVector DimensionsCm = FVector(100.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Makers|Missions|Geometry")
    float YawDegrees = 0.0f;
};

UCLASS()
class WORLDMAKERS_API UWMMissionGeometryLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "World Makers|Missions|Geometry")
    static float MeasureWorldDistanceCm(const FVector& Start, const FVector& End);

    UFUNCTION(BlueprintPure, Category = "World Makers|Missions|Geometry")
    static bool IsWorldPointInsideBox(const FVector& WorldPoint, const FTransform& BoxTransform, const FVector& HalfExtent);

    UFUNCTION(BlueprintPure, Category = "World Makers|Missions|Geometry")
    static float CalculateScopedSpanAlongZoneX(
        const TArray<FWMMissionPieceGeometrySample>& Pieces,
        const FTransform& ZoneTransform,
        const FVector& ZoneHalfExtent);
};
