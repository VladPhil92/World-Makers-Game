#include "Science/WMScienceSimulationSubsystem.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

void UWMScienceSimulationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    ReloadScienceCatalog();
}

bool UWMScienceSimulationSubsystem::ReloadScienceCatalog()
{
    bCatalogLoaded = false;
    Catalog = FWMScienceSimulationCatalog();

    const FString CatalogPath = FPaths::Combine(
        FPaths::ProjectContentDir(),
        TEXT("WorldMakers/Science/science-simulation-core-v1.json"));

    FString Json;
    if (!FFileHelper::LoadFileToString(Json, *CatalogPath))
    {
        return false;
    }

    FWMScienceSimulationCatalog Candidate;
    FString Error;
    if (!FWMScienceSimulationCatalog::TryParseJson(Json, Candidate, Error))
    {
        return false;
    }

    Catalog = MoveTemp(Candidate);
    bCatalogLoaded = true;
    return true;
}
