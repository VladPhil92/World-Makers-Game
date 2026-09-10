#pragma once

#include "CoreMinimal.h"
#include "Science/WMScienceSimulationCore.h"
#include "Subsystems/WorldSubsystem.h"
#include "WMScienceSimulationSubsystem.generated.h"

UCLASS()
class WORLDMAKERS_API UWMScienceSimulationSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "World Makers|Science")
    bool ReloadScienceCatalog();

    UFUNCTION(BlueprintPure, Category = "World Makers|Science")
    bool IsScienceCatalogLoaded() const { return bCatalogLoaded; }

    UFUNCTION(BlueprintPure, Category = "World Makers|Science")
    int32 GetSubstanceCount() const { return Catalog.Substances.Num(); }

    UFUNCTION(BlueprintPure, Category = "World Makers|Science")
    int32 GetReactionCount() const { return Catalog.Reactions.Num(); }

    UFUNCTION(BlueprintPure, Category = "World Makers|Science")
    int32 GetPlantSpeciesCount() const { return Catalog.PlantSpecies.Num(); }

    const FWMScienceSimulationCatalog& GetCatalog() const { return Catalog; }

private:
    bool bCatalogLoaded = false;
    FWMScienceSimulationCatalog Catalog;
};
