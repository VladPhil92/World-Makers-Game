#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"

struct WORLDMAKERS_API FWMVFXMeshData
{
    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UV0;
    TArray<FLinearColor> VertexColors;
    TArray<FProcMeshTangent> Tangents;

    void Reset();
    bool IsSane() const;
};

struct WORLDMAKERS_API FWMProceduralVFXGeometry
{
    static void BuildRing(FWMVFXMeshData& OutMesh, int32 Segments = 24, float InnerRadius = 0.72f, float OuterRadius = 1.0f);
    static void BuildBurst(FWMVFXMeshData& OutMesh, int32 Rays = 8, float InnerRadius = 0.35f, float OuterRadius = 1.0f);
    static void BuildDirectionalChevron(FWMVFXMeshData& OutMesh);
    static void BuildHalo(FWMVFXMeshData& OutMesh, int32 Segments = 24);
};
