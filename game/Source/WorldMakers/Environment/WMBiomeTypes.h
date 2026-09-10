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

    /** M3.2: deliberate interaction metadata. Stable IDs/keys only; no authored child free text. */
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Interaction")
    bool bRequiresInteraction = false;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Interaction")
    FName InteractionMode;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Interaction")
    float InteractionRadiusCm = 350.0f;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Interaction")
    float FocusRadiusCm = 90.0f;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Interaction")
    FName PromptKey;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Interaction")
    FName ObservationId;

    bool IsSane() const;
    bool IsWithinDiscoveryRange(const FVector& WorldLocation, const FVector& BiomeOriginCm) const;
    bool IsWithinInteractionRange(const FVector& WorldLocation, const FVector& BiomeOriginCm) const;
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
    const FWMPointOfInterestDefinition* FindPointOfInterest(FName PointId) const;
    TArray<FName> FindNearbyPointOfInterestIds(const FVector& WorldLocation) const;

    static bool TryParseJson(const FString& Json, FWMBiomeRuntimeDefinition& OutDefinition, FString& OutError);
};

struct WORLDMAKERS_API FWMExplorationProgressModel
{
    bool RegisterDiscovery(FName DiscoveryId);
    bool HasDiscovered(FName DiscoveryId) const;
    TArray<FName> GetDiscoveredIds() const;

    bool RegisterObservation(FName ObservationId);
    bool HasObserved(FName ObservationId) const;
    TArray<FName> GetObservedIds() const;

    void Reset();

private:
    TSet<FName> DiscoveredIds;
    TSet<FName> ObservedIds;
};
