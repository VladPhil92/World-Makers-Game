#include "Building/WMBuildCatalogSettings.h"

bool FWMBuildPieceSpec::IsSane() const
{
    const bool bDimensionsFinite =
        FMath::IsFinite(DimensionsCm.X) && FMath::IsFinite(DimensionsCm.Y) && FMath::IsFinite(DimensionsCm.Z);
    const bool bDimensionsPositive =
        DimensionsCm.X >= 10.0f && DimensionsCm.Y >= 10.0f && DimensionsCm.Z >= 10.0f &&
        DimensionsCm.X <= 2000.0f && DimensionsCm.Y <= 2000.0f && DimensionsCm.Z <= 2000.0f;

    return !PieceId.IsNone() &&
        bDimensionsFinite &&
        bDimensionsPositive &&
        RotationStepDegrees >= 15.0f &&
        RotationStepDegrees <= 180.0f &&
        MinSurfaceUpDot >= 0.0f &&
        MinSurfaceUpDot <= 1.0f;
}

bool UWMBuildCatalogSettings::FindPieceSpec(const FName PieceId, FWMBuildPieceSpec& OutSpec) const
{
    for (const FWMBuildPieceSpec& Spec : Pieces)
    {
        if (Spec.PieceId == PieceId && Spec.IsSane())
        {
            OutSpec = Spec;
            return true;
        }
    }

    return false;
}

void UWMBuildCatalogSettings::GetPieceIds(TArray<FName>& OutPieceIds) const
{
    OutPieceIds.Reset();
    for (const FWMBuildPieceSpec& Spec : Pieces)
    {
        if (Spec.IsSane() && !OutPieceIds.Contains(Spec.PieceId))
        {
            OutPieceIds.Add(Spec.PieceId);
        }
    }
}
