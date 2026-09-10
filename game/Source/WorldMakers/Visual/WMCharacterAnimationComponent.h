#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Visual/WMCharacterAnimationRuntime.h"
#include "WMCharacterAnimationComponent.generated.h"

class AWMPlayerCharacter;

/**
 * Bridges CharacterMovement/view state to the deterministic V5 animation runtime.
 * Procedural V4 art consumes the pose directly; a future AnimBP consumes the exposed state/read-model.
 */
UCLASS(ClassGroup = (WorldMakers), meta = (BlueprintSpawnableComponent))
class WORLDMAKERS_API UWMCharacterAnimationComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UWMCharacterAnimationComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    void TriggerAction(EWMCharacterInteractionAction Action, float DurationSeconds = 0.45f);

    UFUNCTION(BlueprintCallable, Category = "World Makers|Animation")
    bool TriggerActionById(FName ActionId, float DurationSeconds = 0.45f);

    UFUNCTION(BlueprintPure, Category = "World Makers|Animation")
    FName GetLocomotionStateId() const { return Runtime.GetLocomotionStateId(); }

    UFUNCTION(BlueprintPure, Category = "World Makers|Animation")
    FName GetInteractionActionId() const { return Runtime.GetInteractionActionId(); }

    UFUNCTION(BlueprintPure, Category = "World Makers|Animation")
    float GetSpeedAlpha() const { return Runtime.GetSpeedAlpha(); }

    UFUNCTION(BlueprintPure, Category = "World Makers|Animation")
    float GetInteractionAlpha() const { return Runtime.GetInteractionAlpha(); }

    UFUNCTION(BlueprintPure, Category = "World Makers|Animation")
    float GetStateAgeSeconds() const { return Runtime.GetStateAgeSeconds(); }

    UFUNCTION(BlueprintPure, Category = "World Makers|Animation")
    float GetAimYawDegrees() const { return Runtime.GetLookYawDegrees(); }

    UFUNCTION(BlueprintPure, Category = "World Makers|Animation")
    float GetAimPitchDegrees() const { return Runtime.GetLookPitchDegrees(); }

    UFUNCTION(BlueprintPure, Category = "World Makers|Animation")
    bool IsAirborne() const { return Runtime.IsAirborne(); }

    UFUNCTION(BlueprintPure, Category = "World Makers|Animation")
    bool IsProductionAnimBPExpected() const;

    const FWMCharacterAnimationPose& GetLastSourcePose() const { return LastSourcePose; }

private:
    FWMCharacterAnimationInput BuildInput(const AWMPlayerCharacter& Character, float DeltaSeconds) const;
    void ApplyProceduralPose(AWMPlayerCharacter& Character, const FWMCharacterAnimationPose& Pose) const;

    FWMCharacterAnimationRuntime Runtime;
    FWMCharacterAnimationPose LastSourcePose;
};
