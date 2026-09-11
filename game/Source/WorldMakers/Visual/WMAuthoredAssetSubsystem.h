#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Visual/WMAuthoredAssetTypes.h"
#include "WMAuthoredAssetSubsystem.generated.h"

class UStaticMesh;
class USkeletalMesh;

UCLASS()
class WORLDMAKERS_API UWMAuthoredAssetSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "World Makers|Visual|Authored")
    bool ReloadCatalog();

    UFUNCTION(BlueprintPure, Category = "World Makers|Visual|Authored")
    bool IsCatalogLoaded() const { return bCatalogLoaded; }

    UFUNCTION(BlueprintPure, Category = "World Makers|Visual|Authored")
    bool IsAuthoredAssetDeclaredPresent(FName AssetId) const;

    UFUNCTION(BlueprintPure, Category = "World Makers|Visual|Authored")
    FString GetObjectPath(FName AssetId) const;

    UStaticMesh* LoadStaticMesh(FName AssetId) const;
    USkeletalMesh* LoadSkeletalMesh(FName AssetId) const;
    const FWMAuthoredVisualAssetCatalog& GetCatalog() const { return Catalog; }

private:
    const FWMAuthoredVisualAssetDefinition* FindLoadableAsset(FName AssetId, FName ExpectedKind) const;

    UPROPERTY(Transient)
    bool bCatalogLoaded = false;

    FWMAuthoredVisualAssetCatalog Catalog;
};
