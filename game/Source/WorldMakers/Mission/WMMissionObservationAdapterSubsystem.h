#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WMMissionObservationAdapterSubsystem.generated.h"

class UWMBiomeRuntimeSubsystem;

/**
 * One-way adapter from Environment observation events into Mission evidence.
 * Environment never imports Mission code; the adapter owns the dependency edge.
 */
UCLASS()
class WORLDMAKERS_API UWMMissionObservationAdapterSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

private:
    UFUNCTION()
    void HandleObservationRegistered(FName PointId, FName ObservationId);

    TWeakObjectPtr<UWMBiomeRuntimeSubsystem> BoundBiomeRuntime;
};
