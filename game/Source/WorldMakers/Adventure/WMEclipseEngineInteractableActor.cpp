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
    FocusVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    FocusVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
    FocusVolume->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    FocusVolume->SetGenerateOverlapEvents(false);
    FocusVolume->SetSphereRadius(110.0f);

    ProxyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProxyMesh"));
    ProxyMesh->SetupAttachment(FocusVolume);
    ProxyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ProxyMesh->SetVisibility(false, true);
    ProxyMesh->SetRelativeScale3D(FVector(0.65f, 0.65f, 1.20f));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (Mesh.Succeeded()) ProxyMesh->SetStaticMesh(Mesh.Object);
}

void AWMEclipseEngineInteractableActor::Configure(const FName InChapterId, const FWMEclipseActionDefinition& Definition)
{
    ChapterId = InChapterId;
    ActionId = Definition.ActionId;
    PromptKey = Definition.PromptKey;
    InteractionVerb = Definition.InteractionVerb;
    SetActorLocation(Definition.PrototypeLocationCm);
    bResolved = false;
    bAvailable = false;
    PrototypeInteractionStep = 0;

    if (ProxyMesh)
    {
        if (Definition.InteractionVerb == TEXT("build") || Definition.InteractionVerb == TEXT("align") || Definition.InteractionVerb == TEXT("rotate"))
            ProxyMesh->SetRelativeScale3D(FVector(0.85f, 0.30f, 1.35f));
        else if (Definition.InteractionVerb == TEXT("interpret") || Definition.InteractionVerb == TEXT("decode") || Definition.InteractionVerb == TEXT("communicate"))
            ProxyMesh->SetRelativeScale3D(FVector(0.70f, 0.18f, 0.95f));
        else
            ProxyMesh->SetRelativeScale3D(FVector(0.55f, 0.55f, 1.10f));
    }
    SetAvailable(false);
}

bool AWMEclipseEngineInteractableActor::CanInteract(const AActor* Interactor) const
{
    if (!Interactor || !bAvailable || bResolved || ActionId.IsNone() ||
        FVector::DistSquared(Interactor->GetActorLocation(), GetActorLocation()) > FMath::Square(InteractionRadiusCm)) return false;
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

    if (Experience->IsWorldStateAction(ActionId))
    {
        return Experience->ResolveTrustedAction(ActionId);
    }

    // Source-proxy tactile tuning: every accepted interaction advances a bounded world-mechanism state.
    // The click itself is never evidence. C++ mechanic validators decide whether the resulting state is solved.
    PrototypeInteractionStep = (PrototypeInteractionStep % 8) + 1;
    if (ProxyMesh)
    {
        ProxyMesh->AddLocalRotation(FRotator(0.0f, 22.5f, InteractionVerb == TEXT("rotate") ? 12.0f : 0.0f));
        const float Pulse = 1.0f + 0.025f * static_cast<float>((PrototypeInteractionStep % 3) - 1);
        ProxyMesh->SetRelativeScale3D(ProxyMesh->GetRelativeScale3D().GetSafeNormal() * FMath::Max(0.25f, ProxyMesh->GetRelativeScale3D().Size()) * Pulse);
    }

    Experience->AdvancePrototypeMechanic(ActionId, PrototypeInteractionStep);
    return true;
}

void AWMEclipseEngineInteractableActor::MarkResolved(const bool bInResolved)
{
    bResolved = bInResolved;
    SetAvailable(bAvailable);
}

void AWMEclipseEngineInteractableActor::SetAvailable(const bool bInAvailable)
{
    bAvailable = bInAvailable;
    const bool bVisibleAndInteractive = bAvailable && !bResolved;
    if (ProxyMesh) ProxyMesh->SetVisibility(bVisibleAndInteractive, true);
    if (FocusVolume) FocusVolume->SetCollisionEnabled(bVisibleAndInteractive ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
}
