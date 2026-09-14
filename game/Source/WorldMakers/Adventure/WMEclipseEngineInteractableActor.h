#pragma once

#include "Adventure/WMEclipseEngineExperience.h"
#include "CoreMinimal.h"
#include "Environment/WMInteractable.h"
#include "GameFramework/Actor.h"
#include "WMEclipseEngineInteractableActor.generated.h"

class USphereComponent;
class UStaticMeshComponent;

/** Source-proxy world-space affordance for an Eclipse Engine action. */
UCLASS()
class WORLDMAKERS_API AWMEclipseEngineInteractableActor : public AActor, public IWMInteractable
{
    GENERATED_BODY()

public:
    AWMEclipseEngineInteractableActor();

    void Configure(FName InChapterId, const FWMEclipseActionDefinition& Definition);

    virtual FName GetInteractionPointId() const override { return ActionId; }
    virtual FName GetInteractionPromptKey() const override { return PromptKey; }
    virtual FName GetInteractionMode() const override { return TEXT("eclipse-engine"); }
    virtual FName GetInteractionObservationId() const override { return NAME_None; }
    virtual FName GetInteractionActionId() const override { return ActionId; }
    virtual float GetInteractionRadiusCm() const override { return InteractionRadiusCm; }
    virtual FVector GetInteractionAnchorLocation() const override { return GetActorLocation(); }
    virtual bool CanInteract(const AActor* Interactor) const override;
    virtual bool Interact(AActor* Interactor) override;

    UFUNCTION(BlueprintPure, Category = "World Makers|Eclipse")
    bool IsResolved() const { return bResolved; }

    UFUNCTION(BlueprintPure, Category = "World Makers|Eclipse")
    bool IsAvailable() const { return bAvailable; }

    UFUNCTION(BlueprintCallable, Category = "World Makers|Eclipse")
    void MarkResolved(bool bInResolved = true);

    UFUNCTION(BlueprintCallable, Category = "World Makers|Eclipse")
    void SetAvailable(bool bInAvailable);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Eclipse")
    TObjectPtr<USphereComponent> FocusVolume;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Eclipse")
    TObjectPtr<UStaticMeshComponent> ProxyMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Eclipse")
    FName ChapterId;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Eclipse")
    FName ActionId;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Eclipse")
    FName PromptKey;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Eclipse")
    FName InteractionVerb;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Makers|Eclipse", meta = (ClampMin = "100.0", ClampMax = "1200.0"))
    float InteractionRadiusCm = 500.0f;

    static const FName EclipseTargetTag;

private:
    bool bResolved = false;
    bool bAvailable = false;
    int32 PrototypeInteractionStep = 0;
};
