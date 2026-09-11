#include "Environment/WMPlanetaryCommonsSubsystem.h"

#include "Environment/WMEnvironmentStateSubsystem.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
    const TCHAR* PrototypeRelativePath = TEXT("WorldMakers/Environment/planetary-commons-living-world-v1.json");
}

void UWMPlanetaryCommonsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Collection.InitializeDependency<UWMEnvironmentStateSubsystem>();

    ReloadPrototypeProfile();

    if (UWMEnvironmentStateSubsystem* Legacy = GetWorld() ? GetWorld()->GetSubsystem<UWMEnvironmentStateSubsystem>() : nullptr)
    {
        Legacy->OnEnvironmentStateChanged.AddDynamic(this, &UWMPlanetaryCommonsSubsystem::HandleLegacyEnvironmentChanged);
        SyncFromLegacyEcosystem();
    }
}

void UWMPlanetaryCommonsSubsystem::Deinitialize()
{
    if (UWMEnvironmentStateSubsystem* Legacy = GetWorld() ? GetWorld()->GetSubsystem<UWMEnvironmentStateSubsystem>() : nullptr)
    {
        Legacy->OnEnvironmentStateChanged.RemoveDynamic(this, &UWMPlanetaryCommonsSubsystem::HandleLegacyEnvironmentChanged);
    }

    Definition = FWMPlanetaryCommonsDefinition();
    Model = FWMPlanetaryCommonsModel();
    LastLegacySnapshot = FWMEnvironmentStateSnapshot();
    bHasLegacySnapshot = false;
    Super::Deinitialize();
}

bool UWMPlanetaryCommonsSubsystem::ReloadPrototypeProfile()
{
    const FString Path = FPaths::Combine(FPaths::ProjectContentDir(), PrototypeRelativePath);
    FString Json;
    if (!FFileHelper::LoadFileToString(Json, *Path))
    {
        return false;
    }

    FWMPlanetaryCommonsDefinition Parsed;
    FString Error;
    if (!FWMPlanetaryCommonsDefinition::TryParseJson(Json, Parsed, Error))
    {
        return false;
    }

    Definition = MoveTemp(Parsed);
    bHasLegacySnapshot = false;
    if (!Model.Initialize(Definition))
    {
        return false;
    }

    Broadcast(TEXT("simulation.c2.profile-loaded"));
    return true;
}

bool UWMPlanetaryCommonsSubsystem::SyncFromLegacyEcosystem()
{
    UWMEnvironmentStateSubsystem* Legacy = GetWorld() ? GetWorld()->GetSubsystem<UWMEnvironmentStateSubsystem>() : nullptr;
    if (!Legacy)
    {
        return false;
    }

    const FWMEnvironmentStateSnapshot Current = Legacy->GetStateSnapshot();
    if (!Current.IsBounded())
    {
        return false;
    }

    const bool bAccepted = bHasLegacySnapshot
        ? Model.ApplyLegacyProjectionDelta(LastLegacySnapshot, Current)
        : Model.SeedFromLegacyEcosystem(Current);
    if (!bAccepted)
    {
        return false;
    }

    LastLegacySnapshot = Current;
    bHasLegacySnapshot = true;
    Broadcast(TEXT("simulation.c2.legacy-sync"));
    return true;
}

void UWMPlanetaryCommonsSubsystem::HandleLegacyEnvironmentChanged(const FWMEnvironmentStateSnapshot Snapshot)
{
    if (!Snapshot.IsBounded())
    {
        return;
    }

    const bool bAccepted = bHasLegacySnapshot
        ? Model.ApplyLegacyProjectionDelta(LastLegacySnapshot, Snapshot)
        : Model.SeedFromLegacyEcosystem(Snapshot);
    if (!bAccepted)
    {
        return;
    }

    LastLegacySnapshot = Snapshot;
    bHasLegacySnapshot = true;
    Broadcast(TEXT("simulation.c2.legacy-environment-delta"));
}

bool UWMPlanetaryCommonsSubsystem::ApplyIntervention(const FName InterventionId)
{
    if (!Model.ApplyIntervention(InterventionId))
    {
        return false;
    }
    Broadcast(InterventionId);
    return true;
}

bool UWMPlanetaryCommonsSubsystem::AdvanceSimulationStep()
{
    if (!Model.AdvanceStep())
    {
        return false;
    }
    Broadcast(TEXT("simulation.c2.step"));
    return true;
}

void UWMPlanetaryCommonsSubsystem::Broadcast(const FName CausalId)
{
    const FWMPlanetaryCommonsSnapshot Snapshot = Model.GetSnapshot();
    if (Snapshot.IsBounded())
    {
        OnPlanetaryCommonsChanged.Broadcast(CausalId, Snapshot);
    }
}
