#include "Visual/WMProceduralAvatarGeometry.h"

void FWMProceduralAvatarGeometry::BuildTorso(FWMEnvironmentMeshData& Mesh, const FWMAvatarProportions& P)
{
    Mesh.Reset();
    if (!P.IsSane())
    {
        return;
    }

    const float ShoulderX = P.TorsoDepthCm * 0.50f;
    const float ShoulderY = P.ShoulderWidthCm * 0.50f;
    const float HipX = P.TorsoDepthCm * 0.44f;
    const float HipY = P.HipWidthCm * 0.50f;
    const float H = P.TorsoHeightCm;

    const FVector BL0(-HipX, -HipY, 0.0f);
    const FVector BR0(-HipX, HipY, 0.0f);
    const FVector FR0(HipX, HipY, 0.0f);
    const FVector FL0(HipX, -HipY, 0.0f);
    const FVector BL1(-ShoulderX, -ShoulderY, H);
    const FVector BR1(-ShoulderX, ShoulderY, H);
    const FVector FR1(ShoulderX, ShoulderY, H);
    const FVector FL1(ShoulderX, -ShoulderY, H);

    FWMProceduralEnvironmentGeometry::AppendQuad(Mesh, BL0, BR0, FR0, FL0);
    FWMProceduralEnvironmentGeometry::AppendQuad(Mesh, FL1, FR1, BR1, BL1);
    FWMProceduralEnvironmentGeometry::AppendQuad(Mesh, FL0, FR0, FR1, FL1);
    FWMProceduralEnvironmentGeometry::AppendQuad(Mesh, BR0, BL0, BL1, BR1);
    FWMProceduralEnvironmentGeometry::AppendQuad(Mesh, BL0, FL0, FL1, BL1);
    FWMProceduralEnvironmentGeometry::AppendQuad(Mesh, FR0, BR0, BR1, FR1);
}

void FWMProceduralAvatarGeometry::BuildHead(FWMEnvironmentMeshData& Mesh, const FWMAvatarProportions& P, const int32 Segments)
{
    Mesh.Reset();
    if (!P.IsSane())
    {
        return;
    }
    const float H = P.HeadHeightCm;
    FWMProceduralEnvironmentGeometry::AppendFacetedEllipsoid(Mesh, FVector::ZeroVector, FVector(H * 0.44f, H * 0.42f, H * 0.50f), Segments);
}

void FWMProceduralAvatarGeometry::BuildHairCap(FWMEnvironmentMeshData& Mesh, const FWMAvatarProportions& P, const int32 Segments)
{
    Mesh.Reset();
    if (!P.IsSane())
    {
        return;
    }
    const float H = P.HeadHeightCm;
    FWMProceduralEnvironmentGeometry::AppendFacetedEllipsoid(Mesh, FVector(-1.5f, 0.0f, H * 0.29f), FVector(H * 0.45f, H * 0.43f, H * 0.23f), Segments, 9.0f);
    FWMProceduralEnvironmentGeometry::AppendFacetedEllipsoid(Mesh, FVector(1.5f, -H * 0.20f, H * 0.32f), FVector(H * 0.25f, H * 0.24f, H * 0.20f), Segments, -12.0f);
    FWMProceduralEnvironmentGeometry::AppendFacetedEllipsoid(Mesh, FVector(1.0f, H * 0.20f, H * 0.31f), FVector(H * 0.24f, H * 0.23f, H * 0.19f), Segments, 14.0f);
}

void FWMProceduralAvatarGeometry::BuildUpperArm(FWMEnvironmentMeshData& Mesh, const FWMAvatarProportions& P, const int32 Sides)
{
    Mesh.Reset();
    FWMProceduralEnvironmentGeometry::AppendTaperedTrunk(Mesh, FVector::ZeroVector, FVector(0.0f, 0.0f, -P.UpperArmLengthCm), 5.3f, 4.5f, Sides);
}

void FWMProceduralAvatarGeometry::BuildLowerArm(FWMEnvironmentMeshData& Mesh, const FWMAvatarProportions& P, const int32 Sides)
{
    Mesh.Reset();
    FWMProceduralEnvironmentGeometry::AppendTaperedTrunk(Mesh, FVector::ZeroVector, FVector(0.0f, 0.0f, -P.LowerArmLengthCm), 4.4f, 3.6f, Sides, 8.0f);
}

void FWMProceduralAvatarGeometry::BuildHand(FWMEnvironmentMeshData& Mesh, const int32 Segments)
{
    Mesh.Reset();
    FWMProceduralEnvironmentGeometry::AppendFacetedEllipsoid(Mesh, FVector(1.0f, 0.0f, -4.5f), FVector(4.8f, 4.3f, 6.0f), Segments, 12.0f);
}

void FWMProceduralAvatarGeometry::BuildThigh(FWMEnvironmentMeshData& Mesh, const FWMAvatarProportions& P, const int32 Sides)
{
    Mesh.Reset();
    FWMProceduralEnvironmentGeometry::AppendTaperedTrunk(Mesh, FVector::ZeroVector, FVector(0.0f, 0.0f, -P.ThighLengthCm), 7.0f, 5.6f, Sides);
}

void FWMProceduralAvatarGeometry::BuildCalf(FWMEnvironmentMeshData& Mesh, const FWMAvatarProportions& P, const int32 Sides)
{
    Mesh.Reset();
    FWMProceduralEnvironmentGeometry::AppendTaperedTrunk(Mesh, FVector::ZeroVector, FVector(0.0f, 0.0f, -P.CalfLengthCm), 5.5f, 4.1f, Sides, 6.0f);
}

void FWMProceduralAvatarGeometry::BuildFoot(FWMEnvironmentMeshData& Mesh, const int32 Segments)
{
    Mesh.Reset();
    FWMProceduralEnvironmentGeometry::AppendFacetedEllipsoid(Mesh, FVector(5.0f, 0.0f, -3.0f), FVector(10.0f, 6.2f, 5.2f), Segments);
}

int32 FWMProceduralAvatarGeometry::EstimateTriangleCount(const FWMAvatarProportions& P, const FWMAvatarArtBudget& Budget)
{
    if (!P.IsSane() || !Budget.IsSane())
    {
        return 0;
    }

    // Torso = 12. Eight tapered limb segments = 32 triangles per side count.
    // Head/hair/hands/feet together = 32 triangles per ellipsoid segment count.
    return 12 + (32 * Budget.FacetSides) + (32 * Budget.HeadSegments);
}
