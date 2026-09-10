#pragma once

#include "CoreMinimal.h"
#include "WMEnvironmentStateTypes.generated.h"

USTRUCT(BlueprintType)
struct WORLDMAKERS_API FWMEnvironmentStateSnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Environment")
    FName BiomeId;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Environment")
    float VegetationHealth = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Environment")
    float WaterFlow = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Environment")
    float SoilProtection = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Environment")
    float ShadeCoverage = 0.0f;

    /** Derived from the four primary dimensions. Never authored or written independently. */
    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Environment")
    float HabitatQuality = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Environment")
    FName ReactionId;

    bool IsBounded() const;
};

USTRUCT(BlueprintType)
struct WORLDMAKERS_API FWMEnvironmentActionDefinition
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Environment")
    FName ActionId;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Environment")
    FName TargetId;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Environment")
    FName PromptKey;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Environment")
    FVector LocationCm = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Environment")
    float InteractionRadiusCm = 450.0f;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Environment")
    float FocusRadiusCm = 120.0f;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Environment")
    int32 MaxApplications = 1;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Environment")
    float VegetationDelta = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Environment")
    float WaterFlowDelta = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Environment")
    float SoilProtectionDelta = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Environment")
    float ShadeCoverageDelta = 0.0f;

    bool IsSane() const;
};

USTRUCT(BlueprintType)
struct WORLDMAKERS_API FWMEnvironmentReactionBand
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Environment")
    FName ReactionId;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Environment")
    float MaxHabitatQuality = 1.0f;

    bool IsSane() const;
};

USTRUCT(BlueprintType)
struct WORLDMAKERS_API FWMEnvironmentStateDefinition
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Environment")
    int32 SchemaVersion = 1;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Environment")
    FName BiomeId;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Environment")
    bool bPrototypeOnly = true;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Environment")
    float InitialVegetationHealth = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Environment")
    float InitialWaterFlow = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Environment")
    float InitialSoilProtection = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Environment")
    float InitialShadeCoverage = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Environment")
    TArray<FWMEnvironmentActionDefinition> Actions;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Environment")
    TArray<FWMEnvironmentReactionBand> ReactionBands;

    bool IsSane() const;
    const FWMEnvironmentActionDefinition* FindAction(FName ActionId) const;
    FName ResolveReactionId(float HabitatQuality) const;
    static float DeriveHabitatQuality(float VegetationHealth, float WaterFlow, float SoilProtection, float ShadeCoverage);
    static bool TryParseJson(const FString& Json, FWMEnvironmentStateDefinition& OutDefinition, FString& OutError);
};

/** Pure deterministic state model. Contains stable action IDs and bounded numeric state only. */
struct WORLDMAKERS_API FWMEnvironmentStateModel
{
    bool Initialize(const FWMEnvironmentStateDefinition& InDefinition);
    bool CanApplyAction(FName ActionId) const;
    bool ApplyAction(FName ActionId);
    int32 GetAppliedCount(FName ActionId) const;
    const FWMEnvironmentStateSnapshot& GetSnapshot() const { return Snapshot; }
    const FWMEnvironmentStateDefinition& GetDefinition() const { return Definition; }

private:
    void RefreshDerivedState();

    bool bInitialized = false;
    FWMEnvironmentStateDefinition Definition;
    FWMEnvironmentStateSnapshot Snapshot;
    TMap<FName, int32> ApplicationCounts;
};
