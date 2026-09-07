#include "Input/WMTouchGestureLibrary.h"

bool UWMTouchGestureLibrary::HasExceededDragDeadZone(
    const FVector2D& StartPosition,
    const FVector2D& CurrentPosition,
    const float DeadZonePixels)
{
    const float SafeDeadZone = FMath::Max(DeadZonePixels, 1.0f);
    return FVector2D::DistSquared(StartPosition, CurrentPosition) >= FMath::Square(SafeDeadZone);
}
