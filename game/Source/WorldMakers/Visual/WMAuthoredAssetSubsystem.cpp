#include "Visual/WMAuthoredAssetSubsystem.h"

#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/SoftObjectPath.h"

namespace WMAuthoredAssetRuntime
{
    bool StaticMeshMeetsContract(const UStaticMesh* Mesh, const FWMAuthoredVisualAssetDefinition& Definition)
    {
        return Mesh && Mesh->GetNumLODs() >= Definition.MinLods &&
            Mesh->GetStaticMaterials().Num() <= Definition.MaxMaterialSlots;
    }

    bool SkeletalMeshMeetsContract(const USkeletalMesh* Mesh, const FWMAuthoredVisualAssetDefinition& Definition)
    {
        return Mesh && Mesh->GetLODNum() >= Definition.MinLods &&
            Mesh->GetMaterials().Num() <= Definition.MaxMaterialSlots;
    }
}

void UWMAuthoredAssetSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    ReloadCatalog();
}

bool UWMAuthoredAssetSubsystem::ReloadCatalog()
{
    bCatalogLoaded = false;
    Catalog = FWMAuthoredVisualAssetCatalog();

    FString Json;
    const FString Path = FPaths::Combine(
        FPaths::ProjectContentDir(),
        TEXT("WorldMakers/Visual/Authored/authored-assets-p1.json"));
    if (!FFileHelper::LoadFileToString(Json, *Path))
    {
        return false;
    }

    FString Error;
    if (!FWMAuthoredVisualAssetCatalog::TryParseJson(Json, Catalog, Error))
    {
        UE_LOG(LogTemp, Warning, TEXT("World Makers P1 authored asset catalog rejected: %s"), *Error);
        return false;
    }

    bCatalogLoaded = true;
    return true;
}

bool UWMAuthoredAssetSubsystem::IsAuthoredAssetDeclaredPresent(const FName AssetId) const
{
    const FWMAuthoredVisualAssetDefinition* Asset = Catalog.FindAsset(AssetId);
    return bCatalogLoaded && Asset && Asset->bAuthoredPresent;
}

FString UWMAuthoredAssetSubsystem::GetObjectPath(const FName AssetId) const
{
    const FWMAuthoredVisualAssetDefinition* Asset = Catalog.FindAsset(AssetId);
    return bCatalogLoaded && Asset ? Asset->ObjectPath : FString();
}

const FWMAuthoredVisualAssetDefinition* UWMAuthoredAssetSubsystem::FindLoadableAsset(
    const FName AssetId,
    const FName ExpectedKind) const
{
    if (!bCatalogLoaded)
    {
        return nullptr;
    }
    const FWMAuthoredVisualAssetDefinition* Asset = Catalog.FindAsset(AssetId);
    if (!Asset || Asset->AssetKind != ExpectedKind || !Asset->CanAttemptLoad())
    {
        return nullptr;
    }
    return Asset;
}

UStaticMesh* UWMAuthoredAssetSubsystem::LoadStaticMesh(const FName AssetId) const
{
    const FWMAuthoredVisualAssetDefinition* Asset = FindLoadableAsset(AssetId, TEXT("StaticMesh"));
    if (!Asset)
    {
        return nullptr;
    }

    UStaticMesh* Mesh = Cast<UStaticMesh>(FSoftObjectPath(Asset->ObjectPath).TryLoad());
    if (!WMAuthoredAssetRuntime::StaticMeshMeetsContract(Mesh, *Asset))
    {
        UE_LOG(LogTemp, Warning, TEXT("World Makers authored StaticMesh rejected by P1 LOD/material contract: %s"), *AssetId.ToString());
        return nullptr;
    }
    return Mesh;
}

USkeletalMesh* UWMAuthoredAssetSubsystem::LoadSkeletalMesh(const FName AssetId) const
{
    const FWMAuthoredVisualAssetDefinition* Asset = FindLoadableAsset(AssetId, TEXT("SkeletalMesh"));
    if (!Asset)
    {
        return nullptr;
    }

    USkeletalMesh* Mesh = Cast<USkeletalMesh>(FSoftObjectPath(Asset->ObjectPath).TryLoad());
    if (!WMAuthoredAssetRuntime::SkeletalMeshMeetsContract(Mesh, *Asset))
    {
        UE_LOG(LogTemp, Warning, TEXT("World Makers authored SkeletalMesh rejected by P1 LOD/material contract: %s"), *AssetId.ToString());
        return nullptr;
    }
    return Mesh;
}
