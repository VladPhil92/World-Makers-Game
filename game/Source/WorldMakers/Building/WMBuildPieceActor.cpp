#include "Building/WMBuildPieceActor.h"

#include "Building/WMBuildCatalogSettings.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "Visual/WMStylizedSurfaceLibrary.h"

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
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> ProxySurfaceMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (CubeMesh.Succeeded())
    {
        Mesh->SetStaticMesh(CubeMesh.Object);
    }
    if (ProxySurfaceMaterial.Succeeded())
    {
        Mesh->SetMaterial(0, ProxySurfaceMaterial.Object);
    }
}

void AWMBuildPieceActor::ApplyPieceSpec(const FWMBuildPieceSpec& Spec)
{
    if (!Spec.IsSane())
    {
        return;
    }

    PieceId = Spec.PieceId;
    PieceCategory = Spec.Category;
    PieceDimensionsCm = Spec.DimensionsCm;
    Mesh->SetRelativeScale3D(Spec.DimensionsCm / 100.0f);

    if (!bIsPreview)
    {
        ApplyPlacedSurface();
    }
}

void AWMBuildPieceActor::ApplyPlacedSurface()
{
    const bool bEcoSurface = PieceCategory.Equals(TEXT("Eco"), ESearchCase::IgnoreCase);
    UWMStylizedSurfaceLibrary::ApplyConfiguredSurface(
        Mesh,
        bEcoSurface ? EWMStylizedSurfaceRole::BuildEco : EWMStylizedSurfaceRole::BuildNeutral);
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
    else
    {
        ApplyPlacedSurface();
        if (!Tags.Contains(PlacedBuildTag))
        {
            Tags.Add(PlacedBuildTag);
        }
    }
}

void AWMBuildPieceActor::SetPreviewValidity(const bool bValid)
{
    bPreviewPlacementValid = bValid;
    if (bIsPreview)
    {
        // Color is supplemental only. Stencil identity remains independent so valid/invalid state is not color-only.
        UWMStylizedSurfaceLibrary::ApplyConfiguredSurface(
            Mesh,
            bValid ? EWMStylizedSurfaceRole::PreviewValid : EWMStylizedSurfaceRole::PreviewInvalid);
        Mesh->SetRenderCustomDepth(true);
        Mesh->SetCustomDepthStencilValue(bValid ? 1 : 2);
    }
}
