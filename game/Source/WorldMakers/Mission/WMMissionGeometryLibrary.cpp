#include "Mission/WMMissionGeometryLibrary.h"

float UWMMissionGeometryLibrary::MeasureWorldDistanceCm(const FVector& Start, const FVector& End)
{
    return FVector::Dist(Start, End);
}

bool UWMMissionGeometryLibrary::IsWorldPointInsideBox(
    const FVector& WorldPoint,
    const FTransform& BoxTransform,
    const FVector& HalfExtent)
{
    const FVector LocalPoint = BoxTransform.InverseTransformPosition(WorldPoint);
    const FVector SafeExtent = HalfExtent.ComponentMax(FVector::ZeroVector);
    return
        FMath::Abs(LocalPoint.X) <= SafeExtent.X + KINDA_SMALL_NUMBER &&
        FMath::Abs(LocalPoint.Y) <= SafeExtent.Y + KINDA_SMALL_NUMBER &&
        FMath::Abs(LocalPoint.Z) <= SafeExtent.Z + KINDA_SMALL_NUMBER;
}

float UWMMissionGeometryLibrary::CalculateScopedSpanAlongZoneX(
    const TArray<FWMMissionPieceGeometrySample>& Pieces,
    const FTransform& ZoneTransform,
    const FVector& ZoneHalfExtent)
{
    float MinX = TNumericLimits<float>::Max();
    float MaxX = TNumericLimits<float>::Lowest();
    bool bFound = false;
    const float ZoneYaw = ZoneTransform.Rotator().Yaw;

    for (const FWMMissionPieceGeometrySample& Piece : Pieces)
    {
        if (!IsWorldPointInsideBox(Piece.Center, ZoneTransform, ZoneHalfExtent))
        {
            continue;
        }

        const FVector LocalCenter = ZoneTransform.InverseTransformPosition(Piece.Center);
        const float RelativeYawRadians = FMath::DegreesToRadians(Piece.YawDegrees - ZoneYaw);
        const float HalfExtentAlongZoneX = 0.5f * (
            FMath::Abs(FMath::Cos(RelativeYawRadians)) * FMath::Abs(Piece.DimensionsCm.X) +
            FMath::Abs(FMath::Sin(RelativeYawRadians)) * FMath::Abs(Piece.DimensionsCm.Y));

        MinX = FMath::Min(MinX, LocalCenter.X - HalfExtentAlongZoneX);
        MaxX = FMath::Max(MaxX, LocalCenter.X + HalfExtentAlongZoneX);
        bFound = true;
    }

    return bFound ? FMath::Max(0.0f, MaxX - MinX) : 0.0f;
}
