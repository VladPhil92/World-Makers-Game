#pragma once

#include "CoreMinimal.h"
#include "WMPerformanceProfileTypes.generated.h"

USTRUCT(BlueprintType)
struct WORLDMAKERS_API FWMPerformanceBudget
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Performance")
    int32 TargetFps = 30;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Performance")
    float FrameTimeBudgetMs = 33.34f;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Performance")
    int32 MaxWorldActors = 1000;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Performance")
    int32 MaxPlacedBuildPieces = 300;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Performance")
    int32 MaxActiveInteractables = 24;

    bool IsSane() const;
};

USTRUCT(BlueprintType)
struct WORLDMAKERS_API FWMScalabilityProfile
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Performance")
    int32 ScreenPercentage = 85;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Performance")
    int32 TexturePoolMB = 512;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Performance")
    int32 ViewDistanceQuality = 1;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Performance")
    int32 AntiAliasingQuality = 2;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Performance")
    int32 ShadowQuality = 1;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Performance")
    int32 PostProcessQuality = 1;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Performance")
    int32 TextureQuality = 2;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Performance")
    int32 EffectsQuality = 1;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Performance")
    int32 FoliageQuality = 1;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Performance")
    float FoliageDensityScale = 0.65f;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Performance")
    float GrassDensityScale = 0.65f;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Performance")
    float ShadowDistanceScale = 0.70f;

    bool IsSane() const;
    TMap<FString, FString> BuildAllowlistedCVarAssignments() const;
};

USTRUCT(BlueprintType)
struct WORLDMAKERS_API FWMPerformanceProfileDefinition
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Performance")
    FName ProfileId;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Performance")
    FName DeviceClass;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Performance")
    FName Tier;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Performance")
    FWMPerformanceBudget Budget;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Performance")
    FWMScalabilityProfile Scalability;

    bool IsSane() const;
};

USTRUCT(BlueprintType)
struct WORLDMAKERS_API FWMPerformanceProfileCatalog
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Performance")
    int32 SchemaVersion = 1;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Performance")
    TArray<FWMPerformanceProfileDefinition> Profiles;

    bool IsSane() const;
    const FWMPerformanceProfileDefinition* FindProfile(FName ProfileId) const;
    static bool TryParseJson(const FString& Json, FWMPerformanceProfileCatalog& OutCatalog, FString& OutError);
};

USTRUCT(BlueprintType)
struct WORLDMAKERS_API FWMPerformanceCaptureSummary
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Performance|Capture")
    FName ProfileId;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Performance|Capture")
    int32 FrameSampleCount = 0;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Performance|Capture")
    float AverageFrameTimeMs = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Performance|Capture")
    float P95FrameTimeMs = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Performance|Capture")
    float WorstFrameTimeMs = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Performance|Capture")
    int32 MaxWorldActors = 0;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Performance|Capture")
    int32 MaxPlacedBuildPieces = 0;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Performance|Capture")
    int32 MaxActiveInteractables = 0;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Performance|Capture")
    bool bFrameTimeWithinBudget = false;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Performance|Capture")
    bool bActorCountWithinBudget = false;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Performance|Capture")
    bool bBuildPieceCountWithinBudget = false;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Performance|Capture")
    bool bInteractableCountWithinBudget = false;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Performance|Capture")
    bool bWithinBudget = false;

    bool HasSamples() const { return FrameSampleCount > 0; }
    FString ToJson() const;
};

/** Pure deterministic accumulator used by runtime capture and automation tests. */
struct WORLDMAKERS_API FWMPerformanceCaptureAccumulator
{
    void Reset();
    void AddFrameTimeMs(float FrameTimeMs);
    void ObserveStructuralCounts(int32 WorldActorCount, int32 PlacedBuildPieceCount, int32 ActiveInteractableCount);
    FWMPerformanceCaptureSummary BuildSummary(FName ProfileId, const FWMPerformanceBudget& Budget) const;

private:
    TArray<float> FrameTimesMs;
    int32 MaxWorldActors = 0;
    int32 MaxPlacedBuildPieces = 0;
    int32 MaxActiveInteractables = 0;
};
