#include "Environment/WMBiomeRuntimeSubsystem.h"

#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
    const FName DefaultPrototypeBiomeId(TEXT("biome.caribbean-rainforest"));
}

void UWMBiomeRuntimeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    ReloadAndActivatePrototypeBiome();
}

bool UWMBiomeRuntimeSubsystem::ReloadAndActivatePrototypeBiome()
{
    return ReloadBiomeCatalog() && ActivateBiome(DefaultPrototypeBiomeId);
}

bool UWMBiomeRuntimeSubsystem::ReloadBiomeCatalog()
{
    BiomeCatalog.Reset();
    AvailableBiomeIds.Reset();
    ActiveBiomeId = NAME_None;
    CurrentZoneId = NAME_None;

    const FString BiomeDirectory = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("WorldMakers/Biomes"));
    TArray<FString> BiomeFiles;
    IFileManager::Get().FindFiles(BiomeFiles, *FPaths::Combine(BiomeDirectory, TEXT("*.json")), true, false);
    BiomeFiles.Sort();

    for (const FString& BiomeFile : BiomeFiles)
    {
        FString Json;
        if (!FFileHelper::LoadFileToString(Json, *FPaths::Combine(BiomeDirectory, BiomeFile)))
        {
            continue;
        }

        FWMBiomeRuntimeDefinition Definition;
        FString Error;
        if (!FWMBiomeRuntimeDefinition::TryParseJson(Json, Definition, Error) || Definition.BiomeId.IsNone())
        {
            continue;
        }

        if (BiomeCatalog.Contains(Definition.BiomeId))
        {
            BiomeCatalog.Reset();
            AvailableBiomeIds.Reset();
            return false;
        }

        AvailableBiomeIds.Add(Definition.BiomeId);
        BiomeCatalog.Add(Definition.BiomeId, MoveTemp(Definition));
    }

    AvailableBiomeIds.Sort([](const FName& A, const FName& B)
    {
        return A.ToString() < B.ToString();
    });
    return !AvailableBiomeIds.IsEmpty();
}

bool UWMBiomeRuntimeSubsystem::ActivateBiome(const FName BiomeId)
{
    if (!BiomeCatalog.Contains(BiomeId))
    {
        return false;
    }
    ActiveBiomeId = BiomeId;
    CurrentZoneId = NAME_None;
    return true;
}

const FWMBiomeRuntimeDefinition* UWMBiomeRuntimeSubsystem::GetActiveDefinition() const
{
    return BiomeCatalog.Find(ActiveBiomeId);
}

TArray<FName> UWMBiomeRuntimeSubsystem::GetNearbyPointOfInterestIds(const FVector WorldLocation) const
{
    const FWMBiomeRuntimeDefinition* Definition = GetActiveDefinition();
    return Definition ? Definition->FindNearbyPointOfInterestIds(WorldLocation) : TArray<FName>();
}

TArray<FName> UWMBiomeRuntimeSubsystem::ObserveLocation(const FVector WorldLocation)
{
    TArray<FName> NewDiscoveryIds;
    const FWMBiomeRuntimeDefinition* Definition = GetActiveDefinition();
    if (!Definition || WorldLocation.ContainsNaN())
    {
        return NewDiscoveryIds;
    }

    const FWMBiomeZoneDefinition* Zone = Definition->FindZoneAtWorldLocation(WorldLocation);
    const FName NewZoneId = Zone ? Zone->ZoneId : NAME_None;
    if (NewZoneId != CurrentZoneId)
    {
        CurrentZoneId = NewZoneId;
        OnZoneChanged.Broadcast(CurrentZoneId);
    }

    for (const FWMPointOfInterestDefinition& Point : Definition->PointsOfInterest)
    {
        if (!Point.IsWithinDiscoveryRange(WorldLocation, Definition->OriginCm))
        {
            continue;
        }
        if (ExplorationProgress.RegisterDiscovery(Point.DiscoveryId))
        {
            NewDiscoveryIds.Add(Point.DiscoveryId);
            OnDiscoveryRegistered.Broadcast(Point.DiscoveryId);
        }
    }

    NewDiscoveryIds.Sort([](const FName& A, const FName& B)
    {
        return A.ToString() < B.ToString();
    });
    return NewDiscoveryIds;
}

void UWMBiomeRuntimeSubsystem::ResetSessionDiscoveries()
{
    ExplorationProgress.Reset();
}
