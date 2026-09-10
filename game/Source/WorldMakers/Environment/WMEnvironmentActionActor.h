#pragma once

#include "CoreMinimal.h"
#include "Environment/WMEnvironmentStateTypes.h"
#include "Environment/WMInteractable.h"
#include "GameFramework/Actor.h"
#include "WMEnvironmentActionActor.generated.h"

class USphereComponent;

UCLASS()
class WORLDMAKERS_API AWMEnvironmentActionActor : public AActor, public IWMInteractable
{
    GENERATED_BODY()

public:
    AWMEnvironmentActionActor();

    void Configure(const FWMEnvironmentActionDefinition& Definition);

    virtual FName GetInteractionPointId() const override { return TargetId; }
    virtual FName GetInteractionPromptKey() const override { return PromptKey; }
    virtual FName GetInteractionMode() const override { return TEXT("care"); }
    virtual FName GetInteractionObservationId() const override { return NAME_None; }
    virtual FName GetInteractionActionId() const override { return ActionId; }
    virtual float GetInteractionRadiusCm() const override { return InteractionRadiusCm; }
    virtual FVector GetInteractionAnchorLocation() const override { return GetActorLocation(); }
    virtual bool CanInteract(const AActor* Interactor) const override;
    virtual bool Interact(AActor* Interactor) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Environment")
    TObjectPtr<USphereComponent> FocusVolume;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Environment")
    FName TargetId;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Environment")
    FName PromptKey;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Environment")
    FName ActionId;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Environment")
    float InteractionRadiusCm = 450.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Environment")
    float FocusRadiusCm = 120.0f;

    static const FName EnvironmentActionTargetTag;
};
