#include "Environment/WMBiomeTypes.h"

#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
    bool ReadVector(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field, FVector& OutVector)
    {
        const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
        if (!Object.IsValid() || !Object->TryGetArrayField(Field, Values) || !Values || Values->Num() != 3)
        {
            return false;
        }
        for (const TSharedPtr<FJsonValue>& Value : *Values)
        {
            if (!Value.IsValid() || Value->Type != EJson::Number)
            {
                return false;
            }
        }
        OutVector = FVector((*Values)[0]->AsNumber(), (*Values)[1]->AsNumber(), (*Values)[2]->AsNumber());
        return !OutVector.ContainsNaN();
    }

    bool ReadOptionalNameArray(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field, TArray<FName>& OutValues)
    {
        OutValues.Reset();
        if (!Object.IsValid() || !Object->HasField(Field))
        {
            return true;
        }

        const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
        if (!Object->TryGetArrayField(Field, Values) || !Values)
        {
            return false;
        }
        for (const TSharedPtr<FJsonValue>& Value : *Values)
        {
            if (!Value.IsValid() || Value->Type != EJson::String || Value->AsString().IsEmpty())
            {
                return false;
            }
            OutValues.Add(FName(*Value->AsString()));
        }
        return true;
    }
}

bool FWMBiomeZoneDefinition::IsSane() const
{
    return !ZoneId.IsNone() && !Kind.IsNone() && !CenterCm.ContainsNaN() && !ExtentCm.ContainsNaN() &&
        ExtentCm.X > 0.0f && ExtentCm.Y > 0.0f && ExtentCm.Z > 0.0f;
}

bool FWMBiomeZoneDefinition::ContainsWorldLocation(const FVector& WorldLocation, const FVector& BiomeOriginCm) const
{
    if (!IsSane() || WorldLocation.ContainsNaN() || BiomeOriginCm.ContainsNaN())
    {
        return false;
    }
    const FVector LocalDelta = WorldLocation - (BiomeOriginCm + CenterCm);
    return FMath::Abs(LocalDelta.X) <= ExtentCm.X &&
        FMath::Abs(LocalDelta.Y) <= ExtentCm.Y &&
        FMath::Abs(LocalDelta.Z) <= ExtentCm.Z;
}

bool FWMPointOfInterestDefinition::IsSane() const
{
    return !PointId.IsNone() && !ZoneId.IsNone() && !Category.IsNone() && !DiscoveryId.IsNone() &&
        !LocationCm.ContainsNaN() && FMath::IsFinite(DiscoveryRadiusCm) && DiscoveryRadiusCm >= 50.0f && DiscoveryRadiusCm <= 2000.0f;
}

bool FWMPointOfInterestDefinition::IsWithinDiscoveryRange(const FVector& WorldLocation, const FVector& BiomeOriginCm) const
{
    if (!IsSane() || WorldLocation.ContainsNaN() || BiomeOriginCm.ContainsNaN())
    {
        return false;
    }
    return FVector::DistSquared(WorldLocation, BiomeOriginCm + LocationCm) <= FMath::Square(DiscoveryRadiusCm);
}

bool FWMBiomeRuntimeDefinition::IsSane() const
{
    if (SchemaVersion != 1 || BiomeId.IsNone() || OriginCm.ContainsNaN() || Zones.Num() < 2 || PointsOfInterest.IsEmpty())
    {
        return false;
    }

    TSet<FName> ZoneIds;
    for (const FWMBiomeZoneDefinition& Zone : Zones)
    {
        if (!Zone.IsSane() || ZoneIds.Contains(Zone.ZoneId))
        {
            return false;
        }
        ZoneIds.Add(Zone.ZoneId);
    }

    TSet<FName> PointIds;
    TSet<FName> DiscoveryIds;
    for (const FWMPointOfInterestDefinition& Point : PointsOfInterest)
    {
        if (!Point.IsSane() || !ZoneIds.Contains(Point.ZoneId) || PointIds.Contains(Point.PointId) || DiscoveryIds.Contains(Point.DiscoveryId))
        {
            return false;
        }
        PointIds.Add(Point.PointId);
        DiscoveryIds.Add(Point.DiscoveryId);
    }
    return true;
}

const FWMBiomeZoneDefinition* FWMBiomeRuntimeDefinition::FindZoneAtWorldLocation(const FVector& WorldLocation) const
{
    for (const FWMBiomeZoneDefinition& Zone : Zones)
    {
        if (Zone.ContainsWorldLocation(WorldLocation, OriginCm))
        {
            return &Zone;
        }
    }
    return nullptr;
}

TArray<FName> FWMBiomeRuntimeDefinition::FindNearbyPointOfInterestIds(const FVector& WorldLocation) const
{
    TArray<FName> Result;
    for (const FWMPointOfInterestDefinition& Point : PointsOfInterest)
    {
        if (Point.IsWithinDiscoveryRange(WorldLocation, OriginCm))
        {
            Result.Add(Point.PointId);
        }
    }
    Result.Sort([](const FName& A, const FName& B)
    {
        return A.ToString() < B.ToString();
    });
    return Result;
}

bool FWMBiomeRuntimeDefinition::TryParseJson(const FString& Json, FWMBiomeRuntimeDefinition& OutDefinition, FString& OutError)
{
    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        OutError = TEXT("Biome runtime JSON is not a valid object.");
        return false;
    }

    double SchemaVersionValue = 0.0;
    FString BiomeIdString;
    bool bPrototypeOnlyValue = true;
    FWMBiomeRuntimeDefinition Candidate;
    if (!Root->TryGetNumberField(TEXT("schemaVersion"), SchemaVersionValue) ||
        !Root->TryGetStringField(TEXT("id"), BiomeIdString) || BiomeIdString.IsEmpty() ||
        !Root->TryGetBoolField(TEXT("prototypeOnly"), bPrototypeOnlyValue) ||
        !ReadVector(Root, TEXT("originCm"), Candidate.OriginCm))
    {
        OutError = TEXT("Biome runtime header is invalid.");
        return false;
    }

    Candidate.SchemaVersion = static_cast<int32>(SchemaVersionValue);
    Candidate.BiomeId = FName(*BiomeIdString);
    Candidate.bPrototypeOnly = bPrototypeOnlyValue;

    const TArray<TSharedPtr<FJsonValue>>* ZoneValues = nullptr;
    if (!Root->TryGetArrayField(TEXT("zones"), ZoneValues) || !ZoneValues)
    {
        OutError = TEXT("Biome zones are missing.");
        return false;
    }
    for (const TSharedPtr<FJsonValue>& ZoneValue : *ZoneValues)
    {
        const TSharedPtr<FJsonObject>* ZoneObjectPtr = nullptr;
        if (!ZoneValue.IsValid() || !ZoneValue->TryGetObject(ZoneObjectPtr) || !ZoneObjectPtr || !ZoneObjectPtr->IsValid())
        {
            OutError = TEXT("Biome zone entry is invalid.");
            return false;
        }
        const TSharedPtr<FJsonObject>& ZoneObject = *ZoneObjectPtr;
        FString ZoneIdString;
        FString KindString;
        FWMBiomeZoneDefinition Zone;
        if (!ZoneObject->TryGetStringField(TEXT("id"), ZoneIdString) || ZoneIdString.IsEmpty() ||
            !ZoneObject->TryGetStringField(TEXT("kind"), KindString) || KindString.IsEmpty() ||
            !ReadVector(ZoneObject, TEXT("centerCm"), Zone.CenterCm) ||
            !ReadVector(ZoneObject, TEXT("extentCm"), Zone.ExtentCm) ||
            !ZoneObject->TryGetBoolField(TEXT("buildAllowed"), Zone.bBuildAllowed) ||
            !ReadOptionalNameArray(ZoneObject, TEXT("missionIds"), Zone.MissionIds))
        {
            OutError = TEXT("Biome zone fields are invalid.");
            return false;
        }
        Zone.ZoneId = FName(*ZoneIdString);
        Zone.Kind = FName(*KindString);
        Candidate.Zones.Add(MoveTemp(Zone));
    }

    const TArray<TSharedPtr<FJsonValue>>* PointValues = nullptr;
    if (!Root->TryGetArrayField(TEXT("pointsOfInterest"), PointValues) || !PointValues)
    {
        OutError = TEXT("Biome points of interest are missing.");
        return false;
    }
    for (const TSharedPtr<FJsonValue>& PointValue : *PointValues)
    {
        const TSharedPtr<FJsonObject>* PointObjectPtr = nullptr;
        if (!PointValue.IsValid() || !PointValue->TryGetObject(PointObjectPtr) || !PointObjectPtr || !PointObjectPtr->IsValid())
        {
            OutError = TEXT("Biome point of interest entry is invalid.");
            return false;
        }
        const TSharedPtr<FJsonObject>& PointObject = *PointObjectPtr;
        FString PointIdString;
        FString ZoneIdString;
        FString CategoryString;
        FString DiscoveryIdString;
        double RadiusValue = 0.0;
        FWMPointOfInterestDefinition Point;
        if (!PointObject->TryGetStringField(TEXT("id"), PointIdString) || PointIdString.IsEmpty() ||
            !PointObject->TryGetStringField(TEXT("zoneId"), ZoneIdString) || ZoneIdString.IsEmpty() ||
            !PointObject->TryGetStringField(TEXT("category"), CategoryString) || CategoryString.IsEmpty() ||
            !PointObject->TryGetStringField(TEXT("discoveryId"), DiscoveryIdString) || DiscoveryIdString.IsEmpty() ||
            !PointObject->TryGetNumberField(TEXT("discoveryRadiusCm"), RadiusValue) ||
            !ReadVector(PointObject, TEXT("locationCm"), Point.LocationCm))
        {
            OutError = TEXT("Biome point of interest fields are invalid.");
            return false;
        }
        Point.PointId = FName(*PointIdString);
        Point.ZoneId = FName(*ZoneIdString);
        Point.Category = FName(*CategoryString);
        Point.DiscoveryId = FName(*DiscoveryIdString);
        Point.DiscoveryRadiusCm = static_cast<float>(RadiusValue);
        Candidate.PointsOfInterest.Add(MoveTemp(Point));
    }

    if (!Candidate.IsSane())
    {
        OutError = TEXT("Biome runtime definition failed semantic validation.");
        return false;
    }

    OutDefinition = MoveTemp(Candidate);
    OutError.Reset();
    return true;
}

bool FWMExplorationProgressModel::RegisterDiscovery(const FName DiscoveryId)
{
    if (DiscoveryId.IsNone() || DiscoveredIds.Contains(DiscoveryId))
    {
        return false;
    }
    DiscoveredIds.Add(DiscoveryId);
    return true;
}

bool FWMExplorationProgressModel::HasDiscovered(const FName DiscoveryId) const
{
    return !DiscoveryId.IsNone() && DiscoveredIds.Contains(DiscoveryId);
}

TArray<FName> FWMExplorationProgressModel::GetDiscoveredIds() const
{
    TArray<FName> Result = DiscoveredIds.Array();
    Result.Sort([](const FName& A, const FName& B)
    {
        return A.ToString() < B.ToString();
    });
    return Result;
}

void FWMExplorationProgressModel::Reset()
{
    DiscoveredIds.Reset();
}
