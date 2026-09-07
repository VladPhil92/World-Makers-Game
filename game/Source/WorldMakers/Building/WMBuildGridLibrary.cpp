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

FVector UWMBuildGridLibrary::SnapLocationToSurfaceGrid(
    const FVector& SurfaceImpactPoint,
    const float GridSize,
    const float PieceHeightCm)
{
    const float SafeGrid = FMath::Max(GridSize, 1.0f);
    const float SafeHeight = FMath::Max(PieceHeightCm, 1.0f);
    return FVector(
        FMath::GridSnap(SurfaceImpactPoint.X, SafeGrid),
        FMath::GridSnap(SurfaceImpactPoint.Y, SafeGrid),
        SurfaceImpactPoint.Z + (SafeHeight * 0.5f));
}

float UWMBuildGridLibrary::SnapYawToStep(const float YawDegrees, const float RotationStepDegrees)
{
    const float SafeStep = FMath::Max(RotationStepDegrees, 1.0f);
    return FMath::GridSnap(FRotator::ClampAxis(YawDegrees), SafeStep);
}
