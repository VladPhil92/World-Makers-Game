#include "Environment/WMEnvironmentReactionProxyActor.h"

#include "Engine/World.h"
#include "Environment/WMEnvironmentStateSubsystem.h"

AWMEnvironmentReactionProxyActor::AWMEnvironmentReactionProxyActor()
{
    PrimaryActorTick.bCanEverTick = false;
    SetReplicates(false);
}

void AWMEnvironmentReactionProxyActor::BeginPlay()
{
    Super::BeginPlay();
    if (UWorld* World = GetWorld())
    {
        if (UWMEnvironmentStateSubsystem* EnvironmentState = World->GetSubsystem<UWMEnvironmentStateSubsystem>())
        {
            EnvironmentState->OnEnvironmentStateChanged.AddDynamic(this, &AWMEnvironmentReactionProxyActor::HandleEnvironmentStateChanged);
            HandleEnvironmentStateChanged(EnvironmentState->GetStateSnapshot());
        }
    }
}

void AWMEnvironmentReactionProxyActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UWorld* World = GetWorld())
    {
        if (UWMEnvironmentStateSubsystem* EnvironmentState = World->GetSubsystem<UWMEnvironmentStateSubsystem>())
        {
            EnvironmentState->OnEnvironmentStateChanged.RemoveDynamic(this, &AWMEnvironmentReactionProxyActor::HandleEnvironmentStateChanged);
        }
    }
    Super::EndPlay(EndPlayReason);
}

void AWMEnvironmentReactionProxyActor::HandleEnvironmentStateChanged(FWMEnvironmentStateSnapshot Snapshot)
{
    if (!Snapshot.IsBounded())
    {
        return;
    }
    if (CurrentReactionId == Snapshot.ReactionId)
    {
        return;
    }
    CurrentReactionId = Snapshot.ReactionId;
    OnReactionVisualChanged(CurrentReactionId, Snapshot.HabitatQuality);
}
