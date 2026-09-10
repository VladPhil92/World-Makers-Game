#include "Environment/WMCaribbeanRainforestPrototype.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Visual/WMProceduralEnvironmentGeometry.h"
#include "Visual/WMStylizedSurfaceLibrary.h"

namespace WMEnvironmentArt
{
    void ConfigureRenderOnly(UProceduralMeshComponent* Component)
    {
        if (!Component)
        {
            return;
        }
        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Component->SetCanEverAffectNavigation(false);
        Component->bUseAsyncCooking = false;
    }

    void CommitMesh(UProceduralMeshComponent* Component, const FWMEnvironmentMeshData& Mesh)
    {
        if (!Component)
        {
            return;
        }

        Component->ClearAllMeshSections();
        if (!Mesh.IsSane())
        {
            return;
        }

        Component->CreateMeshSection_LinearColor(
            0,
            Mesh.Vertices,
            Mesh.Triangles,
            Mesh.Normals,
            Mesh.UV0,
            Mesh.VertexColors,
            Mesh.Tangents,
            false);
    }
}

const FName AWMCaribbeanRainforestPrototype::PrototypeBiomeTag(TEXT("WM_CaribbeanRainforestPrototype"));

AWMCaribbeanRainforestPrototype::AWMCaribbeanRainforestPrototype()
{
    PrimaryActorTick.bCanEverTick = false;
    Tags.Add(PrototypeBiomeTag);

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    // Gameplay/collision proxies remain separate from the V3 render path.
    GroundTiles = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("GroundTiles"));
    GroundTiles->SetupAttachment(SceneRoot);
    GroundTiles->SetCollisionProfileName(TEXT("BlockAll"));

    TerrainMounds = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("TerrainMounds"));
    TerrainMounds->SetupAttachment(SceneRoot);
    TerrainMounds->SetCollisionProfileName(TEXT("BlockAll"));

    TreeTrunks = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("TreeTrunks"));
    TreeTrunks->SetupAttachment(SceneRoot);
    TreeTrunks->SetCollisionProfileName(TEXT("BlockAll"));

    TreeCanopies = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("TreeCanopies"));
    TreeCanopies->SetupAttachment(SceneRoot);
    TreeCanopies->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    Rocks = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Rocks"));
    Rocks->SetupAttachment(SceneRoot);
    Rocks->SetCollisionProfileName(TEXT("BlockAll"));

    WaterEdgeMarkers = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("WaterEdgeMarkers"));
    WaterEdgeMarkers->SetupAttachment(SceneRoot);
    WaterEdgeMarkers->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // V3 original procedural environment art. One component per surface family keeps V2 material semantics intact.
    GroundArt = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("GroundArt"));
    GroundArt->SetupAttachment(SceneRoot);
    WMEnvironmentArt::ConfigureRenderOnly(GroundArt);

    TerrainArt = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("TerrainArt"));
    TerrainArt->SetupAttachment(SceneRoot);
    WMEnvironmentArt::ConfigureRenderOnly(TerrainArt);

    BarkAndRootsArt = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("BarkAndRootsArt"));
    BarkAndRootsArt->SetupAttachment(SceneRoot);
    WMEnvironmentArt::ConfigureRenderOnly(BarkAndRootsArt);

    FoliageArt = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("FoliageArt"));
    FoliageArt->SetupAttachment(SceneRoot);
    WMEnvironmentArt::ConfigureRenderOnly(FoliageArt);

    StoneArt = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("StoneArt"));
    StoneArt->SetupAttachment(SceneRoot);
    WMEnvironmentArt::ConfigureRenderOnly(StoneArt);

    WaterArt = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("WaterArt"));
    WaterArt->SetupAttachment(SceneRoot);
    WMEnvironmentArt::ConfigureRenderOnly(WaterArt);

    PrototypeSun = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("PrototypeSun"));
    PrototypeSun->SetupAttachment(SceneRoot);
    PrototypeSun->SetRelativeRotation(FRotator(-42.0f, -28.0f, 0.0f));
    PrototypeSun->SetAtmosphereSunLight(true);
    PrototypeSun->SetAtmosphereSunLightIndex(0);

    PrototypeSkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("PrototypeSkyLight"));
    PrototypeSkyLight->SetupAttachment(SceneRoot);
    PrototypeSkyLight->SetRealTimeCapture(false);

    PrototypeSkyAtmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("PrototypeSkyAtmosphere"));
    PrototypeSkyAtmosphere->SetupAttachment(SceneRoot);

    PrototypeHeightFog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("PrototypeHeightFog"));
    PrototypeHeightFog->SetupAttachment(SceneRoot);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> ProxySurfaceMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

    if (CubeMesh.Succeeded())
    {
        GroundTiles->SetStaticMesh(CubeMesh.Object);
        WaterEdgeMarkers->SetStaticMesh(CubeMesh.Object);
    }
    if (SphereMesh.Succeeded())
    {
        TerrainMounds->SetStaticMesh(SphereMesh.Object);
        TreeCanopies->SetStaticMesh(SphereMesh.Object);
        Rocks->SetStaticMesh(SphereMesh.Object);
    }
    if (CylinderMesh.Succeeded())
    {
        TreeTrunks->SetStaticMesh(CylinderMesh.Object);
    }

    if (ProxySurfaceMaterial.Succeeded())
    {
        for (UMeshComponent* MeshComponent : TArray<UMeshComponent*>{GroundTiles, TerrainMounds, TreeTrunks, TreeCanopies, Rocks, WaterEdgeMarkers, GroundArt, TerrainArt, BarkAndRootsArt, FoliageArt, StoneArt, WaterArt})
        {
            if (MeshComponent)
            {
                MeshComponent->SetMaterial(0, ProxySurfaceMaterial.Object);
            }
        }
    }
}

void AWMCaribbeanRainforestPrototype::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    RebuildPrototype();
}

void AWMCaribbeanRainforestPrototype::RefreshFromVisualProfile()
{
    RebuildPrototype();
}

FVector AWMCaribbeanRainforestPrototype::RandomRingPoint(FRandomStream& Random, const float MinRadius, const float MaxRadius) const
{
    const float Angle = Random.FRandRange(0.0f, 2.0f * PI);
    const float Radius = FMath::Sqrt(Random.FRandRange(FMath::Square(MinRadius), FMath::Square(MaxRadius)));
    return FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.0f);
}

void AWMCaribbeanRainforestPrototype::ApplyLookDevelopmentProfile()
{
    const UWMVisualProfileSettings* Profile = GetDefault<UWMVisualProfileSettings>();
    if (!Profile || !Profile->Atmosphere.IsSane())
    {
        return;
    }

    PrototypeSun->SetIntensity(Profile->Atmosphere.SunIntensity);
    PrototypeSun->SetLightColor(Profile->Palette.Sunlight);

    PrototypeSkyLight->SetIntensity(Profile->Atmosphere.SkyLightIntensity);
    PrototypeSkyLight->SetLightColor(Profile->Palette.Sky);

    PrototypeHeightFog->SetFogDensity(Profile->Atmosphere.FogDensity);
    PrototypeHeightFog->SetFogHeightFalloff(Profile->Atmosphere.FogHeightFalloff);
    PrototypeHeightFog->SetFogInscatteringColor(FLinearColor::LerpUsingHSV(Profile->Palette.Sky, Profile->Palette.Sunlight, 0.18f));
}

void AWMCaribbeanRainforestPrototype::ApplySurfaceLanguage()
{
    UWMStylizedSurfaceLibrary::ApplyConfiguredSurface(GroundTiles, EWMStylizedSurfaceRole::GroundEarth);
    UWMStylizedSurfaceLibrary::ApplyConfiguredSurface(TerrainMounds, EWMStylizedSurfaceRole::Terrain);
    UWMStylizedSurfaceLibrary::ApplyConfiguredSurface(TreeTrunks, EWMStylizedSurfaceRole::Bark);
    UWMStylizedSurfaceLibrary::ApplyConfiguredSurface(TreeCanopies, EWMStylizedSurfaceRole::Foliage);
    UWMStylizedSurfaceLibrary::ApplyConfiguredSurface(Rocks, EWMStylizedSurfaceRole::Stone);
    UWMStylizedSurfaceLibrary::ApplyConfiguredSurface(WaterEdgeMarkers, EWMStylizedSurfaceRole::Water);

    UWMStylizedSurfaceLibrary::ApplyConfiguredSurface(GroundArt, EWMStylizedSurfaceRole::GroundEarth);
    UWMStylizedSurfaceLibrary::ApplyConfiguredSurface(TerrainArt, EWMStylizedSurfaceRole::Terrain);
    UWMStylizedSurfaceLibrary::ApplyConfiguredSurface(BarkAndRootsArt, EWMStylizedSurfaceRole::Bark);
    UWMStylizedSurfaceLibrary::ApplyConfiguredSurface(FoliageArt, EWMStylizedSurfaceRole::Foliage);
    UWMStylizedSurfaceLibrary::ApplyConfiguredSurface(StoneArt, EWMStylizedSurfaceRole::Stone);
    UWMStylizedSurfaceLibrary::ApplyConfiguredSurface(WaterArt, EWMStylizedSurfaceRole::Water);
}

void AWMCaribbeanRainforestPrototype::SetEnvironmentArtPathEnabled(const bool bEnabled)
{
    for (UPrimitiveComponent* Proxy : TArray<UPrimitiveComponent*>{GroundTiles, TerrainMounds, TreeTrunks, TreeCanopies, Rocks, WaterEdgeMarkers})
    {
        if (Proxy)
        {
            Proxy->SetVisibility(!bEnabled, true);
            Proxy->SetHiddenInGame(bEnabled, true);
        }
    }

    for (UPrimitiveComponent* Art : TArray<UPrimitiveComponent*>{GroundArt, TerrainArt, BarkAndRootsArt, FoliageArt, StoneArt, WaterArt})
    {
        if (Art)
        {
            Art->SetVisibility(bEnabled, true);
            Art->SetHiddenInGame(!bEnabled, true);
        }
    }
}

void AWMCaribbeanRainforestPrototype::RebuildPrototype()
{
    GroundTiles->ClearInstances();
    TerrainMounds->ClearInstances();
    TreeTrunks->ClearInstances();
    TreeCanopies->ClearInstances();
    Rocks->ClearInstances();
    WaterEdgeMarkers->ClearInstances();

    GroundArt->ClearAllMeshSections();
    TerrainArt->ClearAllMeshSections();
    BarkAndRootsArt->ClearAllMeshSections();
    FoliageArt->ClearAllMeshSections();
    StoneArt->ClearAllMeshSections();
    WaterArt->ClearAllMeshSections();

    const UWMVisualProfileSettings* Profile = GetDefault<UWMVisualProfileSettings>();
    if (!Profile)
    {
        return;
    }

    const EWMVisualQualityTier EffectiveTier = bUseProfileDefaultQuality ? Profile->DefaultQualityTier : QualityTier;
    const FWMVisualBudget& Budget = Profile->GetBudget(EffectiveTier);
    if (!Budget.IsSane() || !Profile->Atmosphere.IsSane() || !Profile->SurfaceResponse.IsSane())
    {
        return;
    }

    const int32 FacetSegments = EffectiveTier == EWMVisualQualityTier::Low ? 5 : (EffectiveTier == EWMVisualQualityTier::High ? 8 : 6);
    const int32 GroundSegments = EffectiveTier == EWMVisualQualityTier::Low ? 18 : (EffectiveTier == EWMVisualQualityTier::High ? 34 : 26);
    const int32 RiverSegments = EffectiveTier == EWMVisualQualityTier::Low ? 10 : (EffectiveTier == EWMVisualQualityTier::High ? 22 : 16);
    const int32 LeafCount = EffectiveTier == EWMVisualQualityTier::Low ? 4 : (EffectiveTier == EWMVisualQualityTier::High ? 7 : 5);

    ApplyLookDevelopmentProfile();
    ApplySurfaceLanguage();
    SetEnvironmentArtPathEnabled(bUseProceduralEnvironmentArt);

    // Keep collision-proxy geometry deterministic and independent from the visible art path.
    GroundTiles->AddInstance(FTransform(FRotator::ZeroRotator, FVector(0.0f, 0.0f, -50.0f), FVector(50.0f, 50.0f, 1.0f)));

    FWMEnvironmentMeshData GroundMesh;
    FWMEnvironmentMeshData TerrainMesh;
    FWMEnvironmentMeshData BarkMesh;
    FWMEnvironmentMeshData FoliageMesh;
    FWMEnvironmentMeshData StoneMesh;
    FWMEnvironmentMeshData WaterMesh;

    FRandomStream Random(Seed);
    FWMProceduralEnvironmentGeometry::AppendIrregularGroundDisc(
        GroundMesh,
        FVector(0.0f, 0.0f, -44.0f),
        BiomeRadiusCm * 1.28f,
        GroundSegments,
        Random);

    for (int32 Index = 0; Index < Budget.TerrainMounds; ++Index)
    {
        FVector Location = RandomRingPoint(Random, BuildClearingRadiusCm + 250.0f, BiomeRadiusCm * 0.95f);
        Location.Z = Random.FRandRange(-70.0f, -20.0f);
        const FVector Scale(Random.FRandRange(2.5f, 5.5f), Random.FRandRange(2.5f, 5.5f), Random.FRandRange(0.35f, 0.75f));
        TerrainMounds->AddInstance(FTransform(FRotator::ZeroRotator, Location, Scale));

        FWMProceduralEnvironmentGeometry::AppendFacetedEllipsoid(
            TerrainMesh,
            Location + FVector(0.0f, 0.0f, 22.0f),
            FVector(Scale.X * 55.0f, Scale.Y * 55.0f, FMath::Max(Scale.Z * 85.0f, 30.0f)),
            FacetSegments + 1,
            Random.FRandRange(0.0f, 360.0f));
    }

    for (int32 Index = 0; Index < Budget.TreeClusters; ++Index)
    {
        const FVector Base = RandomRingPoint(Random, BuildClearingRadiusCm, BiomeRadiusCm);
        const float HeightScale = Random.FRandRange(2.4f, 4.1f);
        const float WidthScale = Random.FRandRange(0.28f, 0.48f);
        const float TrunkHeightCm = 100.0f * HeightScale;
        const float Yaw = Random.FRandRange(0.0f, 360.0f);
        const float LeanAngle = FMath::DegreesToRadians(Yaw + Random.FRandRange(-35.0f, 35.0f));
        const float LeanCm = Random.FRandRange(8.0f, 28.0f);
        const FVector TrunkTop = Base + FVector(FMath::Cos(LeanAngle) * LeanCm, FMath::Sin(LeanAngle) * LeanCm, TrunkHeightCm);

        TreeTrunks->AddInstance(FTransform(
            FRotator(0.0f, Yaw, 0.0f),
            Base + FVector(0.0f, 0.0f, TrunkHeightCm * 0.5f),
            FVector(WidthScale, WidthScale, HeightScale)));

        FWMProceduralEnvironmentGeometry::AppendTaperedTrunk(
            BarkMesh,
            Base,
            TrunkTop,
            WidthScale * 62.0f,
            WidthScale * 34.0f,
            FacetSegments,
            Yaw);

        if ((Index % 3) == 0)
        {
            for (int32 RootIndex = 0; RootIndex < 3; ++RootIndex)
            {
                const float RootAngle = FMath::DegreesToRadians(Yaw + RootIndex * 120.0f);
                FWMProceduralEnvironmentGeometry::AppendButtressRoot(
                    BarkMesh,
                    Base,
                    FVector(FMath::Cos(RootAngle), FMath::Sin(RootAngle), 0.0f),
                    Random.FRandRange(85.0f, 145.0f),
                    Random.FRandRange(32.0f, 52.0f),
                    Random.FRandRange(55.0f, 90.0f));
            }
        }

        const float CanopyScale = Random.FRandRange(0.9f, 1.5f);
        const float CanopyVerticalScale = Random.FRandRange(0.75f, 1.15f);
        const FVector CanopyCenter = TrunkTop + FVector(0.0f, 0.0f, Random.FRandRange(45.0f, 82.0f));
        TreeCanopies->AddInstance(FTransform(
            FRotator::ZeroRotator,
            CanopyCenter,
            FVector(CanopyScale, CanopyScale, CanopyVerticalScale)));

        const int32 Variant = Index % 3;
        if (Variant == 0)
        {
            FWMProceduralEnvironmentGeometry::AppendFacetedEllipsoid(
                FoliageMesh,
                CanopyCenter,
                FVector(118.0f * CanopyScale, 104.0f * CanopyScale, 82.0f * CanopyVerticalScale),
                FacetSegments + 2,
                Yaw);
        }
        else if (Variant == 1)
        {
            const FVector Offset(FMath::Cos(LeanAngle) * 48.0f, FMath::Sin(LeanAngle) * 48.0f, 0.0f);
            FWMProceduralEnvironmentGeometry::AppendFacetedEllipsoid(
                FoliageMesh,
                CanopyCenter - Offset,
                FVector(92.0f * CanopyScale, 82.0f * CanopyScale, 76.0f * CanopyVerticalScale),
                FacetSegments + 1,
                Yaw);
            FWMProceduralEnvironmentGeometry::AppendFacetedEllipsoid(
                FoliageMesh,
                CanopyCenter + Offset * 0.85f + FVector(0.0f, 0.0f, 22.0f),
                FVector(88.0f * CanopyScale, 96.0f * CanopyScale, 70.0f * CanopyVerticalScale),
                FacetSegments + 1,
                Yaw + 24.0f);
        }
        else
        {
            FWMProceduralEnvironmentGeometry::AppendFacetedEllipsoid(
                FoliageMesh,
                CanopyCenter,
                FVector(138.0f * CanopyScale, 126.0f * CanopyScale, 58.0f * CanopyVerticalScale),
                FacetSegments + 2,
                Yaw);
            FWMProceduralEnvironmentGeometry::AppendFacetedEllipsoid(
                FoliageMesh,
                CanopyCenter + FVector(0.0f, 0.0f, 62.0f),
                FVector(72.0f * CanopyScale, 68.0f * CanopyScale, 54.0f * CanopyVerticalScale),
                FacetSegments,
                Yaw + 31.0f);
        }

        if ((Index % 2) == 0)
        {
            const FVector UnderstoryCenter = Base + FVector(
                FMath::Cos(LeanAngle + 1.7f) * Random.FRandRange(45.0f, 105.0f),
                FMath::Sin(LeanAngle + 1.7f) * Random.FRandRange(45.0f, 105.0f),
                6.0f);
            FWMProceduralEnvironmentGeometry::AppendLeafCluster(
                FoliageMesh,
                UnderstoryCenter,
                Random.FRandRange(58.0f, 92.0f),
                LeafCount,
                Yaw + 17.0f);
        }
    }

    for (int32 Index = 0; Index < Budget.RockClusters; ++Index)
    {
        FVector Location = RandomRingPoint(Random, BuildClearingRadiusCm + 150.0f, BiomeRadiusCm);
        Location.Z = Random.FRandRange(10.0f, 35.0f);
        const FVector Scale(Random.FRandRange(0.35f, 0.85f), Random.FRandRange(0.35f, 0.85f), Random.FRandRange(0.25f, 0.65f));
        const float Yaw = Random.FRandRange(0.0f, 360.0f);
        Rocks->AddInstance(FTransform(FRotator(Random.FRandRange(-8.0f, 8.0f), Yaw, 0.0f), Location, Scale));

        FWMProceduralEnvironmentGeometry::AppendFacetedEllipsoid(
            StoneMesh,
            Location + FVector(0.0f, 0.0f, Scale.Z * 38.0f),
            FVector(Scale.X * 78.0f, Scale.Y * 70.0f, Scale.Z * 68.0f),
            FacetSegments,
            Yaw);
    }

    const int32 WaterCount = FMath::Max(Budget.WaterMarkers, 1);
    const float WaterCenterY = -BiomeRadiusCm - 250.0f;
    for (int32 Index = 0; Index < WaterCount; ++Index)
    {
        const float Alpha = WaterCount == 1 ? 0.5f : static_cast<float>(Index) / static_cast<float>(WaterCount - 1);
        const float X = FMath::Lerp(-BiomeRadiusCm, BiomeRadiusCm, Alpha);
        WaterEdgeMarkers->AddInstance(FTransform(
            FRotator::ZeroRotator,
            FVector(X, WaterCenterY, -42.0f),
            FVector(4.0f, 3.0f, 0.12f)));
    }

    FWMProceduralEnvironmentGeometry::AppendSinuousRiverStrip(
        WaterMesh,
        BiomeRadiusCm * 1.16f,
        WaterCenterY,
        -36.0f,
        185.0f,
        RiverSegments,
        118.0f,
        1.35f);

    // A deterministic hero ceiba creates a landmark silhouette without introducing cultural architecture.
    const FVector HeroBase(BiomeRadiusCm * 0.62f, BiomeRadiusCm * 0.54f, 0.0f);
    const float HeroHeight = 560.0f;
    TreeTrunks->AddInstance(FTransform(FRotator::ZeroRotator, HeroBase + FVector(0.0f, 0.0f, HeroHeight * 0.5f), FVector(1.05f, 1.05f, 5.6f)));
    FWMProceduralEnvironmentGeometry::AppendTaperedTrunk(BarkMesh, HeroBase, HeroBase + FVector(28.0f, -14.0f, HeroHeight), 72.0f, 32.0f, FMath::Max(FacetSegments + 2, 8), 12.0f);
    for (int32 RootIndex = 0; RootIndex < 6; ++RootIndex)
    {
        const float Angle = 2.0f * PI * static_cast<float>(RootIndex) / 6.0f;
        FWMProceduralEnvironmentGeometry::AppendButtressRoot(
            BarkMesh,
            HeroBase,
            FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f),
            205.0f,
            76.0f,
            132.0f);
    }
    const FVector HeroCrown = HeroBase + FVector(28.0f, -14.0f, HeroHeight + 38.0f);
    FWMProceduralEnvironmentGeometry::AppendFacetedEllipsoid(FoliageMesh, HeroCrown, FVector(245.0f, 205.0f, 118.0f), FacetSegments + 3, 12.0f);
    FWMProceduralEnvironmentGeometry::AppendFacetedEllipsoid(FoliageMesh, HeroCrown + FVector(145.0f, 28.0f, 22.0f), FVector(155.0f, 135.0f, 98.0f), FacetSegments + 1, 37.0f);
    FWMProceduralEnvironmentGeometry::AppendFacetedEllipsoid(FoliageMesh, HeroCrown + FVector(-132.0f, 46.0f, -4.0f), FVector(148.0f, 128.0f, 92.0f), FacetSegments + 1, -21.0f);

    WMEnvironmentArt::CommitMesh(GroundArt, GroundMesh);
    WMEnvironmentArt::CommitMesh(TerrainArt, TerrainMesh);
    WMEnvironmentArt::CommitMesh(BarkAndRootsArt, BarkMesh);
    WMEnvironmentArt::CommitMesh(FoliageArt, FoliageMesh);
    WMEnvironmentArt::CommitMesh(StoneArt, StoneMesh);
    WMEnvironmentArt::CommitMesh(WaterArt, WaterMesh);
}
