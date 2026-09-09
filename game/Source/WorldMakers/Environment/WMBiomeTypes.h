#pragma once

#include "CoreMinimal.h"
#include "WMBiomeTypes.generated.h"

USTRUCT(BlueprintType)
struct WORLDMAKERS_API FWMBiomeZoneDefinition
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Biome")
    FName ZoneId;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Biome")
    FName Kind;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Biome")
    FVector CenterCm = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Biome")
    FVector ExtentCm = FVector(100.0f);

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Biome")
    bool bBuildAllowed = false;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Biome")
    TArray<FName> MissionIds;

    bool IsSane() const;
    bool ContainsWorldLocation(const FVector& WorldLocation, const FVector& BiomeOriginCm) const;
};

USTRUCT(BlueprintType)
struct WORLDMAKERS_API FWMPointOfInterestDefinition
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Exploration")
    FName PointId;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Exploration")
    FName ZoneId;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Exploration")
    FName Category;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Exploration")
    FVector LocationCm = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Exploration")
    float DiscoveryRadiusCm = 250.0f;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Exploration")
    FName DiscoveryId;

    bool IsSane() const;
    bool IsWithinDiscoveryRange(const FVector& WorldLocation, const FVector& BiomeOriginCm) const;
};

USTRUCT(BlueprintType)
struct WORLDMAKERS_API FWMBiomeRuntimeDefinition
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Biome")
    int32 SchemaVersion = 1;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Biome")
    FName BiomeId;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Biome")
    bool bPrototypeOnly = true;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Biome")
    FVector OriginCm = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Biome")
    TArray<FWMBiomeZoneDefinition> Zones;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Exploration")
    TArray<FWMPointOfInterestDefinition> PointsOfInterest;

    bool IsSane() const;
    const FWMBiomeZoneDefinition* FindZoneAtWorldLocation(const FVector& WorldLocation) const;
    TArray<FName> FindNearbyPointOfInterestIds(const FVector& WorldLocation) const;

    static bool TryParseJson(const FString& Json, FWMBiomeRuntimeDefinition& OutDefinition, FString& OutError);
};

struct WORLDMAKERS_API FWMExplorationProgressModel
{
    bool RegisterDiscovery(FName DiscoveryId);
    bool HasDiscovered(FName DiscoveryId) const;
    TArray<FName> GetDiscoveredIds() const;
    void Reset();

private:
    TSet<FName> DiscoveredIds;
};
