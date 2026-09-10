#pragma once

#include "CoreMinimal.h"
#include "Environment/WMEnvironmentStateTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "WMEnvironmentStateSubsystem.generated.h"

class AWMEnvironmentActionActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWMEnvironmentStateChangedSignature, FWMEnvironmentStateSnapshot, Snapshot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FWMEnvironmentReactionSignature, FName, ActionId, FName, ReactionId);

UCLASS()
class WORLDMAKERS_API UWMEnvironmentStateSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "World Makers|Environment")
    bool ReloadAndActivatePrototypeProfile();

    UFUNCTION(BlueprintCallable, Category = "World Makers|Environment")
    bool ReloadProfileCatalog();

    UFUNCTION(BlueprintCallable, Category = "World Makers|Environment")
    bool ActivateProfile(FName BiomeId);

    UFUNCTION(BlueprintPure, Category = "World Makers|Environment")
    FName GetActiveBiomeId() const { return Model.GetSnapshot().BiomeId; }

    UFUNCTION(BlueprintPure, Category = "World Makers|Environment")
    FWMEnvironmentStateSnapshot GetStateSnapshot() const { return Model.GetSnapshot(); }

    UFUNCTION(BlueprintPure, Category = "World Makers|Environment")
    bool CanApplyAction(FName ActionId) const { return Model.CanApplyAction(ActionId); }

    UFUNCTION(BlueprintPure, Category = "World Makers|Environment")
    int32 GetAppliedActionCount(FName ActionId) const { return Model.GetAppliedCount(ActionId); }

    UFUNCTION(BlueprintCallable, Category = "World Makers|Environment")
    bool ApplyAction(FName ActionId);

    /** Native trusted-domain entrypoint for deterministic build/ecosystem effects. Not exposed to Blueprint. */
    bool ApplyTrustedEffect(FName EffectId, const FWMEnvironmentStateDelta& Delta, int32 MaxApplications = 1);

    UFUNCTION(BlueprintCallable, Category = "World Makers|Environment")
    void EnsureActionTargets();

    UFUNCTION(BlueprintPure, Category = "World Makers|Environment")
    int32 GetActionTargetCount() const;

    UPROPERTY(BlueprintAssignable, Category = "World Makers|Environment")
    FWMEnvironmentStateChangedSignature OnEnvironmentStateChanged;

    UPROPERTY(BlueprintAssignable, Category = "World Makers|Environment")
    FWMEnvironmentReactionSignature OnEnvironmentReaction;

private:
    bool BroadcastAcceptedMutation(FName CausalId, FName PreviousReactionId);
    void ClearActionTargets();

    TMap<FName, FWMEnvironmentStateDefinition> ProfileCatalog;
    FWMEnvironmentStateModel Model;
    TArray<TWeakObjectPtr<AWMEnvironmentActionActor>> ActionTargets;
};
