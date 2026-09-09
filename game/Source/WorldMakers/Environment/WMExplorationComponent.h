#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WMExplorationComponent.generated.h"

UCLASS(ClassGroup = (WorldMakers), meta = (BlueprintSpawnableComponent))
class WORLDMAKERS_API UWMExplorationComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UWMExplorationComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Makers|Exploration", meta = (ClampMin = "0.10", ClampMax = "2.00"))
    float ObservationIntervalSeconds = 0.25f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Exploration")
    FName ActiveBiomeId;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Exploration")
    FName CurrentZoneId;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Exploration")
    TArray<FName> LastNewDiscoveryIds;

    UFUNCTION(BlueprintCallable, Category = "World Makers|Exploration")
    TArray<FName> ObserveNow();
};
