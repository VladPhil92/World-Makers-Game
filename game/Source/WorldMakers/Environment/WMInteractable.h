#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WMInteractable.generated.h"

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UWMInteractable : public UInterface
{
    GENERATED_BODY()
};

class WORLDMAKERS_API IWMInteractable
{
    GENERATED_BODY()

public:
    virtual FName GetInteractionPointId() const = 0;
    virtual FName GetInteractionPromptKey() const = 0;
    virtual FName GetInteractionObservationId() const = 0;
    virtual float GetInteractionRadiusCm() const = 0;
    virtual FVector GetInteractionAnchorLocation() const = 0;
    virtual bool CanInteract(const AActor* Interactor) const = 0;
    virtual bool Interact(AActor* Interactor) = 0;
};
