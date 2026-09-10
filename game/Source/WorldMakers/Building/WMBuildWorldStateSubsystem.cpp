#include "Building/WMBuildWorldStateSubsystem.h"

bool FWMPlacedBuildPieceSnapshot::IsSane() const
{
    return !PieceId.IsNone() && !LocationCm.ContainsNaN() &&
        FMath::IsFinite(LocationCm.X) && FMath::IsFinite(LocationCm.Y) && FMath::IsFinite(LocationCm.Z) &&
        FMath::IsFinite(YawDegrees);
}

void UWMBuildWorldStateSubsystem::PublishSnapshot(const TArray<FWMPlacedBuildPieceSnapshot>& InPieces)
{
    PlacedPieces.Reset();
    PlacedPieces.Reserve(InPieces.Num());
    for (const FWMPlacedBuildPieceSnapshot& Piece : InPieces)
    {
        if (Piece.IsSane())
        {
            PlacedPieces.Add(Piece);
        }
    }

    PlacedPieces.Sort([](const FWMPlacedBuildPieceSnapshot& A, const FWMPlacedBuildPieceSnapshot& B)
    {
        const int32 IdCompare = A.PieceId.ToString().Compare(B.PieceId.ToString());
        if (IdCompare != 0) return IdCompare < 0;
        if (!FMath::IsNearlyEqual(A.LocationCm.X, B.LocationCm.X)) return A.LocationCm.X < B.LocationCm.X;
        if (!FMath::IsNearlyEqual(A.LocationCm.Y, B.LocationCm.Y)) return A.LocationCm.Y < B.LocationCm.Y;
        if (!FMath::IsNearlyEqual(A.LocationCm.Z, B.LocationCm.Z)) return A.LocationCm.Z < B.LocationCm.Z;
        return A.YawDegrees < B.YawDegrees;
    });

    Revision = Revision == MAX_int32 ? 1 : Revision + 1;
    OnBuildWorldChanged.Broadcast(Revision);
}
