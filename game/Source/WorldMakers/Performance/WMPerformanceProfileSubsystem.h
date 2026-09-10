#pragma once

#include "CoreMinimal.h"
#include "Performance/WMPerformanceProfileTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "WMPerformanceProfileSubsystem.generated.h"

UCLASS()
class WORLDMAKERS_API UWMPerformanceProfileSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "World Makers|Performance")
    bool ReloadProfiles();

    UFUNCTION(BlueprintCallable, Category = "World Makers|Performance")
    bool ApplyProfile(FName ProfileId);

    UFUNCTION(BlueprintPure, Category = "World Makers|Performance")
    TArray<FName> GetAvailableProfileIds() const;

    UFUNCTION(BlueprintPure, Category = "World Makers|Performance")
    FName GetActiveProfileId() const { return ActiveProfileId; }

    UFUNCTION(BlueprintPure, Category = "World Makers|Performance")
    FWMPerformanceProfileDefinition GetActiveProfile() const;

    const FWMPerformanceProfileDefinition* FindProfile(FName ProfileId) const;

private:
    FName ResolvePlatformDefaultProfileId() const;
    bool ApplyAllowlistedScalability(const FWMScalabilityProfile& Scalability) const;

    FWMPerformanceProfileCatalog Catalog;
    FName ActiveProfileId;
};
