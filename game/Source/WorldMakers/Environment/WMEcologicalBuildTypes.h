#pragma once

#include "CoreMinimal.h"
#include "Building/WMBuildWorldStateSubsystem.h"
#include "Environment/WMEnvironmentStateTypes.h"
#include "WMEcologicalBuildTypes.generated.h"

USTRUCT(BlueprintType)
struct WORLDMAKERS_API FWMEcologicalPieceRequirement
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Environment|Building")
    FName PieceId;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Environment|Building")
    int32 MinCount = 1;

    bool IsSane() const;
};

USTRUCT(BlueprintType)
struct WORLDMAKERS_API FWMEcologicalBuildInterventionDefinition
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Environment|Building")
    FName InterventionId;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Environment|Building")
    FVector AnchorCm = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Environment|Building")
    float RadiusCm = 300.0f;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Environment|Building")
    TArray<FName> PrerequisiteInterventionIds;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Environment|Building")
    TArray<FWMEcologicalPieceRequirement> Requirements;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Environment|Building")
    FWMEnvironmentStateDelta EffectDelta;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Environment|Building")
    FName RewardId;

    bool IsSane() const;
    int32 CountSatisfiedRequirements(const TArray<FWMPlacedBuildPieceSnapshot>& Pieces) const;
    bool IsSatisfiedBy(const TArray<FWMPlacedBuildPieceSnapshot>& Pieces) const;
};

USTRUCT(BlueprintType)
struct WORLDMAKERS_API FWMEcologicalBuildDefinition
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Environment|Building")
    int32 SchemaVersion = 1;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Environment|Building")
    FName BiomeId;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Environment|Building")
    bool bPrototypeOnly = true;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Environment|Building")
    TArray<FWMEcologicalBuildInterventionDefinition> Interventions;

    bool IsSane() const;
    const FWMEcologicalBuildInterventionDefinition* FindIntervention(FName InterventionId) const;
    static bool TryParseJson(const FString& Json, FWMEcologicalBuildDefinition& OutDefinition, FString& OutError);
};

USTRUCT(BlueprintType)
struct WORLDMAKERS_API FWMEcologicalBuildInterventionReadModel
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Environment|Building")
    FName InterventionId;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Environment|Building")
    bool bCompleted = false;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Environment|Building")
    bool bPrerequisitesSatisfied = false;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Environment|Building")
    int32 SatisfiedRequirementCount = 0;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Environment|Building")
    int32 RequirementCount = 0;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Environment|Building")
    FName RewardId;
};

/** Session-local completion model. Creative reward grants persist separately in Building. */
struct WORLDMAKERS_API FWMEcologicalBuildProgressModel
{
    bool ArePrerequisitesSatisfied(const FWMEcologicalBuildInterventionDefinition& Intervention) const;
    bool CanComplete(const FWMEcologicalBuildInterventionDefinition& Intervention, const TArray<FWMPlacedBuildPieceSnapshot>& Pieces) const;
    bool MarkCompleted(FName InterventionId);
    bool IsCompleted(FName InterventionId) const;
    TArray<FName> GetCompletedInterventionIds() const;
    void Reset();

private:
    TSet<FName> CompletedInterventionIds;
};
