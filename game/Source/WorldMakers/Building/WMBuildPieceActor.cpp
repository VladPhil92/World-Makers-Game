#include "Building/WMBuildPieceActor.h"

#include "Building/WMBuildCatalogSettings.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

const FName AWMBuildPieceActor::PlacedBuildTag(TEXT("WM_PlayerBuild"));

AWMBuildPieceActor::AWMBuildPieceActor()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(SceneRoot);
    Mesh->SetCollisionProfileName(TEXT("BlockAll"));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        Mesh->SetStaticMesh(CubeMesh.Object);
    }
}

void AWMBuildPieceActor::ApplyPieceSpec(const FWMBuildPieceSpec& Spec)
{
    if (!Spec.IsSane())
    {
        return;
    }

    PieceId = Spec.PieceId;
    PieceDimensionsCm = Spec.DimensionsCm;
    Mesh->SetRelativeScale3D(Spec.DimensionsCm / 100.0f);
}

void AWMBuildPieceActor::SetPreviewState(const bool bPreview)
{
    bIsPreview = bPreview;
    SetActorEnableCollision(!bPreview);
    Mesh->SetCollisionEnabled(bPreview ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
    Mesh->SetCastShadow(!bPreview);
    Mesh->SetRenderCustomDepth(bPreview);

    if (bPreview)
    {
        Tags.Remove(PlacedBuildTag);
        SetPreviewValidity(false);
    }
    else if (!Tags.Contains(PlacedBuildTag))
    {
        Tags.Add(PlacedBuildTag);
    }
}

void AWMBuildPieceActor::SetPreviewValidity(const bool bValid)
{
    bPreviewPlacementValid = bValid;
    if (bIsPreview)
    {
        Mesh->SetRenderCustomDepth(true);
        Mesh->SetCustomDepthStencilValue(bValid ? 1 : 2);
    }
}
