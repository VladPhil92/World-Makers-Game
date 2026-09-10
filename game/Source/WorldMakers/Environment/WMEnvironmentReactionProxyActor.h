#pragma once

#include "CoreMinimal.h"
#include "Environment/WMEnvironmentStateTypes.h"
#include "GameFramework/Actor.h"
#include "WMEnvironmentReactionProxyActor.generated.h"

/**
 * Lightweight authored-visual adapter for M3.4 reaction bands.
 * Level art may subclass this actor and react to stable reaction IDs without owning ecosystem rules.
 */
UCLASS(Blueprintable)
class WORLDMAKERS_API AWMEnvironmentReactionProxyActor : public AActor
{
    GENERATED_BODY()

public:
    AWMEnvironmentReactionProxyActor();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Environment")
    FName CurrentReactionId;

    UFUNCTION(BlueprintImplementableEvent, Category = "World Makers|Environment")
    void OnReactionVisualChanged(FName ReactionId, float HabitatQuality);

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UFUNCTION()
    void HandleEnvironmentStateChanged(FWMEnvironmentStateSnapshot Snapshot);
};
