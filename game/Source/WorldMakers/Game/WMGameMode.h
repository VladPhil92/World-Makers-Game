#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "WMGameMode.generated.h"

UCLASS()
class WORLDMAKERS_API AWMGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AWMGameMode();
    virtual void BeginPlay() override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Makers|Prototype")
    bool bSpawnPrototypeGround = true;

private:
    void EnsurePrototypeGround();
};
