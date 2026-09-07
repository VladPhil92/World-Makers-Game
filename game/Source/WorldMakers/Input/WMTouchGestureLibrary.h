#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WMTouchGestureLibrary.generated.h"

UCLASS()
class WORLDMAKERS_API UWMTouchGestureLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "World Makers|Input")
    static bool HasExceededDragDeadZone(const FVector2D& StartPosition, const FVector2D& CurrentPosition, float DeadZonePixels);
};
