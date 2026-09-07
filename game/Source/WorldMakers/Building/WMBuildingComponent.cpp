#include "Building/WMBuildingComponent.h"

#include "Building/WMBuildGridLibrary.h"
#include "Building/WMBuildPieceActor.h"
#include "Building/WMWorldSaveGame.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

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

bool UWMBuildingComponent::UpdatePreviewTransform()
{
    EnsurePreviewActor();
    if (!IsValid(PreviewActor))
    {
        bHasPlacementTarget = false;
        return false;
    }

    FHitResult SurfaceHit;
    if (!GetViewTrace(SurfaceHit, true))
    {
        bHasPlacementTarget = false;
        PreviewActor->SetActorHiddenInGame(true);
        return false;
    }

    const FVector SnappedLocation = UWMBuildGridLibrary::SnapLocationToGrid(SurfaceHit.ImpactPoint, GridSize, true);
    const float SnappedYaw = UWMBuildGridLibrary::SnapYawToStep(CurrentYaw, RotationStepDegrees);
    PreviewActor->SetActorTransform(FTransform(FRotator(0.0f, SnappedYaw, 0.0f), SnappedLocation));
    PreviewActor->SetActorHiddenInGame(false);
    bHasPlacementTarget = true;
    return true;
}

AWMBuildPieceActor* UWMBuildingComponent::SpawnPlacedPiece(const FTransform& Transform, const FName PieceId)
{
    if (!GetWorld() || !BuildPieceClass)
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

        FWMBuildSaveRecord Record;
        Record.PieceId = Piece->PieceId;
        Record.Transform = Piece->GetActorTransform();
        SaveData->Pieces.Add(Record);
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
    if (!SaveData || SaveData->SaveFormatVersion != 1)
    {
        return false;
    }

    DestroyAllPlacedPieces();
    for (const FWMBuildSaveRecord& Record : SaveData->Pieces)
    {
        SpawnPlacedPiece(Record.Transform, Record.PieceId);
    }

    UndoStack.Reset();
    RedoStack.Reset();
    return true;
}
