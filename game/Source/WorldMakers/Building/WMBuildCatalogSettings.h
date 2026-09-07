#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "WMBuildCatalogSettings.generated.h"

USTRUCT(BlueprintType)
struct FWMBuildPieceSpec
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Makers|Building")
    FName PieceId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Makers|Building")
    FString DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Makers|Building")
    FString Category = TEXT("Block");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Makers|Building")
    FVector DimensionsCm = FVector(100.0f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Makers|Building", meta = (ClampMin = "15.0", ClampMax = "180.0"))
    float RotationStepDegrees = 90.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Makers|Building")
    bool bCanBeSupport = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Makers|Building", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MinSurfaceUpDot = 0.90f;

    bool IsSane() const;
};

UCLASS(Config = Game, DefaultConfig)
class WORLDMAKERS_API UWMBuildCatalogSettings : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(Config, EditAnywhere, Category = "World Makers|Building")
    TArray<FWMBuildPieceSpec> Pieces;

    bool FindPieceSpec(FName PieceId, FWMBuildPieceSpec& OutSpec) const;
    void GetPieceIds(TArray<FName>& OutPieceIds) const;
};
