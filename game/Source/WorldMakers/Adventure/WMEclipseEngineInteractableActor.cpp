#include "Adventure/WMEclipseEngineInteractableActor.h"

#include "Adventure/WMEclipseEngineExperienceSubsystem.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

const FName AWMEclipseEngineInteractableActor::EclipseTargetTag(TEXT("WM_EclipseEngineTarget"));

AWMEclipseEngineInteractableActor::AWMEclipseEngineInteractableActor()
{
    PrimaryActorTick.bCanEverTick = false;
    SetReplicates(false);
    Tags.Add(EclipseTargetTag);

    FocusVolume = CreateDefaultSubobject<USphereComponent>(TEXT("FocusVolume"));
    SetRootComponent(FocusVolume);
    FocusVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    FocusVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
    FocusVolume->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    FocusVolume->SetGenerateOverlapEvents(false);
    FocusVolume->SetSphereRadius(110.0f);

    ProxyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProxyMesh"));
    ProxyMesh->SetupAttachment(FocusVolume);
    ProxyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ProxyMesh->SetRelativeScale3D(FVector(0.65f, 0.65f, 1.20f));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (Mesh.Succeeded()) ProxyMesh->SetStaticMesh(Mesh.Object);
}

void AWMEclipseEngineInteractableActor::Configure(const FWMEclipseActionDefinition& Definition)
{
    ActionId = Definition.ActionId;
    PromptKey = Definition.PromptKey;
    InteractionVerb = Definition.InteractionVerb;
    SetActorLocation(Definition.PrototypeLocationCm);
    bResolved = false;

    if (ProxyMesh)
    {
        if (Definition.InteractionVerb == TEXT("build") || Definition.InteractionVerb == TEXT("align") || Definition.InteractionVerb == TEXT("rotate"))
            ProxyMesh->SetRelativeScale3D(FVector(0.85f, 0.30f, 1.35f));
        else if (Definition.InteractionVerb == TEXT("interpret") || Definition.InteractionVerb == TEXT("decode") || Definition.InteractionVerb == TEXT("communicate"))
            ProxyMesh->SetRelativeScale3D(FVector(0.70f, 0.18f, 0.95f));
        else
            ProxyMesh->SetRelativeScale3D(FVector(0.55f, 0.55f, 1.10f));
    }
}

bool AWMEclipseEngineInteractableActor::CanInteract(const AActor* Interactor) const
{
    if (!Interactor || bResolved || ActionId.IsNone() || FVector::DistSquared(Interactor->GetActorLocation(), GetActorLocation()) > FMath::Square(InteractionRadiusCm)) return false;
    const UWorld* World = GetWorld();
    const UWMEclipseEngineExperienceSubsystem* Experience = World ? World->GetSubsystem<UWMEclipseEngineExperienceSubsystem>() : nullptr;
    return Experience && Experience->CanBeginAction(ActionId);
}

bool AWMEclipseEngineInteractableActor::Interact(AActor* Interactor)
{
    if (!CanInteract(Interactor)) return false;
    UWorld* World = GetWorld();
    UWMEclipseEngineExperienceSubsystem* Experience = World ? World->GetSubsystem<UWMEclipseEngineExperienceSubsystem>() : nullptr;
    if (!Experience || !Experience->BeginAction(ActionId)) return false;

    // Causal world-state controls are allowed to resolve only after the chapter's underlying mission is complete.
    // Evidence-bearing puzzle affordances merely enter their interaction mode here; their mechanic must call
    // ResolveTrustedAction after validating the actual player result.
    if (Experience->IsWorldStateAction(ActionId))
    {
        const bool bSucceeded = Experience->ResolveTrustedAction(ActionId);
        if (bSucceeded) MarkResolved();
        return bSucceeded;
    }
    return true;
}

void AWMEclipseEngineInteractableActor::MarkResolved(const bool bInResolved)
{
    bResolved = bInResolved;
    if (ProxyMesh) ProxyMesh->SetVisibility(!bResolved, true);
    if (FocusVolume) FocusVolume->SetCollisionEnabled(bResolved ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryOnly);
}
