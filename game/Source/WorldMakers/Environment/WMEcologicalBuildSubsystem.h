#pragma once

#include "CoreMinimal.h"
#include "Environment/WMEcologicalBuildTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "WMEcologicalBuildSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FWMEcologicalInterventionCompletedSignature,
    FName, InterventionId,
    FName, RewardId);

UCLASS()
class WORLDMAKERS_API UWMEcologicalBuildSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "World Makers|Environment|Building")
    bool ReloadAndActivatePrototypeDefinition();

    UFUNCTION(BlueprintCallable, Category = "World Makers|Environment|Building")
    bool ReloadDefinitionCatalog();

    UFUNCTION(BlueprintCallable, Category = "World Makers|Environment|Building")
    bool ActivateDefinition(FName BiomeId);

    UFUNCTION(BlueprintCallable, Category = "World Makers|Environment|Building")
    bool EvaluateCurrentBuild();

    UFUNCTION(BlueprintPure, Category = "World Makers|Environment|Building")
    TArray<FWMEcologicalBuildInterventionReadModel> GetInterventionReadModel() const;

    UFUNCTION(BlueprintPure, Category = "World Makers|Environment|Building")
    TArray<FName> GetCompletedInterventionIds() const { return Progress.GetCompletedInterventionIds(); }

    UFUNCTION(BlueprintPure, Category = "World Makers|Environment|Building")
    FName GetNextOpenInterventionId() const;

    UPROPERTY(BlueprintAssignable, Category = "World Makers|Environment|Building")
    FWMEcologicalInterventionCompletedSignature OnInterventionCompleted;

private:
    UFUNCTION()
    void HandleBuildWorldChanged(int32 Revision);

    const FWMEcologicalBuildDefinition* GetActiveDefinition() const;

    TMap<FName, FWMEcologicalBuildDefinition> DefinitionCatalog;
    FName ActiveBiomeId;
    FWMEcologicalBuildProgressModel Progress;
};
