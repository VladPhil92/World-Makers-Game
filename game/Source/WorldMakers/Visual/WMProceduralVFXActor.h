#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Visual/WMVFXRuntime.h"
#include "WMProceduralVFXActor.generated.h"

class UProceduralMeshComponent;

UCLASS()
class WORLDMAKERS_API AWMProceduralVFXActor : public AActor
{
    GENERATED_BODY()

public:
    AWMProceduralVFXActor();

    virtual void Tick(float DeltaSeconds) override;

    bool InitializeEffect(const FWMVFXEvent& InEvent, const FWMVFXStyle& InStyle);
    FName GetSemanticEventId() const { return Event.EventId; }

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UProceduralMeshComponent> Mesh;

    FWMVFXEvent Event;
    FWMVFXStyle Style;
    float AgeSeconds = 0.0f;
    bool bInitialized = false;
};
