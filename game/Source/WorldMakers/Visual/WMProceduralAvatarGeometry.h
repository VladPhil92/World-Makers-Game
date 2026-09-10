#pragma once

#include "CoreMinimal.h"
#include "Visual/WMAvatarArtTypes.h"
#include "Visual/WMProceduralEnvironmentGeometry.h"

/** Original low-poly avatar geometry used by the source-visible V4 character art path. */
class WORLDMAKERS_API FWMProceduralAvatarGeometry
{
public:
    static void BuildTorso(FWMEnvironmentMeshData& Mesh, const FWMAvatarProportions& Proportions);
    static void BuildHead(FWMEnvironmentMeshData& Mesh, const FWMAvatarProportions& Proportions, int32 Segments);
    static void BuildHairCap(FWMEnvironmentMeshData& Mesh, const FWMAvatarProportions& Proportions, int32 Segments);
    static void BuildUpperArm(FWMEnvironmentMeshData& Mesh, const FWMAvatarProportions& Proportions, int32 Sides);
    static void BuildLowerArm(FWMEnvironmentMeshData& Mesh, const FWMAvatarProportions& Proportions, int32 Sides);
    static void BuildHand(FWMEnvironmentMeshData& Mesh, int32 Segments);
    static void BuildThigh(FWMEnvironmentMeshData& Mesh, const FWMAvatarProportions& Proportions, int32 Sides);
    static void BuildCalf(FWMEnvironmentMeshData& Mesh, const FWMAvatarProportions& Proportions, int32 Sides);
    static void BuildFoot(FWMEnvironmentMeshData& Mesh, int32 Segments);

    static int32 EstimateTriangleCount(const FWMAvatarProportions& Proportions, const FWMAvatarArtBudget& Budget);
};
