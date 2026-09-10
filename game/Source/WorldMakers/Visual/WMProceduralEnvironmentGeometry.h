#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"

/** Lightweight, deterministic triangle soup used by the V3 environment-art path. */
struct FWMEnvironmentMeshData
{
    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UV0;
    TArray<FLinearColor> VertexColors;
    TArray<FProcMeshTangent> Tangents;

    void Reset();
    bool IsSane() const;
    int32 NumTriangles() const { return Triangles.Num() / 3; }
};

/**
 * Original low-poly geometry vocabulary for the source-controlled V3 art pass.
 * These builders intentionally create faceted silhouettes instead of wrapping /Engine/BasicShapes assets.
 */
class WORLDMAKERS_API FWMProceduralEnvironmentGeometry
{
public:
    static void AppendTriangle(FWMEnvironmentMeshData& Mesh, const FVector& A, const FVector& B, const FVector& C);
    static void AppendQuad(FWMEnvironmentMeshData& Mesh, const FVector& A, const FVector& B, const FVector& C, const FVector& D);

    static void AppendIrregularGroundDisc(
        FWMEnvironmentMeshData& Mesh,
        const FVector& Center,
        float RadiusCm,
        int32 Segments,
        FRandomStream& Random);

    static void AppendTaperedTrunk(
        FWMEnvironmentMeshData& Mesh,
        const FVector& BaseCenter,
        const FVector& TopCenter,
        float BaseRadiusCm,
        float TopRadiusCm,
        int32 Sides,
        float YawDegrees = 0.0f);

    static void AppendFacetedEllipsoid(
        FWMEnvironmentMeshData& Mesh,
        const FVector& Center,
        const FVector& RadiiCm,
        int32 Segments,
        float YawDegrees = 0.0f);

    static void AppendButtressRoot(
        FWMEnvironmentMeshData& Mesh,
        const FVector& TrunkBase,
        const FVector& OutwardDirection,
        float LengthCm,
        float WidthCm,
        float HeightCm);

    static void AppendLeafCluster(
        FWMEnvironmentMeshData& Mesh,
        const FVector& Center,
        float RadiusCm,
        int32 LeafCount,
        float YawDegrees = 0.0f);

    static void AppendSinuousRiverStrip(
        FWMEnvironmentMeshData& Mesh,
        float HalfLengthCm,
        float CenterY,
        float Z,
        float HalfWidthCm,
        int32 Segments,
        float WaveAmplitudeCm,
        float WaveCycles);
};
