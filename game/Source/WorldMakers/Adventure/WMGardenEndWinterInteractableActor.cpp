#include "Adventure/WMGardenEndWinterInteractableActor.h"

#include "Adventure/WMGardenEndWinterExperienceSubsystem.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

AWMGardenEndWinterInteractableActor::AWMGardenEndWinterInteractableActor()
{
    PrimaryActorTick.bCanEverTick=false;
    SetReplicates(false);
    Tags.Add(TEXT("WM_GardenEndWinterTarget"));

    FocusVolume=CreateDefaultSubobject<USphereComponent>(TEXT("FocusVolume"));
    SetRootComponent(FocusVolume);
    FocusVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    FocusVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
    FocusVolume->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
    FocusVolume->SetGenerateOverlapEvents(false);
    FocusVolume->SetSphereRadius(115.0f);

    ProxyMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProxyMesh"));
    ProxyMesh->SetupAttachment(FocusVolume);
    ProxyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ProxyMesh->SetVisibility(false,true);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if(Mesh.Succeeded()) ProxyMesh->SetStaticMesh(Mesh.Object);
}

void AWMGardenEndWinterInteractableActor::Configure(const FName InChapterId,const FWMGardenActionDefinition& Definition)
{
    ChapterId=InChapterId;
    ActionId=Definition.ActionId;
    PromptKey=Definition.PromptKey;
    InteractionVerb=Definition.InteractionVerb;
    SetActorLocation(Definition.PrototypeLocationCm);
    PrototypeInteractionStep=0;
    bResolved=false;
    bAvailable=false;
    if(ProxyMesh)
    {
        FVector Scale(0.65f,0.65f,0.65f);
        if(InteractionVerb==TEXT("route")||InteractionVerb==TEXT("tune")) Scale=FVector(0.85f,0.35f,0.35f);
        else if(InteractionVerb==TEXT("observe")||InteractionVerb==TEXT("deliberate")||InteractionVerb==TEXT("rethink")) Scale=FVector(0.55f,0.55f,0.85f);
        ProxyMesh->SetRelativeScale3D(Scale);
    }
    SetAvailable(false);
}

bool AWMGardenEndWinterInteractableActor::CanInteract(const AActor* Interactor) const
{
    if(!Interactor||!bAvailable||bResolved||ActionId.IsNone()||FVector::DistSquared(Interactor->GetActorLocation(),GetActorLocation())>FMath::Square(InteractionRadiusCm)) return false;
    const UWorld* World=GetWorld();
    const UWMGardenEndWinterExperienceSubsystem* Garden=World?World->GetSubsystem<UWMGardenEndWinterExperienceSubsystem>():nullptr;
    return Garden&&Garden->CanBeginAction(ActionId);
}

bool AWMGardenEndWinterInteractableActor::Interact(AActor* Interactor)
{
    if(!CanInteract(Interactor)) return false;
    UWorld* World=GetWorld();
    UWMGardenEndWinterExperienceSubsystem* Garden=World?World->GetSubsystem<UWMGardenEndWinterExperienceSubsystem>():nullptr;
    if(!Garden||!Garden->BeginAction(ActionId)) return false;
    if(Garden->IsWorldStateAction(ActionId)) return Garden->ResolveTrustedAction(ActionId);

    PrototypeInteractionStep=(PrototypeInteractionStep%8)+1;
    if(ProxyMesh)
    {
        ProxyMesh->AddLocalRotation(FRotator(InteractionVerb==TEXT("tune")?7.5f:0.0f,18.0f,InteractionVerb==TEXT("route")?6.0f:0.0f));
    }
    Garden->AdvancePrototypeMechanic(ActionId,PrototypeInteractionStep);
    return true;
}

void AWMGardenEndWinterInteractableActor::MarkResolved(const bool bInResolved)
{
    bResolved=bInResolved;
    SetAvailable(bAvailable);
}

void AWMGardenEndWinterInteractableActor::SetAvailable(const bool bInAvailable)
{
    bAvailable=bInAvailable;
    const bool bShow=bAvailable&&!bResolved;
    if(ProxyMesh) ProxyMesh->SetVisibility(bShow,true);
    if(FocusVolume) FocusVolume->SetCollisionEnabled(bShow?ECollisionEnabled::QueryOnly:ECollisionEnabled::NoCollision);
}
