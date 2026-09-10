#include "Environment/WMCaribbeanRainforestPrototype.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

const FName AWMCaribbeanRainforestPrototype::PrototypeBiomeTag(TEXT("WM_CaribbeanRainforestPrototype"));

AWMCaribbeanRainforestPrototype::AWMCaribbeanRainforestPrototype()
{
    PrimaryActorTick.bCanEverTick = false;
    Tags.Add(PrototypeBiomeTag);

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

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

    PrototypeSun = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("PrototypeSun"));
    PrototypeSun->SetupAttachment(SceneRoot);
    PrototypeSun->SetRelativeRotation(FRotator(-42.0f, -28.0f, 0.0f));

    PrototypeSkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("PrototypeSkyLight"));
    PrototypeSkyLight->SetupAttachment(SceneRoot);

    PrototypeSkyAtmosphere = CreateDefaultSubobject<USkyAtmosphereComponent>(TEXT("PrototypeSkyAtmosphere"));
    PrototypeSkyAtmosphere->SetupAttachment(SceneRoot);

    PrototypeHeightFog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("PrototypeHeightFog"));
    PrototypeHeightFog->SetupAttachment(SceneRoot);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

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
}

void AWMCaribbeanRainforestPrototype::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    RebuildPrototype();
}

void AWMCaribbeanRainforestPrototype::RefreshFromVisualProfile()
{
    if (bUseProfileDefaultQuality)
    {
        RebuildPrototype();
    }
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

void AWMCaribbeanRainforestPrototype::RebuildPrototype()
{
    GroundTiles->ClearInstances();
    TerrainMounds->ClearInstances();
    TreeTrunks->ClearInstances();
    TreeCanopies->ClearInstances();
    Rocks->ClearInstances();
    WaterEdgeMarkers->ClearInstances();

    const UWMVisualProfileSettings* Profile = GetDefault<UWMVisualProfileSettings>();
    if (!Profile)
    {
        return;
    }

    const EWMVisualQualityTier EffectiveTier = bUseProfileDefaultQuality ? Profile->DefaultQualityTier : QualityTier;
    const FWMVisualBudget& Budget = Profile->GetBudget(EffectiveTier);
    if (!Budget.IsSane() || !Profile->Atmosphere.IsSane())
    {
        return;
    }

    ApplyLookDevelopmentProfile();

    GroundTiles->AddInstance(FTransform(FRotator::ZeroRotator, FVector(0.0f, 0.0f, -50.0f), FVector(50.0f, 50.0f, 1.0f)));

    FRandomStream Random(Seed);

    for (int32 Index = 0; Index < Budget.TerrainMounds; ++Index)
    {
        FVector Location = RandomRingPoint(Random, BuildClearingRadiusCm + 250.0f, BiomeRadiusCm * 0.95f);
        Location.Z = Random.FRandRange(-70.0f, -20.0f);
        const FVector Scale(Random.FRandRange(2.5f, 5.5f), Random.FRandRange(2.5f, 5.5f), Random.FRandRange(0.35f, 0.75f));
        TerrainMounds->AddInstance(FTransform(FRotator::ZeroRotator, Location, Scale));
    }

    for (int32 Index = 0; Index < Budget.TreeClusters; ++Index)
    {
        const FVector Base = RandomRingPoint(Random, BuildClearingRadiusCm, BiomeRadiusCm);
        const float HeightScale = Random.FRandRange(2.4f, 4.1f);
        const float WidthScale = Random.FRandRange(0.28f, 0.48f);
        const float TrunkHeightCm = 100.0f * HeightScale;

        TreeTrunks->AddInstance(FTransform(
            FRotator(0.0f, Random.FRandRange(0.0f, 360.0f), 0.0f),
            Base + FVector(0.0f, 0.0f, TrunkHeightCm * 0.5f),
            FVector(WidthScale, WidthScale, HeightScale)));

        const float CanopyScale = Random.FRandRange(0.9f, 1.5f);
        TreeCanopies->AddInstance(FTransform(
            FRotator::ZeroRotator,
            Base + FVector(0.0f, 0.0f, TrunkHeightCm + Random.FRandRange(35.0f, 85.0f)),
            FVector(CanopyScale, CanopyScale, Random.FRandRange(0.75f, 1.15f))));
    }

    for (int32 Index = 0; Index < Budget.RockClusters; ++Index)
    {
        FVector Location = RandomRingPoint(Random, BuildClearingRadiusCm + 150.0f, BiomeRadiusCm);
        Location.Z = Random.FRandRange(10.0f, 35.0f);
        Rocks->AddInstance(FTransform(
            FRotator(Random.FRandRange(-8.0f, 8.0f), Random.FRandRange(0.0f, 360.0f), 0.0f),
            Location,
            FVector(Random.FRandRange(0.35f, 0.85f), Random.FRandRange(0.35f, 0.85f), Random.FRandRange(0.25f, 0.65f))));
    }

    const int32 WaterCount = FMath::Max(Budget.WaterMarkers, 1);
    for (int32 Index = 0; Index < WaterCount; ++Index)
    {
        const float Alpha = WaterCount == 1 ? 0.5f : static_cast<float>(Index) / static_cast<float>(WaterCount - 1);
        const float X = FMath::Lerp(-BiomeRadiusCm, BiomeRadiusCm, Alpha);
        WaterEdgeMarkers->AddInstance(FTransform(
            FRotator::ZeroRotator,
            FVector(X, -BiomeRadiusCm - 250.0f, -42.0f),
            FVector(4.0f, 3.0f, 0.12f)));
    }
}
