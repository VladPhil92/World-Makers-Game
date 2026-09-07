#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WMMissionMeasurementComponent.generated.h"

UENUM(BlueprintType)
enum class EWMMissionMeasurementState : uint8
{
    Idle,
    AwaitingFirstPoint,
    AwaitingSecondPoint,
    Complete
};

/** Pure deterministic state model used by the interactive component and automation tests. */
struct WORLDMAKERS_API FWMMissionMeasurementModel
{
    EWMMissionMeasurementState State = EWMMissionMeasurementState::Idle;
    FName MissionId = NAME_None;
    FVector StartPoint = FVector::ZeroVector;
    FVector EndPoint = FVector::ZeroVector;
    float LastDistanceCm = 0.0f;

    void Begin(FName InMissionId);
    void Reset();
    bool CapturePoint(const FVector& WorldPoint, bool bInsideMissionZone, float MinPointSeparationCm, float& OutDistanceCm);
};

UCLASS(ClassGroup = (WorldMakers), meta = (BlueprintSpawnableComponent))
class WORLDMAKERS_API UWMMissionMeasurementComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UWMMissionMeasurementComponent();

    /** Captures the point currently under the player's center view. Two valid captures complete one measurement. */
    UFUNCTION(BlueprintCallable, Category = "World Makers|Missions|Measurement")
    bool CapturePointFromView();

    UFUNCTION(BlueprintCallable, Category = "World Makers|Missions|Measurement")
    void ResetMeasurement();

    UFUNCTION(BlueprintPure, Category = "World Makers|Missions|Measurement")
    EWMMissionMeasurementState GetMeasurementState() const { return Model.State; }

    UFUNCTION(BlueprintPure, Category = "World Makers|Missions|Measurement")
    FVector GetStartPoint() const { return Model.StartPoint; }

    UFUNCTION(BlueprintPure, Category = "World Makers|Missions|Measurement")
    FVector GetEndPoint() const { return Model.EndPoint; }

    UFUNCTION(BlueprintPure, Category = "World Makers|Missions|Measurement")
    float GetMeasuredDistanceCm() const { return Model.LastDistanceCm; }

    UFUNCTION(BlueprintPure, Category = "World Makers|Missions|Measurement")
    FName GetMeasurementMissionId() const { return Model.MissionId; }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Makers|Missions|Measurement", meta = (ClampMin = "250.0"))
    float MeasurementTraceDistanceCm = 2500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Makers|Missions|Measurement", meta = (ClampMin = "1.0"))
    float MinPointSeparationCm = 5.0f;

private:
    bool TracePointFromView(FVector& OutWorldPoint) const;
    FWMMissionMeasurementModel Model;
};
