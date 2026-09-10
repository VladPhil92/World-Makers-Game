#include "Visual/WMProceduralVFXActor.h"

#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Visual/WMProceduralVFXGeometry.h"

AWMProceduralVFXActor::AWMProceduralVFXActor()
{
    PrimaryActorTick.bCanEverTick = true;
    SetActorEnableCollision(false);

    Mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("VFXMesh"));
    RootComponent = Mesh;
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Mesh->SetCanEverAffectNavigation(false);
    Mesh->bUseAsyncCooking = false;
    Mesh->SetCastShadow(false);

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> FallbackMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (FallbackMaterial.Succeeded())
    {
        Mesh->SetMaterial(0, FallbackMaterial.Object);
    }
}

bool AWMProceduralVFXActor::InitializeEffect(const FWMVFXEvent& InEvent, const FWMVFXStyle& InStyle)
{
    if (!Mesh || !InEvent.IsSane() || !InStyle.IsSane())
    {
        return false;
    }

    Event = InEvent;
    Style = InStyle;
    AgeSeconds = 0.0f;

    FWMVFXMeshData Geometry;
    switch (Style.Shape)
    {
        case EWMVFXShape::Burst:
            FWMProceduralVFXGeometry::BuildBurst(Geometry, 8);
            break;
        case EWMVFXShape::Directional:
            FWMProceduralVFXGeometry::BuildDirectionalChevron(Geometry);
            break;
        case EWMVFXShape::Halo:
            FWMProceduralVFXGeometry::BuildHalo(Geometry, 24);
            break;
        case EWMVFXShape::Ring:
        default:
            FWMProceduralVFXGeometry::BuildRing(Geometry, 24);
            break;
    }

    if (!Geometry.IsSane())
    {
        return false;
    }

    Mesh->CreateMeshSection_LinearColor(
        0,
        Geometry.Vertices,
        Geometry.Triangles,
        Geometry.Normals,
        Geometry.UV0,
        Geometry.VertexColors,
        Geometry.Tangents,
        false);
    Mesh->SetVectorParameterValueOnMaterials(TEXT("Color"), Style.Color * Event.Intensity);

    SetActorLocation(Event.LocationCm);
    if (Style.Shape == EWMVFXShape::Directional && !Event.Direction.IsNearlyZero())
    {
        SetActorRotation(Event.Direction.GetSafeNormal().Rotation());
    }
    else
    {
        SetActorRotation(FRotator::ZeroRotator);
    }

    const float InitialScale = Style.BaseRadiusCm;
    SetActorScale3D(FVector(InitialScale));
    SetLifeSpan(Event.DurationSeconds + 0.05f);
    bInitialized = true;
    return true;
}

void AWMProceduralVFXActor::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!bInitialized || !FMath::IsFinite(DeltaSeconds) || DeltaSeconds <= 0.0f)
    {
        return;
    }

    AgeSeconds += DeltaSeconds;
    const float Alpha = FMath::Clamp(AgeSeconds / FMath::Max(Event.DurationSeconds, 0.08f), 0.0f, 1.0f);
    const float Ease = FMath::Sin(Alpha * PI * 0.5f);
    const float Travel = Style.TravelCm * Event.MotionScale * Ease;
    float Radius = Style.BaseRadiusCm + Travel;

    if (Style.Shape == EWMVFXShape::Burst)
    {
        Radius *= 0.82f + (0.18f * FMath::Sin(Alpha * PI));
        AddActorLocalRotation(FRotator(0.0f, DeltaSeconds * 55.0f * Event.MotionScale, 0.0f));
    }
    else if (Style.Shape == EWMVFXShape::Halo)
    {
        const float Pulse = 1.0f + 0.08f * FMath::Sin(Alpha * Style.PulseCycles * 2.0f * PI) * Event.MotionScale;
        Radius *= Pulse;
    }
    else if (Style.Shape == EWMVFXShape::Directional)
    {
        const FVector Offset = Event.Direction.GetSafeNormal() * (Style.TravelCm * Event.MotionScale * Alpha);
        SetActorLocation(Event.LocationCm + Offset);
    }

    SetActorScale3D(FVector(FMath::Max(Radius, 1.0f)));
}
