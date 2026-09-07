#include "Building/WMBuildingComponent.h"

#include "Building/WMBuildGridLibrary.h"
#include "Building/WMBuildPieceActor.h"
#include "Building/WMWorldSaveGame.h"
#include "CollisionShape.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

namespace
{
    bool IsSafeBuildTransform(const FTransform& Transform)
    {
        if (Transform.ContainsNaN() || !Transform.GetRotation().IsNormalized())
        {
            return false;
        }

        const FVector Location = Transform.GetLocation();
        const FVector Scale = Transform.GetScale3D();
        const bool bFinite =
            FMath::IsFinite(Location.X) && FMath::IsFinite(Location.Y) && FMath::IsFinite(Location.Z) &&
            FMath::IsFinite(Scale.X) && FMath::IsFinite(Scale.Y) && FMath::IsFinite(Scale.Z);
        const bool bScaleReasonable =
            FMath::Abs(Scale.X) >= 0.01f && FMath::Abs(Scale.X) <= 100.0f &&
            FMath::Abs(Scale.Y) >= 0.01f && FMath::Abs(Scale.Y) <= 100.0f &&
            FMath::Abs(Scale.Z) >= 0.01f && FMath::Abs(Scale.Z) <= 100.0f;
        const bool bLocationReasonable =
            FMath::Abs(Location.X) <= 10000000.0f &&
            FMath::Abs(Location.Y) <= 10000000.0f &&
            FMath::Abs(Location.Z) <= 10000000.0f;

        return bFinite && bScaleReasonable && bLocationReasonable;
    }
}

UWMBuildingComponent::UWMBuildingComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    BuildPieceClass = AWMBuildPieceActor::StaticClass();
}

void UWMBuildingComponent::BeginPlay()
{
    Super::BeginPlay();
    EnsurePreviewActor();
}

void UWMBuildingComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (IsValid(PreviewActor))
    {
        PreviewActor->Destroy();
    }

    Super::EndPlay(EndPlayReason);
}

void UWMBuildingComponent::TickComponent(
    const float DeltaTime,
    const ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    UpdatePreviewTransform();
}

void UWMBuildingComponent::EnsurePreviewActor()
{
    if (IsValid(PreviewActor) || !GetWorld() || !BuildPieceClass)
    {
        return;
    }

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.Owner = GetOwner();
    SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    PreviewActor = GetWorld()->SpawnActor<AWMBuildPieceActor>(BuildPieceClass, FTransform::Identity, SpawnParameters);
    if (IsValid(PreviewActor))
    {
        PreviewActor->SetPreviewState(true);
        PreviewActor->SetActorHiddenInGame(true);
    }
}

bool UWMBuildingComponent::GetViewTrace(FHitResult& OutHit, const bool bIgnorePreview) const
{
    if (!GetWorld())
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
        PlayerController = GetWorld()->GetFirstPlayerController();
    }

    if (!PlayerController)
    {
        return false;
    }

    FVector ViewLocation;
    FRotator ViewRotation;
    PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);

    const FVector TraceEnd = ViewLocation + (ViewRotation.Vector() * BuildDistance);
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WorldMakersBuildTrace), false, GetOwner());
    if (bIgnorePreview && IsValid(PreviewActor))
    {
        QueryParams.AddIgnoredActor(PreviewActor);
    }

    return GetWorld()->LineTraceSingleByChannel(OutHit, ViewLocation, TraceEnd, ECC_Visibility, QueryParams);
}

bool UWMBuildingComponent::IsPlacementValid(const FTransform& CandidateTransform, const AActor* SupportingActor) const
{
    if (!GetWorld() || !IsSafeBuildTransform(CandidateTransform))
    {
        return false;
    }

    const float Clearance = FMath::Max(GridSize * FMath::Clamp(PlacementClearanceRatio, 0.10f, 0.49f), 1.0f);
    const FCollisionShape PlacementShape = FCollisionShape::MakeBox(FVector(Clearance));
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WorldMakersPlacementOverlap), false, GetOwner());

    if (IsValid(PreviewActor))
    {
        QueryParams.AddIgnoredActor(PreviewActor);
    }
    if (IsValid(SupportingActor))
    {
        QueryParams.AddIgnoredActor(SupportingActor);
    }

    TArray<FOverlapResult> Overlaps;
    const bool bHasBlockingOverlap = GetWorld()->OverlapMultiByChannel(
        Overlaps,
        CandidateTransform.GetLocation(),
        CandidateTransform.GetRotation(),
        ECC_Visibility,
        PlacementShape,
        QueryParams,
        FCollisionResponseParams::DefaultResponseParam);

    return !bHasBlockingOverlap;
}

bool UWMBuildingComponent::UpdatePreviewTransform()
{
    EnsurePreviewActor();
    if (!IsValid(PreviewActor))
    {
        bHasPlacementTarget = false;
        return false;
    }

    FHitResult SurfaceHit;
    if (!GetViewTrace(SurfaceHit, true) || SurfaceHit.ImpactNormal.Z < MinPlacementSurfaceUpDot)
    {
        bHasPlacementTarget = false;
        PreviewActor->SetActorHiddenInGame(true);
        return false;
    }

    const FVector SnappedLocation = UWMBuildGridLibrary::SnapLocationToSurfaceGrid(SurfaceHit.ImpactPoint, GridSize);
    const float SnappedYaw = UWMBuildGridLibrary::SnapYawToStep(CurrentYaw, RotationStepDegrees);
    const FTransform CandidateTransform(FRotator(0.0f, SnappedYaw, 0.0f), SnappedLocation);

    PreviewActor->SetActorTransform(CandidateTransform);
    bHasPlacementTarget = IsPlacementValid(CandidateTransform, SurfaceHit.GetActor());
    PreviewActor->SetActorHiddenInGame(!bHasPlacementTarget);
    return bHasPlacementTarget;
}

AWMBuildPieceActor* UWMBuildingComponent::SpawnPlacedPiece(const FTransform& Transform, const FName PieceId)
{
    if (!GetWorld() || !BuildPieceClass || !IsSafeBuildTransform(Transform))
    {
        return nullptr;
    }

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.Owner = GetOwner();
    SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AWMBuildPieceActor* Piece = GetWorld()->SpawnActor<AWMBuildPieceActor>(BuildPieceClass, Transform, SpawnParameters);
    if (Piece)
    {
        Piece->PieceId = PieceId.IsNone() ? FName(TEXT("prototype.cube")) : PieceId;
        Piece->SetPreviewState(false);
    }

    return Piece;
}

void UWMBuildingComponent::PushCommand(const FWMBuildCommand& Command)
{
    UndoStack.Add(Command);
    if (UndoStack.Num() > MaxHistoryEntries)
    {
        UndoStack.RemoveAt(0, UndoStack.Num() - MaxHistoryEntries);
    }
    RedoStack.Reset();
}

bool UWMBuildingComponent::TryPlaceCurrentPiece()
{
    if (!UpdatePreviewTransform() || !bHasPlacementTarget || !IsValid(PreviewActor))
    {
        return false;
    }

    const FTransform PlacementTransform = PreviewActor->GetActorTransform();
    if (!IsPlacementValid(PlacementTransform, nullptr))
    {
        bHasPlacementTarget = false;
        PreviewActor->SetActorHiddenInGame(true);
        return false;
    }

    AWMBuildPieceActor* PlacedPiece = SpawnPlacedPiece(PlacementTransform, PreviewActor->PieceId);
    if (!PlacedPiece)
    {
        return false;
    }

    FWMBuildCommand Command;
    Command.Type = EWMBuildCommandType::Place;
    Command.Transform = PlacementTransform;
    Command.PieceId = PlacedPiece->PieceId;
    Command.ActiveActor = PlacedPiece;
    PushCommand(Command);
    return true;
}

bool UWMBuildingComponent::TryRemoveTargetPiece()
{
    FHitResult Hit;
    if (!GetViewTrace(Hit, true))
    {
        return false;
    }

    AWMBuildPieceActor* TargetPiece = Cast<AWMBuildPieceActor>(Hit.GetActor());
    if (!IsValid(TargetPiece) || TargetPiece->IsPreview() || !TargetPiece->ActorHasTag(AWMBuildPieceActor::PlacedBuildTag))
    {
        return false;
    }

    FWMBuildCommand Command;
    Command.Type = EWMBuildCommandType::Remove;
    Command.Transform = TargetPiece->GetActorTransform();
    Command.PieceId = TargetPiece->PieceId;

    TargetPiece->Destroy();
    PushCommand(Command);
    return true;
}

void UWMBuildingComponent::RotatePreview(const float Direction)
{
    CurrentYaw += RotationStepDegrees * FMath::Sign(Direction);
    CurrentYaw = UWMBuildGridLibrary::SnapYawToStep(CurrentYaw, RotationStepDegrees);
    UpdatePreviewTransform();
}

bool UWMBuildingComponent::UndoLastAction()
{
    if (UndoStack.IsEmpty())
    {
        return false;
    }

    FWMBuildCommand Command = UndoStack.Pop();
    bool bSucceeded = false;

    if (Command.Type == EWMBuildCommandType::Place)
    {
        if (Command.ActiveActor.IsValid())
        {
            Command.ActiveActor->Destroy();
            Command.ActiveActor.Reset();
            bSucceeded = true;
        }
    }
    else
    {
        if (AWMBuildPieceActor* Restored = SpawnPlacedPiece(Command.Transform, Command.PieceId))
        {
            Command.ActiveActor = Restored;
            bSucceeded = true;
        }
    }

    if (bSucceeded)
    {
        RedoStack.Add(Command);
    }
    else
    {
        UndoStack.Add(Command);
    }

    return bSucceeded;
}

bool UWMBuildingComponent::RedoLastAction()
{
    if (RedoStack.IsEmpty())
    {
        return false;
    }

    FWMBuildCommand Command = RedoStack.Pop();
    bool bSucceeded = false;

    if (Command.Type == EWMBuildCommandType::Place)
    {
        if (AWMBuildPieceActor* Restored = SpawnPlacedPiece(Command.Transform, Command.PieceId))
        {
            Command.ActiveActor = Restored;
            bSucceeded = true;
        }
    }
    else if (Command.ActiveActor.IsValid())
    {
        Command.ActiveActor->Destroy();
        Command.ActiveActor.Reset();
        bSucceeded = true;
    }

    if (bSucceeded)
    {
        UndoStack.Add(Command);
    }
    else
    {
        RedoStack.Add(Command);
    }

    return bSucceeded;
}

bool UWMBuildingComponent::SaveWorld(const FString& SlotName)
{
    UWMWorldSaveGame* SaveData = Cast<UWMWorldSaveGame>(
        UGameplayStatics::CreateSaveGameObject(UWMWorldSaveGame::StaticClass()));
    if (!SaveData)
    {
        return false;
    }

    TArray<AActor*> BuildActors;
    UGameplayStatics::GetAllActorsOfClass(this, AWMBuildPieceActor::StaticClass(), BuildActors);
    for (AActor* Actor : BuildActors)
    {
        AWMBuildPieceActor* Piece = Cast<AWMBuildPieceActor>(Actor);
        if (!IsValid(Piece) || Piece->IsPreview() || !Piece->ActorHasTag(AWMBuildPieceActor::PlacedBuildTag))
        {
            continue;
        }
        if (SaveData->Pieces.Num() >= MaxSavedPieces)
        {
            return false;
        }

        FWMBuildSaveRecord Record;
        Record.PieceId = Piece->PieceId;
        Record.Transform = Piece->GetActorTransform();
        if (!Record.PieceId.IsNone() && IsSafeBuildTransform(Record.Transform))
        {
            SaveData->Pieces.Add(Record);
        }
    }

    return UGameplayStatics::SaveGameToSlot(SaveData, SlotName, 0);
}

void UWMBuildingComponent::DestroyAllPlacedPieces()
{
    TArray<AActor*> BuildActors;
    UGameplayStatics::GetAllActorsOfClass(this, AWMBuildPieceActor::StaticClass(), BuildActors);
    for (AActor* Actor : BuildActors)
    {
        AWMBuildPieceActor* Piece = Cast<AWMBuildPieceActor>(Actor);
        if (IsValid(Piece) && !Piece->IsPreview() && Piece->ActorHasTag(AWMBuildPieceActor::PlacedBuildTag))
        {
            Piece->Destroy();
        }
    }
}

bool UWMBuildingComponent::LoadWorld(const FString& SlotName)
{
    if (!UGameplayStatics::DoesSaveGameExist(SlotName, 0))
    {
        return false;
    }

    UWMWorldSaveGame* SaveData = Cast<UWMWorldSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
    if (!SaveData || SaveData->SaveFormatVersion != 1 || SaveData->Pieces.Num() > MaxSavedPieces)
    {
        return false;
    }

    for (const FWMBuildSaveRecord& Record : SaveData->Pieces)
    {
        if (Record.PieceId.IsNone() || !IsSafeBuildTransform(Record.Transform))
        {
            return false;
        }
    }

    DestroyAllPlacedPieces();
    for (const FWMBuildSaveRecord& Record : SaveData->Pieces)
    {
        if (!SpawnPlacedPiece(Record.Transform, Record.PieceId))
        {
            DestroyAllPlacedPieces();
            return false;
        }
    }

    UndoStack.Reset();
    RedoStack.Reset();
    return true;
}
