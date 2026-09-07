#include "Mission/WMMissionGeometryActor.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Mission/WMMissionGeometryLibrary.h"
#include "Mission/WMMissionRuntimeSubsystem.h"
#include "UObject/ConstructorHelpers.h"

const FName AWMMissionGeometryActor::PrototypeMissionGeometryTag(TEXT("WM_PrototypeMissionGeometry"));

AWMMissionGeometryActor::AWMMissionGeometryActor()
{
    PrimaryActorTick.bCanEverTick = false;
    Tags.Add(PrototypeMissionGeometryTag);

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;

    BuildZone = CreateDefaultSubobject<UBoxComponent>(TEXT("BuildZone"));
    BuildZone->SetupAttachment(SceneRoot);
    BuildZone->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BuildZone->SetHiddenInGame(true);

    MeasureStart = CreateDefaultSubobject<USceneComponent>(TEXT("MeasureStart"));
    MeasureStart->SetupAttachment(SceneRoot);
    MeasureEnd = CreateDefaultSubobject<USceneComponent>(TEXT("MeasureEnd"));
    MeasureEnd->SetupAttachment(SceneRoot);

    MeasureStartMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeasureStartMarker"));
    MeasureStartMarker->SetupAttachment(MeasureStart);
    MeasureStartMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    MeasureStartMarker->SetRelativeScale3D(FVector(0.14f));

    MeasureEndMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeasureEndMarker"));
    MeasureEndMarker->SetupAttachment(MeasureEnd);
    MeasureEndMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    MeasureEndMarker->SetRelativeScale3D(FVector(0.14f));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (SphereMesh.Succeeded())
    {
        MeasureStartMarker->SetStaticMesh(SphereMesh.Object);
        MeasureEndMarker->SetStaticMesh(SphereMesh.Object);
    }

    ApplyPrototypeLayout();
}

void AWMMissionGeometryActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    ApplyPrototypeLayout();
}

void AWMMissionGeometryActor::BeginPlay()
{
    Super::BeginPlay();
    ApplyPrototypeLayout();
    if (UWorld* World = GetWorld())
    {
        if (UWMMissionRuntimeSubsystem* Missions = World->GetSubsystem<UWMMissionRuntimeSubsystem>())
        {
            Missions->RegisterMissionGeometry(this);
        }
    }
}

void AWMMissionGeometryActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UWorld* World = GetWorld())
    {
        if (UWMMissionRuntimeSubsystem* Missions = World->GetSubsystem<UWMMissionRuntimeSubsystem>())
        {
            Missions->UnregisterMissionGeometry(this);
        }
    }
    Super::EndPlay(EndPlayReason);
}

void AWMMissionGeometryActor::ApplyPrototypeLayout()
{
    if (BuildZone)
    {
        BuildZone->SetBoxExtent(BuildZoneHalfExtent.ComponentMax(FVector(1.0f)));
    }

    const float HalfSpan = FMath::Max(1.0f, AnchorSpanCm) * 0.5f;
    if (MeasureStart)
    {
        MeasureStart->SetRelativeLocation(FVector(-HalfSpan, 0.0f, AnchorHeightCm));
    }
    if (MeasureEnd)
    {
        MeasureEnd->SetRelativeLocation(FVector(HalfSpan, 0.0f, AnchorHeightCm));
    }
}

FVector AWMMissionGeometryActor::GetMeasureStartWorld() const
{
    return MeasureStart ? MeasureStart->GetComponentLocation() : GetActorLocation();
}

FVector AWMMissionGeometryActor::GetMeasureEndWorld() const
{
    return MeasureEnd ? MeasureEnd->GetComponentLocation() : GetActorLocation();
}

float AWMMissionGeometryActor::GetMeasuredSpanCm() const
{
    return UWMMissionGeometryLibrary::MeasureWorldDistanceCm(GetMeasureStartWorld(), GetMeasureEndWorld());
}

FTransform AWMMissionGeometryActor::GetBuildZoneTransform() const
{
    return BuildZone ? BuildZone->GetComponentTransform() : GetActorTransform();
}

FVector AWMMissionGeometryActor::GetBuildZoneHalfExtent() const
{
    return BuildZone ? BuildZone->GetUnscaledBoxExtent() : BuildZoneHalfExtent;
}
