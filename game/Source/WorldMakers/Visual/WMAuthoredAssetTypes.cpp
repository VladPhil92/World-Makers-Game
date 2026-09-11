#include "Visual/WMAuthoredAssetTypes.h"

#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace WMAuthoredAssets
{
    bool TryReadName(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field, FName& OutValue)
    {
        FString Value;
        if (!Object.IsValid() || !Object->TryGetStringField(Field, Value) || Value.IsEmpty())
        {
            return false;
        }
        OutValue = FName(*Value);
        return true;
    }
}

bool FWMAuthoredVisualAssetDefinition::IsSane() const
{
    const bool bValidKind = AssetKind == TEXT("StaticMesh") || AssetKind == TEXT("SkeletalMesh") ||
        AssetKind == TEXT("Material") || AssetKind == TEXT("NiagaraSystem") ||
        AssetKind == TEXT("AnimationBlueprint") || AssetKind == TEXT("LevelSequence") || AssetKind == TEXT("DataAsset");
    const bool bValidCollision = CollisionPolicy == TEXT("none") || CollisionPolicy == TEXT("proxy") || CollisionPolicy == TEXT("authored-simple");
    const bool bValidFallback = FallbackMode == TEXT("procedural") || FallbackMode == TEXT("legacy") || FallbackMode == TEXT("none");
    return !AssetId.IsNone() && bValidKind &&
        ObjectPath.StartsWith(TEXT("/Game/WorldMakers/")) && ObjectPath.Contains(TEXT(".")) &&
        SourcePath.StartsWith(TEXT("SourceArt/WorldMakers/")) &&
        bValidCollision && bValidFallback && MinLods >= 1 && MinLods <= 8 &&
        MaxMaterialSlots >= 1 && MaxMaterialSlots <= 4;
}

bool FWMAuthoredVisualAssetCatalog::IsSane() const
{
    if (SchemaVersion != 1 || Units != TEXT("centimeters") || UpAxis != TEXT("Z") || ForwardAxis != TEXT("X") || Assets.Num() < 8)
    {
        return false;
    }

    TSet<FName> Seen;
    for (const FWMAuthoredVisualAssetDefinition& Asset : Assets)
    {
        if (!Asset.IsSane() || Seen.Contains(Asset.AssetId))
        {
            return false;
        }
        Seen.Add(Asset.AssetId);
    }
    return true;
}

const FWMAuthoredVisualAssetDefinition* FWMAuthoredVisualAssetCatalog::FindAsset(const FName AssetId) const
{
    return Assets.FindByPredicate([AssetId](const FWMAuthoredVisualAssetDefinition& Asset)
    {
        return Asset.AssetId == AssetId;
    });
}

bool FWMAuthoredVisualAssetCatalog::TryParseJson(const FString& Json, FWMAuthoredVisualAssetCatalog& OutCatalog, FString& OutError)
{
    OutCatalog = FWMAuthoredVisualAssetCatalog();
    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        OutError = TEXT("Authored asset catalog is not valid JSON.");
        return false;
    }

    double SchemaVersion = 0.0;
    FString Units;
    FString UpAxis;
    FString ForwardAxis;
    if (!Root->TryGetNumberField(TEXT("schemaVersion"), SchemaVersion) ||
        !Root->TryGetStringField(TEXT("units"), Units) ||
        !Root->TryGetStringField(TEXT("upAxis"), UpAxis) ||
        !Root->TryGetStringField(TEXT("forwardAxis"), ForwardAxis))
    {
        OutError = TEXT("Authored asset catalog header is incomplete.");
        return false;
    }

    OutCatalog.SchemaVersion = static_cast<int32>(SchemaVersion);
    OutCatalog.Units = FName(*Units);
    OutCatalog.UpAxis = FName(*UpAxis);
    OutCatalog.ForwardAxis = FName(*ForwardAxis);

    const TArray<TSharedPtr<FJsonValue>>* Assets = nullptr;
    if (!Root->TryGetArrayField(TEXT("assets"), Assets) || !Assets)
    {
        OutError = TEXT("Authored asset catalog assets array is missing.");
        return false;
    }

    for (const TSharedPtr<FJsonValue>& Value : *Assets)
    {
        const TSharedPtr<FJsonObject> Object = Value.IsValid() ? Value->AsObject() : nullptr;
        FWMAuthoredVisualAssetDefinition Asset;
        FString ObjectPath;
        FString SourcePath;
        FString CollisionPolicy;
        FString FallbackMode;
        double MinLods = 0.0;
        double MaxMaterialSlots = 0.0;
        bool bPresent = false;
        if (!WMAuthoredAssets::TryReadName(Object, TEXT("id"), Asset.AssetId) ||
            !WMAuthoredAssets::TryReadName(Object, TEXT("kind"), Asset.AssetKind) ||
            !Object->TryGetStringField(TEXT("objectPath"), ObjectPath) ||
            !Object->TryGetStringField(TEXT("sourcePath"), SourcePath) ||
            !Object->TryGetStringField(TEXT("collisionPolicy"), CollisionPolicy) ||
            !Object->TryGetStringField(TEXT("fallbackMode"), FallbackMode) ||
            !Object->TryGetNumberField(TEXT("minLods"), MinLods) ||
            !Object->TryGetNumberField(TEXT("maxMaterialSlots"), MaxMaterialSlots) ||
            !Object->TryGetBoolField(TEXT("authoredPresent"), bPresent))
        {
            OutError = TEXT("Authored asset entry is incomplete.");
            return false;
        }
        Asset.ObjectPath = ObjectPath;
        Asset.SourcePath = SourcePath;
        Asset.CollisionPolicy = FName(*CollisionPolicy);
        Asset.FallbackMode = FName(*FallbackMode);
        Asset.MinLods = static_cast<int32>(MinLods);
        Asset.MaxMaterialSlots = static_cast<int32>(MaxMaterialSlots);
        Asset.bAuthoredPresent = bPresent;
        OutCatalog.Assets.Add(MoveTemp(Asset));
    }

    if (!OutCatalog.IsSane())
    {
        OutError = TEXT("Authored asset catalog failed semantic validation.");
        return false;
    }
    OutError.Reset();
    return true;
}
