#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "WMGameMode.generated.h"

UCLASS(Config = Game)
class WORLDMAKERS_API AWMGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AWMGameMode();
    virtual void BeginPlay() override;

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "World Makers|Prototype")
    bool bSpawnMicroVerticalSlice = true;

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "World Makers|Prototype")
    bool bSpawnPrototypeGround = false;

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "World Makers|Prototype")
    bool bSpawnPrototypeMissionGeometry = true;

private:
    void EnsurePrototypeEnvironment();
    void EnsurePrototypeMissionGeometry();
    void EnsurePrototypeGround();
};
