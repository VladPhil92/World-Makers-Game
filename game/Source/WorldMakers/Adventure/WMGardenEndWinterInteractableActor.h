#pragma once

#include "Adventure/WMGardenEndWinterExperience.h"
#include "CoreMinimal.h"
#include "Environment/WMInteractable.h"
#include "GameFramework/Actor.h"
#include "WMGardenEndWinterInteractableActor.generated.h"

class USphereComponent;
class UStaticMeshComponent;

UCLASS()
class WORLDMAKERS_API AWMGardenEndWinterInteractableActor : public AActor, public IWMInteractable
{
    GENERATED_BODY()
public:
    AWMGardenEndWinterInteractableActor();
    void Configure(FName InChapterId, const FWMGardenActionDefinition& Definition);

    virtual FName GetInteractionPointId() const override { return ActionId; }
    virtual FName GetInteractionPromptKey() const override { return PromptKey; }
    virtual FName GetInteractionMode() const override { return TEXT("garden-end-winter"); }
    virtual FName GetInteractionObservationId() const override { return NAME_None; }
    virtual FName GetInteractionActionId() const override { return ActionId; }
    virtual float GetInteractionRadiusCm() const override { return InteractionRadiusCm; }
    virtual FVector GetInteractionAnchorLocation() const override { return GetActorLocation(); }
    virtual bool CanInteract(const AActor* Interactor) const override;
    virtual bool Interact(AActor* Interactor) override;

    UFUNCTION(BlueprintPure, Category = "World Makers|Garden") bool IsResolved() const { return bResolved; }
    UFUNCTION(BlueprintPure, Category = "World Makers|Garden") bool IsAvailable() const { return bAvailable; }
    UFUNCTION(BlueprintCallable, Category = "World Makers|Garden") void MarkResolved(bool bInResolved = true);
    UFUNCTION(BlueprintCallable, Category = "World Makers|Garden") void SetAvailable(bool bInAvailable);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Garden") TObjectPtr<USphereComponent> FocusVolume;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Garden") TObjectPtr<UStaticMeshComponent> ProxyMesh;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Garden") FName ChapterId;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Garden") FName ActionId;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Garden") FName PromptKey;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Garden") FName InteractionVerb;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Makers|Garden", meta=(ClampMin="100.0",ClampMax="1200.0")) float InteractionRadiusCm=500.0f;

private:
    bool bResolved=false;
    bool bAvailable=false;
    int32 PrototypeInteractionStep=0;
};
