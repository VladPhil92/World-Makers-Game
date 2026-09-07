#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WMBuildGridLibrary.generated.h"

UCLASS()
class WORLDMAKERS_API UWMBuildGridLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "World Makers|Building")
    static FVector SnapLocationToGrid(const FVector& Location, float GridSize, bool bOffsetHalfCellZ = true);

    UFUNCTION(BlueprintPure, Category = "World Makers|Building")
    static float SnapYawToStep(float YawDegrees, float RotationStepDegrees);
};
