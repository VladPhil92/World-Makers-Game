#include "Visual/WMProceduralVFXGeometry.h"

namespace
{
    void AddVertex(FWMVFXMeshData& Mesh, const FVector& Position, const FVector2D& UV)
    {
        Mesh.Vertices.Add(Position);
        Mesh.Normals.Add(FVector::UpVector);
        Mesh.UV0.Add(UV);
        Mesh.VertexColors.Add(FLinearColor::White);
        Mesh.Tangents.Add(FProcMeshTangent(FVector::ForwardVector, false));
    }

    void AddTri(FWMVFXMeshData& Mesh, const int32 A, const int32 B, const int32 C)
    {
        Mesh.Triangles.Add(A);
        Mesh.Triangles.Add(B);
        Mesh.Triangles.Add(C);
    }
}

void FWMVFXMeshData::Reset()
{
    Vertices.Reset();
    Triangles.Reset();
    Normals.Reset();
    UV0.Reset();
    VertexColors.Reset();
    Tangents.Reset();
}

bool FWMVFXMeshData::IsSane() const
{
    return Vertices.Num() >= 3 && Triangles.Num() >= 3 && Triangles.Num() % 3 == 0 &&
        Normals.Num() == Vertices.Num() && UV0.Num() == Vertices.Num() &&
        VertexColors.Num() == Vertices.Num() && Tangents.Num() == Vertices.Num();
}

void FWMProceduralVFXGeometry::BuildRing(FWMVFXMeshData& OutMesh, const int32 Segments, const float InnerRadius, const float OuterRadius)
{
    OutMesh.Reset();
    const int32 SafeSegments = FMath::Clamp(Segments, 6, 64);
    const float Inner = FMath::Clamp(InnerRadius, 0.1f, 0.95f);
    const float Outer = FMath::Max(OuterRadius, Inner + 0.05f);

    for (int32 Index = 0; Index < SafeSegments; ++Index)
    {
        const float T = static_cast<float>(Index) / static_cast<float>(SafeSegments);
        const float Angle = T * 2.0f * PI;
        const FVector Direction(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f);
        AddVertex(OutMesh, Direction * Inner, FVector2D(T, 0.0f));
        AddVertex(OutMesh, Direction * Outer, FVector2D(T, 1.0f));
    }

    for (int32 Index = 0; Index < SafeSegments; ++Index)
    {
        const int32 Next = (Index + 1) % SafeSegments;
        const int32 I0 = Index * 2;
        const int32 O0 = I0 + 1;
        const int32 I1 = Next * 2;
        const int32 O1 = I1 + 1;
        AddTri(OutMesh, I0, O0, O1);
        AddTri(OutMesh, I0, O1, I1);
    }
}

void FWMProceduralVFXGeometry::BuildBurst(FWMVFXMeshData& OutMesh, const int32 Rays, const float InnerRadius, const float OuterRadius)
{
    OutMesh.Reset();
    const int32 SafeRays = FMath::Clamp(Rays, 4, 16);
    const float Inner = FMath::Clamp(InnerRadius, 0.05f, 0.75f);
    const float Outer = FMath::Max(OuterRadius, Inner + 0.1f);

    AddVertex(OutMesh, FVector::ZeroVector, FVector2D(0.5f, 0.5f));
    for (int32 Index = 0; Index < SafeRays * 2; ++Index)
    {
        const float T = static_cast<float>(Index) / static_cast<float>(SafeRays * 2);
        const float Angle = T * 2.0f * PI;
        const float Radius = (Index % 2 == 0) ? Outer : Inner;
        AddVertex(OutMesh, FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f) * Radius, FVector2D(T, Radius));
    }

    for (int32 Index = 0; Index < SafeRays * 2; ++Index)
    {
        const int32 Current = 1 + Index;
        const int32 Next = 1 + ((Index + 1) % (SafeRays * 2));
        AddTri(OutMesh, 0, Current, Next);
    }
}

void FWMProceduralVFXGeometry::BuildDirectionalChevron(FWMVFXMeshData& OutMesh)
{
    OutMesh.Reset();
    const FVector Points[] = {
        FVector(-1.0f, -0.28f, 0.0f),
        FVector(0.15f, -0.28f, 0.0f),
        FVector(0.15f, -0.58f, 0.0f),
        FVector(1.0f, 0.0f, 0.0f),
        FVector(0.15f, 0.58f, 0.0f),
        FVector(0.15f, 0.28f, 0.0f),
        FVector(-1.0f, 0.28f, 0.0f)
    };

    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Points); ++Index)
    {
        AddVertex(OutMesh, Points[Index], FVector2D((Points[Index].X + 1.0f) * 0.5f, (Points[Index].Y + 0.6f) / 1.2f));
    }

    AddTri(OutMesh, 0, 1, 6);
    AddTri(OutMesh, 1, 5, 6);
    AddTri(OutMesh, 1, 2, 3);
    AddTri(OutMesh, 1, 3, 5);
    AddTri(OutMesh, 3, 4, 5);
}

void FWMProceduralVFXGeometry::BuildHalo(FWMVFXMeshData& OutMesh, const int32 Segments)
{
    BuildRing(OutMesh, Segments, 0.82f, 1.0f);
}
