#include "Environment/WMEnvironmentActionActor.h"

#include "Components/SphereComponent.h"
#include "Environment/WMEnvironmentStateSubsystem.h"

const FName AWMEnvironmentActionActor::EnvironmentActionTargetTag(TEXT("WM_EnvironmentAction"));

AWMEnvironmentActionActor::AWMEnvironmentActionActor()
{
    PrimaryActorTick.bCanEverTick = false;
    SetReplicates(false);
    Tags.Add(EnvironmentActionTargetTag);

    FocusVolume = CreateDefaultSubobject<USphereComponent>(TEXT("FocusVolume"));
    SetRootComponent(FocusVolume);
    FocusVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    FocusVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
    FocusVolume->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    FocusVolume->SetGenerateOverlapEvents(false);
    FocusVolume->SetSphereRadius(FocusRadiusCm);
}

void AWMEnvironmentActionActor::Configure(const FWMEnvironmentActionDefinition& Definition)
{
    TargetId = Definition.TargetId;
    PromptKey = Definition.PromptKey;
    ActionId = Definition.ActionId;
    InteractionRadiusCm = Definition.InteractionRadiusCm;
    FocusRadiusCm = Definition.FocusRadiusCm;
    SetActorLocation(Definition.LocationCm);
    if (FocusVolume)
    {
        FocusVolume->SetSphereRadius(FocusRadiusCm);
    }
}

bool AWMEnvironmentActionActor::CanInteract(const AActor* Interactor) const
{
    if (!Interactor || TargetId.IsNone() || ActionId.IsNone() || InteractionRadiusCm <= 0.0f)
    {
        return false;
    }

    if (FVector::DistSquared(Interactor->GetActorLocation(), GetActorLocation()) > FMath::Square(InteractionRadiusCm))
    {
        return false;
    }

    UWorld* World = GetWorld();
    const UWMEnvironmentStateSubsystem* EnvironmentState = World ? World->GetSubsystem<UWMEnvironmentStateSubsystem>() : nullptr;
    return EnvironmentState && EnvironmentState->CanApplyAction(ActionId);
}

bool AWMEnvironmentActionActor::Interact(AActor* Interactor)
{
    if (!CanInteract(Interactor))
    {
        return false;
    }

    UWorld* World = GetWorld();
    UWMEnvironmentStateSubsystem* EnvironmentState = World ? World->GetSubsystem<UWMEnvironmentStateSubsystem>() : nullptr;
    return EnvironmentState && EnvironmentState->ApplyAction(ActionId);
}
