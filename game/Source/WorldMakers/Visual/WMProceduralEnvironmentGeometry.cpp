#include "Visual/WMProceduralEnvironmentGeometry.h"

void FWMEnvironmentMeshData::Reset()
{
    Vertices.Reset();
    Triangles.Reset();
    Normals.Reset();
    UV0.Reset();
    VertexColors.Reset();
    Tangents.Reset();
}

bool FWMEnvironmentMeshData::IsSane() const
{
    if (Vertices.IsEmpty() || Triangles.IsEmpty() || (Triangles.Num() % 3) != 0)
    {
        return false;
    }

    const int32 VertexCount = Vertices.Num();
    if (Normals.Num() != VertexCount || UV0.Num() != VertexCount || VertexColors.Num() != VertexCount || Tangents.Num() != VertexCount)
    {
        return false;
    }

    for (const FVector& Vertex : Vertices)
    {
        if (Vertex.ContainsNaN())
        {
            return false;
        }
    }

    for (const FVector& Normal : Normals)
    {
        if (Normal.ContainsNaN() || Normal.IsNearlyZero())
        {
            return false;
        }
    }

    for (const int32 Index : Triangles)
    {
        if (Index < 0 || Index >= VertexCount)
        {
            return false;
        }
    }

    return true;
}

void FWMProceduralEnvironmentGeometry::AppendTriangle(
    FWMEnvironmentMeshData& Mesh,
    const FVector& A,
    const FVector& B,
    const FVector& C)
{
    const FVector EdgeAB = B - A;
    const FVector Normal = FVector::CrossProduct(EdgeAB, C - A).GetSafeNormal();
    if (Normal.IsNearlyZero())
    {
        return;
    }

    const int32 BaseIndex = Mesh.Vertices.Num();
    Mesh.Vertices.Append({A, B, C});
    Mesh.Triangles.Append({BaseIndex, BaseIndex + 1, BaseIndex + 2});
    Mesh.Normals.Append({Normal, Normal, Normal});
    Mesh.UV0.Append({FVector2D(0.0f, 0.0f), FVector2D(1.0f, 0.0f), FVector2D(0.5f, 1.0f)});
    Mesh.VertexColors.Append({FLinearColor::White, FLinearColor::White, FLinearColor::White});

    const FVector TangentDirection = EdgeAB.GetSafeNormal();
    const FProcMeshTangent Tangent(TangentDirection.IsNearlyZero() ? FVector::ForwardVector : TangentDirection, false);
    Mesh.Tangents.Append({Tangent, Tangent, Tangent});
}

void FWMProceduralEnvironmentGeometry::AppendQuad(
    FWMEnvironmentMeshData& Mesh,
    const FVector& A,
    const FVector& B,
    const FVector& C,
    const FVector& D)
{
    AppendTriangle(Mesh, A, B, C);
    AppendTriangle(Mesh, A, C, D);
}

void FWMProceduralEnvironmentGeometry::AppendIrregularGroundDisc(
    FWMEnvironmentMeshData& Mesh,
    const FVector& Center,
    const float RadiusCm,
    const int32 Segments,
    FRandomStream& Random)
{
    if (RadiusCm <= 0.0f || Segments < 6)
    {
        return;
    }

    TArray<FVector> Rim;
    Rim.Reserve(Segments);
    for (int32 Index = 0; Index < Segments; ++Index)
    {
        const float Angle = 2.0f * PI * static_cast<float>(Index) / static_cast<float>(Segments);
        const float RadiusScale = Random.FRandRange(0.91f, 1.04f);
        Rim.Add(Center + FVector(FMath::Cos(Angle) * RadiusCm * RadiusScale, FMath::Sin(Angle) * RadiusCm * RadiusScale, Random.FRandRange(-4.0f, 4.0f)));
    }

    for (int32 Index = 0; Index < Segments; ++Index)
    {
        AppendTriangle(Mesh, Center, Rim[Index], Rim[(Index + 1) % Segments]);
    }
}

void FWMProceduralEnvironmentGeometry::AppendTaperedTrunk(
    FWMEnvironmentMeshData& Mesh,
    const FVector& BaseCenter,
    const FVector& TopCenter,
    const float BaseRadiusCm,
    const float TopRadiusCm,
    const int32 Sides,
    const float YawDegrees)
{
    if (BaseRadiusCm <= 0.0f || TopRadiusCm <= 0.0f || Sides < 3 || BaseCenter.Equals(TopCenter))
    {
        return;
    }

    const float YawRadians = FMath::DegreesToRadians(YawDegrees);
    TArray<FVector> BaseRing;
    TArray<FVector> TopRing;
    BaseRing.Reserve(Sides);
    TopRing.Reserve(Sides);

    for (int32 SideIndex = 0; SideIndex < Sides; ++SideIndex)
    {
        const float Angle = YawRadians + 2.0f * PI * static_cast<float>(SideIndex) / static_cast<float>(Sides);
        const FVector Radial(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f);
        BaseRing.Add(BaseCenter + Radial * BaseRadiusCm);
        TopRing.Add(TopCenter + Radial * TopRadiusCm);
    }

    for (int32 SideIndex = 0; SideIndex < Sides; ++SideIndex)
    {
        const int32 Next = (SideIndex + 1) % Sides;
        AppendQuad(Mesh, BaseRing[SideIndex], BaseRing[Next], TopRing[Next], TopRing[SideIndex]);
        AppendTriangle(Mesh, BaseCenter, BaseRing[Next], BaseRing[SideIndex]);
        AppendTriangle(Mesh, TopCenter, TopRing[SideIndex], TopRing[Next]);
    }
}

void FWMProceduralEnvironmentGeometry::AppendFacetedEllipsoid(
    FWMEnvironmentMeshData& Mesh,
    const FVector& Center,
    const FVector& RadiiCm,
    const int32 Segments,
    const float YawDegrees)
{
    if (RadiiCm.X <= 0.0f || RadiiCm.Y <= 0.0f || RadiiCm.Z <= 0.0f || Segments < 4)
    {
        return;
    }

    const float YawRadians = FMath::DegreesToRadians(YawDegrees);
    const FVector Bottom = Center - FVector(0.0f, 0.0f, RadiiCm.Z);
    const FVector Top = Center + FVector(0.0f, 0.0f, RadiiCm.Z);
    TArray<FVector> LowerRing;
    TArray<FVector> UpperRing;
    LowerRing.Reserve(Segments);
    UpperRing.Reserve(Segments);

    for (int32 Index = 0; Index < Segments; ++Index)
    {
        const float Angle = YawRadians + 2.0f * PI * static_cast<float>(Index) / static_cast<float>(Segments);
        const FVector Radial(FMath::Cos(Angle) * RadiiCm.X, FMath::Sin(Angle) * RadiiCm.Y, 0.0f);
        LowerRing.Add(Center + Radial * 0.82f - FVector(0.0f, 0.0f, RadiiCm.Z * 0.35f));
        UpperRing.Add(Center + Radial * 0.94f + FVector(0.0f, 0.0f, RadiiCm.Z * 0.30f));
    }

    for (int32 Index = 0; Index < Segments; ++Index)
    {
        const int32 Next = (Index + 1) % Segments;
        AppendTriangle(Mesh, Bottom, LowerRing[Next], LowerRing[Index]);
        AppendQuad(Mesh, LowerRing[Index], LowerRing[Next], UpperRing[Next], UpperRing[Index]);
        AppendTriangle(Mesh, Top, UpperRing[Index], UpperRing[Next]);
    }
}

void FWMProceduralEnvironmentGeometry::AppendButtressRoot(
    FWMEnvironmentMeshData& Mesh,
    const FVector& TrunkBase,
    const FVector& OutwardDirection,
    const float LengthCm,
    const float WidthCm,
    const float HeightCm)
{
    FVector Direction = OutwardDirection;
    Direction.Z = 0.0f;
    Direction = Direction.GetSafeNormal();
    if (Direction.IsNearlyZero() || LengthCm <= 0.0f || WidthCm <= 0.0f || HeightCm <= 0.0f)
    {
        return;
    }

    const FVector Side(-Direction.Y, Direction.X, 0.0f);
    const FVector BaseLeft = TrunkBase - Side * WidthCm * 0.5f;
    const FVector BaseRight = TrunkBase + Side * WidthCm * 0.5f;
    const FVector BaseTop = TrunkBase + FVector(0.0f, 0.0f, HeightCm);
    const FVector FarCenter = TrunkBase + Direction * LengthCm;
    const FVector FarLeft = FarCenter - Side * WidthCm * 0.08f;
    const FVector FarRight = FarCenter + Side * WidthCm * 0.08f;
    const FVector FarTop = FarCenter + FVector(0.0f, 0.0f, HeightCm * 0.08f);

    AppendTriangle(Mesh, BaseLeft, BaseRight, BaseTop);
    AppendTriangle(Mesh, FarRight, FarLeft, FarTop);
    AppendQuad(Mesh, BaseLeft, FarLeft, FarRight, BaseRight);
    AppendQuad(Mesh, BaseTop, BaseRight, FarRight, FarTop);
    AppendQuad(Mesh, BaseLeft, BaseTop, FarTop, FarLeft);
}

void FWMProceduralEnvironmentGeometry::AppendLeafCluster(
    FWMEnvironmentMeshData& Mesh,
    const FVector& Center,
    const float RadiusCm,
    const int32 LeafCount,
    const float YawDegrees)
{
    if (RadiusCm <= 0.0f || LeafCount < 3)
    {
        return;
    }

    const float BaseYaw = FMath::DegreesToRadians(YawDegrees);
    for (int32 Index = 0; Index < LeafCount; ++Index)
    {
        const float Angle = BaseYaw + 2.0f * PI * static_cast<float>(Index) / static_cast<float>(LeafCount);
        const FVector Direction(FMath::Cos(Angle), FMath::Sin(Angle), 0.16f + 0.08f * FMath::Sin(Angle * 2.0f));
        const FVector FlatDirection(Direction.X, Direction.Y, 0.0f);
        const FVector Side(-FlatDirection.Y, FlatDirection.X, 0.0f);
        const FVector Base = Center + FVector(0.0f, 0.0f, RadiusCm * 0.08f);
        const FVector Tip = Center + Direction * RadiusCm;
        const FVector Left = Center + Direction * RadiusCm * 0.45f - Side * RadiusCm * 0.18f;
        const FVector Right = Center + Direction * RadiusCm * 0.45f + Side * RadiusCm * 0.18f;
        AppendQuad(Mesh, Base, Left, Tip, Right);
    }
}

void FWMProceduralEnvironmentGeometry::AppendSinuousRiverStrip(
    FWMEnvironmentMeshData& Mesh,
    const float HalfLengthCm,
    const float CenterY,
    const float Z,
    const float HalfWidthCm,
    const int32 Segments,
    const float WaveAmplitudeCm,
    const float WaveCycles)
{
    if (HalfLengthCm <= 0.0f || HalfWidthCm <= 0.0f || Segments < 2 || WaveCycles <= 0.0f)
    {
        return;
    }

    TArray<FVector> LeftEdge;
    TArray<FVector> RightEdge;
    LeftEdge.Reserve(Segments + 1);
    RightEdge.Reserve(Segments + 1);

    for (int32 Index = 0; Index <= Segments; ++Index)
    {
        const float Alpha = static_cast<float>(Index) / static_cast<float>(Segments);
        const float X = FMath::Lerp(-HalfLengthCm, HalfLengthCm, Alpha);
        const float Phase = Alpha * WaveCycles * 2.0f * PI;
        const float Y = CenterY + FMath::Sin(Phase) * WaveAmplitudeCm;
        const float Dx = 2.0f * HalfLengthCm / static_cast<float>(Segments);
        const float Dy = FMath::Cos(Phase) * WaveAmplitudeCm * WaveCycles * 2.0f * PI / (2.0f * HalfLengthCm) * Dx;
        const FVector Tangent(Dx, Dy, 0.0f);
        const FVector Side(-Tangent.Y, Tangent.X, 0.0f);
        const FVector SideNormal = Side.GetSafeNormal();
        const FVector CenterPoint(X, Y, Z);
        LeftEdge.Add(CenterPoint - SideNormal * HalfWidthCm);
        RightEdge.Add(CenterPoint + SideNormal * HalfWidthCm);
    }

    for (int32 Index = 0; Index < Segments; ++Index)
    {
        AppendQuad(Mesh, LeftEdge[Index], LeftEdge[Index + 1], RightEdge[Index + 1], RightEdge[Index]);
    }
}
