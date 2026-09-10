#include "Environment/WMEnvironmentStateSubsystem.h"

#include "Engine/World.h"
#include "Environment/WMEnvironmentActionActor.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
    const FName DefaultPrototypeBiomeId(TEXT("biome.caribbean-rainforest"));
}

void UWMEnvironmentStateSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    ReloadAndActivatePrototypeProfile();
}

void UWMEnvironmentStateSubsystem::Deinitialize()
{
    ClearActionTargets();
    ProfileCatalog.Reset();
    Model = FWMEnvironmentStateModel();
    Super::Deinitialize();
}

bool UWMEnvironmentStateSubsystem::ReloadAndActivatePrototypeProfile()
{
    return ReloadProfileCatalog() && ActivateProfile(DefaultPrototypeBiomeId);
}

bool UWMEnvironmentStateSubsystem::ReloadProfileCatalog()
{
    ClearActionTargets();
    ProfileCatalog.Reset();

    const FString ProfileDirectory = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("WorldMakers/Biomes"));
    TArray<FString> ProfileFiles;
    IFileManager::Get().FindFiles(ProfileFiles, *FPaths::Combine(ProfileDirectory, TEXT("*.ecosystem.json")), true, false);
    ProfileFiles.Sort();

    for (const FString& ProfileFile : ProfileFiles)
    {
        FString Json;
        if (!FFileHelper::LoadFileToString(Json, *FPaths::Combine(ProfileDirectory, ProfileFile)))
        {
            continue;
        }

        FWMEnvironmentStateDefinition Definition;
        FString Error;
        if (!FWMEnvironmentStateDefinition::TryParseJson(Json, Definition, Error) || Definition.BiomeId.IsNone())
        {
            continue;
        }
        if (ProfileCatalog.Contains(Definition.BiomeId))
        {
            ProfileCatalog.Reset();
            return false;
        }
        ProfileCatalog.Add(Definition.BiomeId, MoveTemp(Definition));
    }

    return !ProfileCatalog.IsEmpty();
}

bool UWMEnvironmentStateSubsystem::ActivateProfile(const FName BiomeId)
{
    const FWMEnvironmentStateDefinition* Definition = ProfileCatalog.Find(BiomeId);
    if (!Definition)
    {
        return false;
    }

    ClearActionTargets();
    if (!Model.Initialize(*Definition))
    {
        return false;
    }

    // Action actors are intentionally spawned lazily after gameplay begins by the interaction component.
    OnEnvironmentStateChanged.Broadcast(Model.GetSnapshot());
    return true;
}

bool UWMEnvironmentStateSubsystem::ApplyAction(const FName ActionId)
{
    if (!Model.CanApplyAction(ActionId))
    {
        return false;
    }

    const FName PreviousReactionId = Model.GetSnapshot().ReactionId;
    if (!Model.ApplyAction(ActionId))
    {
        return false;
    }

    const FWMEnvironmentStateSnapshot Snapshot = Model.GetSnapshot();
    OnEnvironmentStateChanged.Broadcast(Snapshot);
    if (Snapshot.ReactionId != PreviousReactionId)
    {
        OnEnvironmentReaction.Broadcast(ActionId, Snapshot.ReactionId);
    }
    return true;
}

void UWMEnvironmentStateSubsystem::EnsureActionTargets()
{
    UWorld* World = GetWorld();
    const FWMEnvironmentStateDefinition& Definition = Model.GetDefinition();
    if (!World || Definition.BiomeId.IsNone())
    {
        return;
    }

    ActionTargets.RemoveAll([](const TWeakObjectPtr<AWMEnvironmentActionActor>& Target)
    {
        return !Target.IsValid();
    });

    for (const FWMEnvironmentActionDefinition& Action : Definition.Actions)
    {
        const bool bAlreadyExists = ActionTargets.ContainsByPredicate([&Action](const TWeakObjectPtr<AWMEnvironmentActionActor>& Target)
        {
            return Target.IsValid() && Target->GetInteractionActionId() == Action.ActionId;
        });
        if (bAlreadyExists)
        {
            continue;
        }

        FActorSpawnParameters SpawnParameters;
        SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AWMEnvironmentActionActor* Target = World->SpawnActor<AWMEnvironmentActionActor>(
            AWMEnvironmentActionActor::StaticClass(),
            Action.LocationCm,
            FRotator::ZeroRotator,
            SpawnParameters);
        if (Target)
        {
            Target->Configure(Action);
            ActionTargets.Add(Target);
        }
    }
}

int32 UWMEnvironmentStateSubsystem::GetActionTargetCount() const
{
    int32 Count = 0;
    for (const TWeakObjectPtr<AWMEnvironmentActionActor>& Target : ActionTargets)
    {
        if (Target.IsValid())
        {
            ++Count;
        }
    }
    return Count;
}

void UWMEnvironmentStateSubsystem::ClearActionTargets()
{
    for (const TWeakObjectPtr<AWMEnvironmentActionActor>& Target : ActionTargets)
    {
        if (Target.IsValid())
        {
            Target->Destroy();
        }
    }
    ActionTargets.Reset();
}
