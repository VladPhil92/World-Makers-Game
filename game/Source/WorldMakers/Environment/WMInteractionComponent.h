#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WMInteractionComponent.generated.h"

class AActor;

UCLASS(ClassGroup = (WorldMakers), meta = (BlueprintSpawnableComponent))
class WORLDMAKERS_API UWMInteractionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UWMInteractionComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Makers|Interaction", meta = (ClampMin = "300.0", ClampMax = "2500.0"))
    float FocusMaxDistanceCm = 1200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Makers|Interaction", meta = (ClampMin = "0.50", ClampMax = "0.999"))
    float MinimumFocusDot = 0.93f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Makers|Interaction", meta = (ClampMin = "0.05", ClampMax = "1.00"))
    float FocusRefreshIntervalSeconds = 0.10f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Makers|Interaction", meta = (ClampMin = "0.10", ClampMax = "2.00"))
    float MinimumInteractionIntervalSeconds = 0.25f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Interaction")
    FName FocusedPointId;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Interaction")
    FName FocusedPromptKey;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Interaction")
    FName FocusedInteractionMode;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Interaction")
    FName FocusedObservationId;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Interaction")
    FName FocusedActionId;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Interaction")
    FName LastObservationId;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Interaction")
    FName LastActionId;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Interaction")
    bool bCanInteract = false;

    UFUNCTION(BlueprintCallable, Category = "World Makers|Interaction")
    void RefreshFocus();

    UFUNCTION(BlueprintCallable, Category = "World Makers|Interaction")
    bool TryInteractFocused();

    UFUNCTION(BlueprintPure, Category = "World Makers|Interaction")
    bool CanInteractNow() const { return bCanInteract && FocusedActor.IsValid(); }

    static bool IsFocusCandidate(
        const FVector& ViewLocation,
        const FVector& ViewForward,
        const FVector& TargetLocation,
        float MaxDistanceCm,
        float MinDot);

private:
    void ClearFocus();

    TWeakObjectPtr<AActor> FocusedActor;
    double LastInteractionTimeSeconds = -1000000.0;
};
