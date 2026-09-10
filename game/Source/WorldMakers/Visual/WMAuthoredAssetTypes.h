#pragma once

#include "CoreMinimal.h"
#include "WMAuthoredAssetTypes.generated.h"

USTRUCT(BlueprintType)
struct WORLDMAKERS_API FWMAuthoredVisualAssetDefinition
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Visual|Authored")
    FName AssetId;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Visual|Authored")
    FName AssetKind;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Visual|Authored")
    FString ObjectPath;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Visual|Authored")
    FString SourcePath;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Visual|Authored")
    FName CollisionPolicy;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Visual|Authored")
    FName FallbackMode;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Visual|Authored")
    int32 MinLods = 1;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Visual|Authored")
    int32 MaxMaterialSlots = 2;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Visual|Authored")
    bool bAuthoredPresent = false;

    bool IsSane() const;
    bool CanAttemptLoad() const { return bAuthoredPresent && IsSane(); }
};

USTRUCT(BlueprintType)
struct WORLDMAKERS_API FWMAuthoredVisualAssetCatalog
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Visual|Authored")
    int32 SchemaVersion = 1;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Visual|Authored")
    FName Units;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Visual|Authored")
    FName UpAxis;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Visual|Authored")
    FName ForwardAxis;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Visual|Authored")
    TArray<FWMAuthoredVisualAssetDefinition> Assets;

    bool IsSane() const;
    const FWMAuthoredVisualAssetDefinition* FindAsset(FName AssetId) const;
    static bool TryParseJson(const FString& Json, FWMAuthoredVisualAssetCatalog& OutCatalog, FString& OutError);
};
