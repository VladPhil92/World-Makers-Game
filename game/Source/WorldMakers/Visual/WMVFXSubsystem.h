#pragma once

#include "CoreMinimal.h"
#include "Building/WMBuildWorldStateSubsystem.h"
#include "Environment/WMEnvironmentStateTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "Visual/WMVFXRuntime.h"
#include "WMVFXSubsystem.generated.h"

class AWMProceduralVFXActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FWMVFXAcceptedSignature, FName, EventId, FVector, LocationCm, float, Intensity);

UCLASS()
class WORLDMAKERS_API UWMVFXSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    bool EmitEvent(const FWMVFXEvent& Event);

    UFUNCTION(BlueprintCallable, Category = "World Makers|VFX")
    bool EmitSemanticEvent(FName EventId, FVector LocationCm, float Intensity = 1.0f, FVector Direction = FVector::ForwardVector, float DurationSeconds = 0.65f);

    UFUNCTION(BlueprintCallable, Category = "World Makers|VFX|Accessibility")
    void SetReducedMotion(bool bEnabled) { bReducedMotion = bEnabled; }

    UFUNCTION(BlueprintPure, Category = "World Makers|VFX|Accessibility")
    bool IsReducedMotion() const { return bReducedMotion; }

    UFUNCTION(BlueprintPure, Category = "World Makers|VFX")
    int32 GetActiveProxyEffectCount() const;

    UFUNCTION(BlueprintPure, Category = "World Makers|VFX")
    FName GetLastAcceptedEventId() const { return LastAcceptedEventId; }

    /** Presentation-only notification emitted after a VFX event has passed all V6 validation and budget gates. */
    UPROPERTY(BlueprintAssignable, Category = "World Makers|VFX")
    FWMVFXAcceptedSignature OnVFXAccepted;

private:
    UFUNCTION()
    void HandleBuildWorldChanged(int32 Revision);

    UFUNCTION()
    void HandleEnvironmentStateChanged(FWMEnvironmentStateSnapshot Snapshot);

    FWMVFXBudget ResolveBudget() const;
    void PruneExpiredEffects();

    FWMVFXRuntime Runtime;
    TArray<TWeakObjectPtr<AWMProceduralVFXActor>> ActiveProxyEffects;
    TArray<FWMPlacedBuildPieceSnapshot> LastBuildSnapshot;
    FWMEnvironmentStateSnapshot LastEnvironmentSnapshot;
    FName LastAcceptedEventId = NAME_None;
    bool bHasEnvironmentSnapshot = false;
    bool bReducedMotion = false;
};
