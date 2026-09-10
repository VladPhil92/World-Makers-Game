#include "Environment/WMEnvironmentalInteractableActor.h"

#include "Components/SphereComponent.h"
#include "Environment/WMBiomeRuntimeSubsystem.h"

const FName AWMEnvironmentalInteractableActor::InteractionTargetTag(TEXT("WM_EnvironmentalInteractable"));

AWMEnvironmentalInteractableActor::AWMEnvironmentalInteractableActor()
{
    PrimaryActorTick.bCanEverTick = false;
    SetReplicates(false);
    Tags.Add(InteractionTargetTag);

    FocusVolume = CreateDefaultSubobject<USphereComponent>(TEXT("FocusVolume"));
    SetRootComponent(FocusVolume);
    FocusVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    FocusVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
    FocusVolume->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    FocusVolume->SetGenerateOverlapEvents(false);
    FocusVolume->SetSphereRadius(FocusRadiusCm);
}

void AWMEnvironmentalInteractableActor::Configure(
    const FWMPointOfInterestDefinition& Definition,
    const FVector& BiomeOriginCm)
{
    PointId = Definition.PointId;
    PromptKey = Definition.PromptKey;
    ObservationId = Definition.ObservationId;
    InteractionRadiusCm = Definition.InteractionRadiusCm;
    FocusRadiusCm = Definition.FocusRadiusCm;
    SetActorLocation(BiomeOriginCm + Definition.LocationCm);
    if (FocusVolume)
    {
        FocusVolume->SetSphereRadius(FocusRadiusCm);
    }
}

bool AWMEnvironmentalInteractableActor::CanInteract(const AActor* Interactor) const
{
    if (!Interactor || PointId.IsNone() || ObservationId.IsNone() || InteractionRadiusCm <= 0.0f)
    {
        return false;
    }
    return FVector::DistSquared(Interactor->GetActorLocation(), GetActorLocation()) <= FMath::Square(InteractionRadiusCm);
}

bool AWMEnvironmentalInteractableActor::Interact(AActor* Interactor)
{
    if (!CanInteract(Interactor))
    {
        return false;
    }

    UWorld* World = GetWorld();
    UWMBiomeRuntimeSubsystem* Biomes = World ? World->GetSubsystem<UWMBiomeRuntimeSubsystem>() : nullptr;
    if (!Biomes)
    {
        return false;
    }

    FName RegisteredObservationId;
    return Biomes->RegisterDeliberateInteraction(PointId, Interactor->GetActorLocation(), RegisteredObservationId);
}
