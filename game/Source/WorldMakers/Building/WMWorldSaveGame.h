#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "WMWorldSaveGame.generated.h"

USTRUCT(BlueprintType)
struct FWMBuildSaveRecord
{
    GENERATED_BODY()

    UPROPERTY(SaveGame, BlueprintReadOnly)
    FName PieceId = NAME_None;

    UPROPERTY(SaveGame, BlueprintReadOnly)
    FTransform Transform = FTransform::Identity;
};

UCLASS()
class WORLDMAKERS_API UWMWorldSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY(SaveGame, BlueprintReadOnly)
    TArray<FWMBuildSaveRecord> Pieces;

    UPROPERTY(SaveGame, BlueprintReadOnly)
    int32 SaveFormatVersion = 1;
};
