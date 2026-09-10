#pragma once

#include "CoreMinimal.h"
#include "Environment/WMBiomeTypes.h"
#include "Environment/WMInteractable.h"
#include "GameFramework/Actor.h"
#include "WMEnvironmentalInteractableActor.generated.h"

class USphereComponent;

UCLASS()
class WORLDMAKERS_API AWMEnvironmentalInteractableActor : public AActor, public IWMInteractable
{
    GENERATED_BODY()

public:
    AWMEnvironmentalInteractableActor();

    void Configure(const FWMPointOfInterestDefinition& Definition, const FVector& BiomeOriginCm);

    virtual FName GetInteractionPointId() const override { return PointId; }
    virtual FName GetInteractionPromptKey() const override { return PromptKey; }
    virtual FName GetInteractionMode() const override { return InteractionMode; }
    virtual FName GetInteractionObservationId() const override { return ObservationId; }
    virtual FName GetInteractionActionId() const override { return NAME_None; }
    virtual float GetInteractionRadiusCm() const override { return InteractionRadiusCm; }
    virtual FVector GetInteractionAnchorLocation() const override { return GetActorLocation(); }
    virtual bool CanInteract(const AActor* Interactor) const override;
    virtual bool Interact(AActor* Interactor) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Interaction")
    TObjectPtr<USphereComponent> FocusVolume;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Interaction")
    FName PointId;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Interaction")
    FName PromptKey;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Interaction")
    FName InteractionMode;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Interaction")
    FName ObservationId;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Interaction")
    float InteractionRadiusCm = 350.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Interaction")
    float FocusRadiusCm = 90.0f;

    static const FName InteractionTargetTag;
};
