#include "Building/WMBuildGridLibrary.h"

FVector UWMBuildGridLibrary::SnapLocationToGrid(const FVector& Location, const float GridSize, const bool bOffsetHalfCellZ)
{
    const float SafeGrid = FMath::Max(GridSize, 1.0f);
    FVector Result(
        FMath::GridSnap(Location.X, SafeGrid),
        FMath::GridSnap(Location.Y, SafeGrid),
        FMath::GridSnap(Location.Z, SafeGrid));

    if (bOffsetHalfCellZ)
    {
        Result.Z += SafeGrid * 0.5f;
    }

    return Result;
}

float UWMBuildGridLibrary::SnapYawToStep(const float YawDegrees, const float RotationStepDegrees)
{
    const float SafeStep = FMath::Max(RotationStepDegrees, 1.0f);
    return FMath::GridSnap(FRotator::ClampAxis(YawDegrees), SafeStep);
}
