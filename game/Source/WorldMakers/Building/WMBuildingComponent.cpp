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
    SelectPiece(SelectedPieceId);
}

void UWMBuildingComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    CancelMove();
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

bool UWMBuildingComponent::ResolvePieceSpec(const FName PieceId, FWMBuildPieceSpec& OutSpec) const
{
    const UWMBuildCatalogSettings* Catalog = GetDefault<UWMBuildCatalogSettings>();
    return Catalog && Catalog->FindPieceSpec(PieceId, OutSpec);
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

bool UWMBuildingComponent::SelectPiece(const FName PieceId)
{
    if (IsMoveInProgress())
    {
        return false;
    }

    FWMBuildPieceSpec Spec;
    if (!ResolvePieceSpec(PieceId, Spec))
    {
        return false;
    }

    SelectedPieceId = PieceId;
    CurrentYaw = UWMBuildGridLibrary::SnapYawToStep(CurrentYaw, Spec.RotationStepDegrees);
    EnsurePreviewActor();
    if (IsValid(PreviewActor))
    {
        PreviewActor->ApplyPieceSpec(Spec);
    }
    UpdatePreviewTransform();
    return true;
}

bool UWMBuildingComponent::CycleSelectedPiece(const int32 Direction)
{
    if (IsMoveInProgress())
    {
        return false;
    }

    const UWMBuildCatalogSettings* Catalog = GetDefault<UWMBuildCatalogSettings>();
    if (!Catalog)
    {
        return false;
    }

    TArray<FName> PieceIds;
    Catalog->GetPieceIds(PieceIds);
    if (PieceIds.IsEmpty())
    {
        return false;
    }

    int32 Index = PieceIds.IndexOfByKey(SelectedPieceId);
    if (Index == INDEX_NONE)
    {
        Index = 0;
    }
    else
    {
        const int32 Step = Direction >= 0 ? 1 : -1;
        Index = (Index + Step + PieceIds.Num()) % PieceIds.Num();
    }

    return SelectPiece(PieceIds[Index]);
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
    if (MovingActor.IsValid())
    {
        QueryParams.AddIgnoredActor(MovingActor.Get());
    }

    return GetWorld()->LineTraceSingleByChannel(OutHit, ViewLocation, TraceEnd, ECC_Visibility, QueryParams);
}

bool UWMBuildingComponent::IsPlacementValid(
    const FTransform& CandidateTransform,
    const FWMBuildPieceSpec& Spec,
    const AActor* SupportingActor) const
{
    if (!GetWorld() || !IsSafeBuildTransform(CandidateTransform) || !Spec.IsSane())
    {
        return false;
    }

    if (const AWMBuildPieceActor* SupportPiece = Cast<AWMBuildPieceActor>(SupportingActor))
    {
        FWMBuildPieceSpec SupportSpec;
        if (!ResolvePieceSpec(SupportPiece->PieceId, SupportSpec) || !SupportSpec.bCanBeSupport)
        {
            return false;
        }
    }

    const FVector HalfExtent = (Spec.DimensionsCm * 0.5f * FMath::Clamp(PlacementBoundsScale, 0.10f, 1.0f)).ComponentMax(FVector(1.0f));
    const FCollisionShape PlacementShape = FCollisionShape::MakeBox(HalfExtent);
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(WorldMakersPlacementOverlap), false, GetOwner());

    if (IsValid(PreviewActor))
    {
        QueryParams.AddIgnoredActor(PreviewActor);
    }
    if (IsValid(SupportingActor))
    {
        QueryParams.AddIgnoredActor(SupportingActor);
    }
    if (MovingActor.IsValid())
    {
        QueryParams.AddIgnoredActor(MovingActor.Get());
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
    FWMBuildPieceSpec Spec;
    if (!IsValid(PreviewActor) || !ResolvePieceSpec(SelectedPieceId, Spec))
    {
        bHasPlacementTarget = false;
        return false;
    }

    PreviewActor->ApplyPieceSpec(Spec);

    FHitResult SurfaceHit;
    const float RequiredUpDot = FMath::Max(MinPlacementSurfaceUpDot, Spec.MinSurfaceUpDot);
    if (!GetViewTrace(SurfaceHit, true) || SurfaceHit.ImpactNormal.Z < RequiredUpDot)
    {
        bHasPlacementTarget = false;
        PreviewSupportingActor.Reset();
        PreviewActor->SetPreviewValidity(false);
        PreviewActor->SetActorHiddenInGame(true);
        return false;
    }

    const FVector SnappedLocation = UWMBuildGridLibrary::SnapLocationToSurfaceGrid(
        SurfaceHit.ImpactPoint,
        GridSize,
        Spec.DimensionsCm.Z);
    const float SnappedYaw = UWMBuildGridLibrary::SnapYawToStep(CurrentYaw, Spec.RotationStepDegrees);
    const FTransform CandidateTransform(FRotator(0.0f, SnappedYaw, 0.0f), SnappedLocation);

    PreviewActor->SetActorTransform(CandidateTransform);
    PreviewSupportingActor = SurfaceHit.GetActor();
    bHasPlacementTarget = IsPlacementValid(CandidateTransform, Spec, PreviewSupportingActor.Get());
    PreviewActor->SetPreviewValidity(bHasPlacementTarget);
    PreviewActor->SetActorHiddenInGame(false);
    return bHasPlacementTarget;
}

AWMBuildPieceActor* UWMBuildingComponent::SpawnPlacedPiece(const FTransform& Transform, const FName PieceId)
{
    FWMBuildPieceSpec Spec;
    if (!GetWorld() || !BuildPieceClass || !IsSafeBuildTransform(Transform) || !ResolvePieceSpec(PieceId, Spec))
    {
        return nullptr;
    }

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.Owner = GetOwner();
    SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AWMBuildPieceActor* Piece = GetWorld()->SpawnActor<AWMBuildPieceActor>(BuildPieceClass, Transform, SpawnParameters);
    if (Piece)
    {
        Piece->ApplyPieceSpec(Spec);
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
    if (IsMoveInProgress())
    {
        return CommitMove();
    }

    FWMBuildPieceSpec Spec;
    if (!ResolvePieceSpec(SelectedPieceId, Spec) || !UpdatePreviewTransform() || !bHasPlacementTarget || !IsValid(PreviewActor))
    {
        return false;
    }

    const FTransform PlacementTransform = PreviewActor->GetActorTransform();
    if (!IsPlacementValid(PlacementTransform, Spec, PreviewSupportingActor.Get()))
    {
        bHasPlacementTarget = false;
        PreviewActor->SetPreviewValidity(false);
        return false;
    }

    AWMBuildPieceActor* PlacedPiece = SpawnPlacedPiece(PlacementTransform, SelectedPieceId);
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
    if (IsMoveInProgress())
    {
        return false;
    }

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

bool UWMBuildingComponent::TryBeginMoveTargetPiece()
{
    if (IsMoveInProgress())
    {
        return false;
    }

    FHitResult Hit;
    if (!GetViewTrace(Hit, true))
    {
        return false;
    }

    AWMBuildPieceActor* TargetPiece = Cast<AWMBuildPieceActor>(Hit.GetActor());
    FWMBuildPieceSpec Spec;
    if (!IsValid(TargetPiece) || TargetPiece->IsPreview() ||
        !TargetPiece->ActorHasTag(AWMBuildPieceActor::PlacedBuildTag) ||
        !ResolvePieceSpec(TargetPiece->PieceId, Spec))
    {
        return false;
    }

    MovingActor = TargetPiece;
    MoveOriginalTransform = TargetPiece->GetActorTransform();
    SelectedPieceId = TargetPiece->PieceId;
    CurrentYaw = TargetPiece->GetActorRotation().Yaw;
    TargetPiece->SetActorHiddenInGame(true);
    TargetPiece->SetActorEnableCollision(false);

    EnsurePreviewActor();
    if (IsValid(PreviewActor))
    {
        PreviewActor->ApplyPieceSpec(Spec);
    }
    UpdatePreviewTransform();
    return true;
}

bool UWMBuildingComponent::CommitMove()
{
    if (!MovingActor.IsValid() || !UpdatePreviewTransform() || !bHasPlacementTarget || !IsValid(PreviewActor))
    {
        return false;
    }

    FWMBuildPieceSpec Spec;
    if (!ResolvePieceSpec(SelectedPieceId, Spec))
    {
        return false;
    }

    const FTransform NewTransform = PreviewActor->GetActorTransform();
    if (!IsPlacementValid(NewTransform, Spec, PreviewSupportingActor.Get()))
    {
        return false;
    }

    AWMBuildPieceActor* Actor = MovingActor.Get();
    Actor->SetActorTransform(NewTransform);
    Actor->SetActorHiddenInGame(false);
    Actor->SetPreviewState(false);

    FWMBuildCommand Command;
    Command.Type = EWMBuildCommandType::Move;
    Command.PreviousTransform = MoveOriginalTransform;
    Command.Transform = NewTransform;
    Command.PieceId = Actor->PieceId;
    Command.ActiveActor = Actor;
    PushCommand(Command);

    MovingActor.Reset();
    MoveOriginalTransform = FTransform::Identity;
    UpdatePreviewTransform();
    return true;
}

bool UWMBuildingComponent::CancelMove()
{
    if (!MovingActor.IsValid())
    {
        return false;
    }

    AWMBuildPieceActor* Actor = MovingActor.Get();
    Actor->SetActorTransform(MoveOriginalTransform);
    Actor->SetActorHiddenInGame(false);
    Actor->SetPreviewState(false);
    MovingActor.Reset();
    MoveOriginalTransform = FTransform::Identity;
    UpdatePreviewTransform();
    return true;
}

void UWMBuildingComponent::RotatePreview(const float Direction)
{
    FWMBuildPieceSpec Spec;
    if (!ResolvePieceSpec(SelectedPieceId, Spec))
    {
        return;
    }

    CurrentYaw += Spec.RotationStepDegrees * FMath::Sign(Direction);
    CurrentYaw = UWMBuildGridLibrary::SnapYawToStep(CurrentYaw, Spec.RotationStepDegrees);
    UpdatePreviewTransform();
}

bool UWMBuildingComponent::UndoLastAction()
{
    if (IsMoveInProgress() || UndoStack.IsEmpty())
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
    else if (Command.Type == EWMBuildCommandType::Remove)
    {
        if (AWMBuildPieceActor* Restored = SpawnPlacedPiece(Command.Transform, Command.PieceId))
        {
            Command.ActiveActor = Restored;
            bSucceeded = true;
        }
    }
    else if (Command.ActiveActor.IsValid())
    {
        Command.ActiveActor->SetActorTransform(Command.PreviousTransform);
        bSucceeded = true;
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
    if (IsMoveInProgress() || RedoStack.IsEmpty())
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
    else if (Command.Type == EWMBuildCommandType::Remove)
    {
        if (Command.ActiveActor.IsValid())
        {
            Command.ActiveActor->Destroy();
            Command.ActiveActor.Reset();
            bSucceeded = true;
        }
    }
    else if (Command.ActiveActor.IsValid())
    {
        Command.ActiveActor->SetActorTransform(Command.Transform);
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
        FWMBuildPieceSpec Spec;
        if (!IsValid(Piece) || Piece->IsPreview() ||
            !Piece->ActorHasTag(AWMBuildPieceActor::PlacedBuildTag) ||
            !ResolvePieceSpec(Piece->PieceId, Spec))
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
    if (IsMoveInProgress() || !UGameplayStatics::DoesSaveGameExist(SlotName, 0))
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
        FWMBuildPieceSpec Spec;
        if (Record.PieceId.IsNone() || !ResolvePieceSpec(Record.PieceId, Spec) || !IsSafeBuildTransform(Record.Transform))
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
