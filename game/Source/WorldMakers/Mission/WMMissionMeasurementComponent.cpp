#include "Mission/WMMissionMeasurementComponent.h"

#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Mission/WMMissionGeometryActor.h"
#include "Mission/WMMissionGeometryLibrary.h"
#include "Mission/WMMissionRuntimeSubsystem.h"

void FWMMissionMeasurementModel::Begin(const FName InMissionId)
{
    MissionId = InMissionId;
    StartPoint = FVector::ZeroVector;
    EndPoint = FVector::ZeroVector;
    LastDistanceCm = 0.0f;
    State = EWMMissionMeasurementState::AwaitingFirstPoint;
}

void FWMMissionMeasurementModel::Reset()
{
    MissionId = NAME_None;
    StartPoint = FVector::ZeroVector;
    EndPoint = FVector::ZeroVector;
    LastDistanceCm = 0.0f;
    State = EWMMissionMeasurementState::Idle;
}

bool FWMMissionMeasurementModel::CapturePoint(
    const FVector& WorldPoint,
    const bool bInsideMissionZone,
    const float MinPointSeparationCm,
    float& OutDistanceCm)
{
    OutDistanceCm = 0.0f;
    if (!bInsideMissionZone || WorldPoint.ContainsNaN())
    {
        return false;
    }

    if (State == EWMMissionMeasurementState::AwaitingFirstPoint)
    {
        StartPoint = WorldPoint;
        EndPoint = FVector::ZeroVector;
        LastDistanceCm = 0.0f;
        State = EWMMissionMeasurementState::AwaitingSecondPoint;
        return true;
    }

    if (State != EWMMissionMeasurementState::AwaitingSecondPoint)
    {
        return false;
    }

    const float DistanceCm = FVector::Dist(StartPoint, WorldPoint);
    if (!FMath::IsFinite(DistanceCm) || DistanceCm < FMath::Max(1.0f, MinPointSeparationCm))
    {
        return false;
    }

    EndPoint = WorldPoint;
    LastDistanceCm = DistanceCm;
    OutDistanceCm = DistanceCm;
    State = EWMMissionMeasurementState::Complete;
    return true;
}

UWMMissionMeasurementComponent::UWMMissionMeasurementComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

bool UWMMissionMeasurementComponent::TracePointFromView(FVector& OutWorldPoint) const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }

    APlayerController* PlayerController = nullptr;
    if (const APawn* PawnOwner = Cast<APawn>(GetOwner()))
    {
        PlayerController = Cast<APlayerController>(PawnOwner->GetController());
    }
    if (!PlayerController)
    {
        PlayerController = World->GetFirstPlayerController();
    }
    if (!PlayerController)
    {
        return false;
    }

    FVector ViewLocation;
    FRotator ViewRotation;
    PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);
    const FVector TraceEnd = ViewLocation + (ViewRotation.Vector() * MeasurementTraceDistanceCm);

    FHitResult Hit;
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WorldMakersMissionMeasurementTrace), false, GetOwner());
    if (!World->LineTraceSingleByChannel(Hit, ViewLocation, TraceEnd, ECC_Visibility, QueryParams) || !Hit.bBlockingHit)
    {
        return false;
    }

    OutWorldPoint = Hit.ImpactPoint;
    return true;
}

bool UWMMissionMeasurementComponent::CapturePointFromView()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }

    UWMMissionRuntimeSubsystem* Missions = World->GetSubsystem<UWMMissionRuntimeSubsystem>();
    AWMMissionGeometryActor* Geometry = Missions ? Missions->GetActiveMissionGeometry() : nullptr;
    if (!Missions || !Geometry || Missions->GetMissionState() != EWMMissionRuntimeState::Active)
    {
        return false;
    }

    const FName ActiveMissionId = Missions->GetActiveMissionId();
    if (ActiveMissionId.IsNone())
    {
        return false;
    }

    if (Model.MissionId != ActiveMissionId || Model.State == EWMMissionMeasurementState::Idle || Model.State == EWMMissionMeasurementState::Complete)
    {
        Geometry->ClearInteractiveMeasurement();
        Model.Begin(ActiveMissionId);
    }

    FVector WorldPoint;
    if (!TracePointFromView(WorldPoint))
    {
        return false;
    }

    const bool bInsideZone = UWMMissionGeometryLibrary::IsWorldPointInsideBox(
        WorldPoint,
        Geometry->GetBuildZoneTransform(),
        Geometry->GetBuildZoneHalfExtent());

    float CompletedDistanceCm = 0.0f;
    if (!Model.CapturePoint(WorldPoint, bInsideZone, MinPointSeparationCm, CompletedDistanceCm))
    {
        return false;
    }

    if (Model.State == EWMMissionMeasurementState::AwaitingSecondPoint)
    {
        Geometry->SetInteractiveMeasurementStart(Model.StartPoint);
        return true;
    }

    if (Model.State == EWMMissionMeasurementState::Complete)
    {
        Geometry->SetInteractiveMeasurementComplete(Model.StartPoint, Model.EndPoint);
        if (!Missions->RecordMeasurement(CompletedDistanceCm))
        {
            Geometry->ClearInteractiveMeasurement();
            Model.Begin(ActiveMissionId);
            return false;
        }
        return true;
    }

    return false;
}

void UWMMissionMeasurementComponent::ResetMeasurement()
{
    if (UWorld* World = GetWorld())
    {
        if (UWMMissionRuntimeSubsystem* Missions = World->GetSubsystem<UWMMissionRuntimeSubsystem>())
        {
            if (AWMMissionGeometryActor* Geometry = Missions->GetActiveMissionGeometry())
            {
                Geometry->ClearInteractiveMeasurement();
            }
        }
    }
    Model.Reset();
}
