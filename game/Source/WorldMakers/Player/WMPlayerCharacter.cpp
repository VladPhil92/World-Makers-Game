#include "Player/WMPlayerCharacter.h"

#include "Building/WMBuildingComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Environment/WMExplorationComponent.h"
#include "Environment/WMInteractionComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Input/WMTouchGestureLibrary.h"
#include "Mission/WMMissionMeasurementComponent.h"
#include "Mission/WMMissionRuntimeSubsystem.h"
#include "UI/WMBuildHUDWidget.h"
#include "UObject/ConstructorHelpers.h"
#include "Visual/WMPrototypeMotionStyle.h"
#include "Visual/WMVisualProfileSettings.h"

AWMPlayerCharacter::AWMPlayerCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
    GetCharacterMovement()->JumpZVelocity = 500.0f;
    GetCharacterMovement()->AirControl = 0.25f;
    GetCharacterMovement()->MaxWalkSpeed = 450.0f;

    PrototypeBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrototypeBody"));
    PrototypeBody->SetupAttachment(RootComponent);
    PrototypeBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PrototypeBody->SetRelativeLocation(FVector(0.0f, 0.0f, 12.0f));
    PrototypeBody->SetRelativeScale3D(FVector(0.34f, 0.30f, 0.62f));

    PrototypeHead = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrototypeHead"));
    PrototypeHead->SetupAttachment(RootComponent);
    PrototypeHead->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PrototypeHead->SetRelativeLocation(FVector(0.0f, 0.0f, 62.0f));
    PrototypeHead->SetRelativeScale3D(FVector(0.40f));

    PrototypeLeftArm = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrototypeLeftArm"));
    PrototypeLeftArm->SetupAttachment(RootComponent);
    PrototypeLeftArm->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PrototypeLeftArm->SetRelativeLocation(FVector(0.0f, -24.0f, 12.0f));
    PrototypeLeftArm->SetRelativeRotation(FRotator(0.0f, 0.0f, -7.0f));
    PrototypeLeftArm->SetRelativeScale3D(FVector(0.11f, 0.11f, 0.46f));

    PrototypeRightArm = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrototypeRightArm"));
    PrototypeRightArm->SetupAttachment(RootComponent);
    PrototypeRightArm->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PrototypeRightArm->SetRelativeLocation(FVector(0.0f, 24.0f, 12.0f));
    PrototypeRightArm->SetRelativeRotation(FRotator(0.0f, 0.0f, 7.0f));
    PrototypeRightArm->SetRelativeScale3D(FVector(0.11f, 0.11f, 0.46f));

    PrototypeLeftLeg = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrototypeLeftLeg"));
    PrototypeLeftLeg->SetupAttachment(RootComponent);
    PrototypeLeftLeg->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PrototypeLeftLeg->SetRelativeLocation(FVector(0.0f, -10.0f, -45.0f));
    PrototypeLeftLeg->SetRelativeScale3D(FVector(0.15f, 0.15f, 0.50f));

    PrototypeRightLeg = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrototypeRightLeg"));
    PrototypeRightLeg->SetupAttachment(RootComponent);
    PrototypeRightLeg->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PrototypeRightLeg->SetRelativeLocation(FVector(0.0f, 10.0f, -45.0f));
    PrototypeRightLeg->SetRelativeScale3D(FVector(0.15f, 0.15f, 0.50f));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> HeadMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (CylinderMesh.Succeeded())
    {
        PrototypeBody->SetStaticMesh(CylinderMesh.Object);
        PrototypeLeftArm->SetStaticMesh(CylinderMesh.Object);
        PrototypeRightArm->SetStaticMesh(CylinderMesh.Object);
        PrototypeLeftLeg->SetStaticMesh(CylinderMesh.Object);
        PrototypeRightLeg->SetStaticMesh(CylinderMesh.Object);
    }
    if (HeadMesh.Succeeded())
    {
        PrototypeHead->SetStaticMesh(HeadMesh.Object);
    }

    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 500.0f;
    CameraBoom->bUsePawnControlRotation = true;

    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;

    BuildingComponent = CreateDefaultSubobject<UWMBuildingComponent>(TEXT("BuildingComponent"));
    MissionMeasurementComponent = CreateDefaultSubobject<UWMMissionMeasurementComponent>(TEXT("MissionMeasurementComponent"));
    ExplorationComponent = CreateDefaultSubobject<UWMExplorationComponent>(TEXT("ExplorationComponent"));
    InteractionComponent = CreateDefaultSubobject<UWMInteractionComponent>(TEXT("InteractionComponent"));
}

void AWMPlayerCharacter::BeginPlay()
{
    Super::BeginPlay();
    ApplyVisualProfileToCamera();
    EnsureBuildHUD();
}

void AWMPlayerCharacter::PawnClientRestart()
{
    Super::PawnClientRestart();
    ApplyVisualProfileToCamera();
    EnsureBuildHUD();
}

void AWMPlayerCharacter::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    UpdatePrototypeMotion(DeltaSeconds);
}

void AWMPlayerCharacter::ApplyVisualProfileToCamera()
{
    const UWMVisualProfileSettings* Profile = GetDefault<UWMVisualProfileSettings>();
    if (Profile && Profile->Atmosphere.IsSane() && FollowCamera)
    {
        FollowCamera->SetFieldOfView(Profile->Atmosphere.CameraFOVDegrees);
    }
}

void AWMPlayerCharacter::UpdatePrototypeMotion(const float DeltaSeconds)
{
    if (!PrototypeBody || !PrototypeHead || !PrototypeLeftArm || !PrototypeRightArm || !PrototypeLeftLeg || !PrototypeRightLeg ||
        !GetCharacterMovement() || !FMath::IsFinite(DeltaSeconds) || DeltaSeconds <= 0.0f)
    {
        return;
    }

    const float MaxWalkSpeed = FMath::Max(GetCharacterMovement()->MaxWalkSpeed, 1.0f);
    const float SpeedAlpha = FMath::Clamp(GetVelocity().Size2D() / MaxWalkSpeed, 0.0f, 1.0f);
    const bool bAirborne = GetCharacterMovement()->IsFalling();

    if (!bAirborne && SpeedAlpha > 0.01f)
    {
        const float CyclesPerSecond = FMath::Lerp(1.35f, 2.25f, SpeedAlpha);
        PrototypeMotionPhase = FMath::Fmod(PrototypeMotionPhase + DeltaSeconds * CyclesPerSecond * 2.0f * PI, 2.0f * PI);
    }

    const FWMPrototypeMotionPose Pose = FWMPrototypeMotionStyle::Evaluate(SpeedAlpha, PrototypeMotionPhase, bAirborne);
    PrototypeBody->SetRelativeLocation(FVector(0.0f, 0.0f, 12.0f + Pose.BodyBobCm));
    PrototypeBody->SetRelativeRotation(FRotator(Pose.BodyLeanDegrees, 0.0f, 0.0f));
    PrototypeHead->SetRelativeLocation(FVector(0.0f, 0.0f, 62.0f + Pose.HeadBobCm));
    PrototypeLeftArm->SetRelativeRotation(FRotator(Pose.ArmSwingDegrees, 0.0f, -7.0f));
    PrototypeRightArm->SetRelativeRotation(FRotator(-Pose.ArmSwingDegrees, 0.0f, 7.0f));
    PrototypeLeftLeg->SetRelativeRotation(FRotator(Pose.LegSwingDegrees, 0.0f, 0.0f));
    PrototypeRightLeg->SetRelativeRotation(FRotator(-Pose.LegSwingDegrees, 0.0f, 0.0f));
}

void AWMPlayerCharacter::EnsureBuildHUD()
{
    if (BuildHUD || !IsLocallyControlled() || !BuildingComponent || !MissionMeasurementComponent || !InteractionComponent)
    {
        return;
    }

    APlayerController* PlayerController = Cast<APlayerController>(GetController());
    if (!PlayerController)
    {
        return;
    }

    BuildHUD = CreateWidget<UWMBuildHUDWidget>(PlayerController, UWMBuildHUDWidget::StaticClass());
    if (!BuildHUD)
    {
        return;
    }

    BuildHUD->BindBuildingComponent(BuildingComponent);
    BuildHUD->BindMissionMeasurementComponent(MissionMeasurementComponent);
    BuildHUD->BindInteractionComponent(InteractionComponent);
    BuildHUD->AddToPlayerScreen(10);

    FInputModeGameAndUI InputMode;
    InputMode.SetHideCursorDuringCapture(false);
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    PlayerController->SetInputMode(InputMode);
}

void AWMPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    check(PlayerInputComponent);

    PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AWMPlayerCharacter::MoveForward);
    PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AWMPlayerCharacter::MoveRight);
    PlayerInputComponent->BindAxis(TEXT("Turn"), this, &AWMPlayerCharacter::Turn);
    PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &AWMPlayerCharacter::LookUp);

    PlayerInputComponent->BindAction(TEXT("PlaceBuild"), IE_Pressed, this, &AWMPlayerCharacter::PlaceBuild);
    PlayerInputComponent->BindAction(TEXT("RotateBuild"), IE_Pressed, this, &AWMPlayerCharacter::RotateBuildClockwise);
    PlayerInputComponent->BindAction(TEXT("RotateBuildBack"), IE_Pressed, this, &AWMPlayerCharacter::RotateBuildCounterClockwise);
    PlayerInputComponent->BindAction(TEXT("RemoveBuild"), IE_Pressed, this, &AWMPlayerCharacter::RemoveBuild);
    PlayerInputComponent->BindAction(TEXT("MoveBuild"), IE_Pressed, this, &AWMPlayerCharacter::MoveBuild);
    PlayerInputComponent->BindAction(TEXT("CycleBuildPiece"), IE_Pressed, this, &AWMPlayerCharacter::CycleBuildPiece);
    PlayerInputComponent->BindAction(TEXT("CancelBuildEdit"), IE_Pressed, this, &AWMPlayerCharacter::CancelBuildEdit);
    PlayerInputComponent->BindAction(TEXT("UndoBuild"), IE_Pressed, this, &AWMPlayerCharacter::UndoBuild);
    PlayerInputComponent->BindAction(TEXT("RedoBuild"), IE_Pressed, this, &AWMPlayerCharacter::RedoBuild);
    PlayerInputComponent->BindAction(TEXT("SaveWorld"), IE_Pressed, this, &AWMPlayerCharacter::SavePrototypeWorld);
    PlayerInputComponent->BindAction(TEXT("LoadWorld"), IE_Pressed, this, &AWMPlayerCharacter::LoadPrototypeWorld);
    PlayerInputComponent->BindAction(TEXT("MeasureMission"), IE_Pressed, this, &AWMPlayerCharacter::CaptureMissionMeasurementPoint);
    PlayerInputComponent->BindAction(TEXT("ResetMissionMeasurement"), IE_Pressed, this, &AWMPlayerCharacter::ResetMissionMeasurement);
    PlayerInputComponent->BindAction(TEXT("CycleMission"), IE_Pressed, this, &AWMPlayerCharacter::CycleMission);
    PlayerInputComponent->BindAction(TEXT("ObserveWorld"), IE_Pressed, this, &AWMPlayerCharacter::ObserveWorld);

    PlayerInputComponent->BindTouch(IE_Pressed, this, &AWMPlayerCharacter::HandleTouchPressed);
    PlayerInputComponent->BindTouch(IE_Repeat, this, &AWMPlayerCharacter::HandleTouchRepeat);
    PlayerInputComponent->BindTouch(IE_Released, this, &AWMPlayerCharacter::HandleTouchReleased);
}

void AWMPlayerCharacter::MoveForward(const float Value)
{
    if (!Controller || FMath::IsNearlyZero(Value)) return;
    const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
    AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), Value);
}

void AWMPlayerCharacter::MoveRight(const float Value)
{
    if (!Controller || FMath::IsNearlyZero(Value)) return;
    const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
    AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), Value);
}

void AWMPlayerCharacter::Turn(const float Value) { AddControllerYawInput(Value); }
void AWMPlayerCharacter::LookUp(const float Value) { AddControllerPitchInput(Value); }
void AWMPlayerCharacter::PlaceBuild() { if (BuildingComponent) BuildingComponent->TryPlaceCurrentPiece(); }
void AWMPlayerCharacter::RotateBuildClockwise() { if (BuildingComponent) BuildingComponent->RotatePreview(1.0f); }
void AWMPlayerCharacter::RotateBuildCounterClockwise() { if (BuildingComponent) BuildingComponent->RotatePreview(-1.0f); }
void AWMPlayerCharacter::RemoveBuild() { if (BuildingComponent) BuildingComponent->TryRemoveTargetPiece(); }
void AWMPlayerCharacter::MoveBuild() { if (BuildingComponent) BuildingComponent->TryBeginMoveTargetPiece(); }
void AWMPlayerCharacter::CycleBuildPiece() { if (BuildingComponent) BuildingComponent->CycleSelectedPiece(1); }
void AWMPlayerCharacter::CancelBuildEdit() { if (BuildingComponent) BuildingComponent->CancelMove(); }
void AWMPlayerCharacter::UndoBuild() { if (BuildingComponent) BuildingComponent->UndoLastAction(); }
void AWMPlayerCharacter::RedoBuild() { if (BuildingComponent) BuildingComponent->RedoLastAction(); }
void AWMPlayerCharacter::SavePrototypeWorld() { if (BuildingComponent) BuildingComponent->SaveWorld(); }
void AWMPlayerCharacter::LoadPrototypeWorld() { if (BuildingComponent) BuildingComponent->LoadWorld(); }
void AWMPlayerCharacter::CaptureMissionMeasurementPoint() { if (MissionMeasurementComponent) MissionMeasurementComponent->CapturePointFromView(); }
void AWMPlayerCharacter::ResetMissionMeasurement() { if (MissionMeasurementComponent) MissionMeasurementComponent->ResetMeasurement(); }
void AWMPlayerCharacter::ObserveWorld() { if (InteractionComponent) InteractionComponent->TryInteractFocused(); }

void AWMPlayerCharacter::CycleMission()
{
    if (MissionMeasurementComponent)
    {
        MissionMeasurementComponent->ResetMeasurement();
    }
    if (UWorld* World = GetWorld())
    {
        if (UWMMissionRuntimeSubsystem* Missions = World->GetSubsystem<UWMMissionRuntimeSubsystem>())
        {
            Missions->CycleMission(1);
        }
    }
}

void AWMPlayerCharacter::HandleTouchPressed(const ETouchIndex::Type FingerIndex, const FVector Location)
{
    if (bTouchTracking) return;
    bTouchTracking = true;
    bTouchDragging = false;
    ActiveTouchFinger = FingerIndex;
    TouchStartScreen = FVector2D(Location.X, Location.Y);
    TouchLastScreen = TouchStartScreen;
}

void AWMPlayerCharacter::HandleTouchRepeat(const ETouchIndex::Type FingerIndex, const FVector Location)
{
    if (!bTouchTracking || FingerIndex != ActiveTouchFinger) return;

    const FVector2D CurrentScreen(Location.X, Location.Y);
    if (!bTouchDragging && UWMTouchGestureLibrary::HasExceededDragDeadZone(TouchStartScreen, CurrentScreen, TouchDragDeadZonePx))
    {
        bTouchDragging = true;
    }

    if (bTouchDragging)
    {
        FVector2D Delta = CurrentScreen - TouchLastScreen;
        constexpr float MaxTouchDeltaPx = 96.0f;
        if (Delta.SizeSquared() > FMath::Square(MaxTouchDeltaPx))
        {
            Delta = Delta.GetSafeNormal() * MaxTouchDeltaPx;
        }
        AddControllerYawInput(Delta.X * TouchLookDegreesPerPixel);
        AddControllerPitchInput(-Delta.Y * TouchLookDegreesPerPixel);
    }
    TouchLastScreen = CurrentScreen;
}

void AWMPlayerCharacter::HandleTouchReleased(const ETouchIndex::Type FingerIndex, const FVector Location)
{
    if (!bTouchTracking || FingerIndex != ActiveTouchFinger) return;
    bTouchTracking = false;
    bTouchDragging = false;
    TouchStartScreen = FVector2D::ZeroVector;
    TouchLastScreen = FVector2D::ZeroVector;
}
