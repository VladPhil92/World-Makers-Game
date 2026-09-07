#include "Player/WMPlayerCharacter.h"

#include "Building/WMBuildingComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "UObject/ConstructorHelpers.h"

AWMPlayerCharacter::AWMPlayerCharacter()
{
    PrimaryActorTick.bCanEverTick = false;

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
    PrototypeBody->SetRelativeScale3D(FVector(0.45f, 0.45f, 0.85f));

    PrototypeHead = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrototypeHead"));
    PrototypeHead->SetupAttachment(RootComponent);
    PrototypeHead->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PrototypeHead->SetRelativeLocation(FVector(0.0f, 0.0f, 82.0f));
    PrototypeHead->SetRelativeScale3D(FVector(0.38f));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> BodyMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> HeadMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (BodyMesh.Succeeded())
    {
        PrototypeBody->SetStaticMesh(BodyMesh.Object);
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
    PlayerInputComponent->BindAction(TEXT("UndoBuild"), IE_Pressed, this, &AWMPlayerCharacter::UndoBuild);
    PlayerInputComponent->BindAction(TEXT("RedoBuild"), IE_Pressed, this, &AWMPlayerCharacter::RedoBuild);
    PlayerInputComponent->BindAction(TEXT("SaveWorld"), IE_Pressed, this, &AWMPlayerCharacter::SavePrototypeWorld);
    PlayerInputComponent->BindAction(TEXT("LoadWorld"), IE_Pressed, this, &AWMPlayerCharacter::LoadPrototypeWorld);

    PlayerInputComponent->BindTouch(IE_Pressed, this, &AWMPlayerCharacter::HandleTouchPressed);
}

void AWMPlayerCharacter::MoveForward(const float Value)
{
    if (!Controller || FMath::IsNearlyZero(Value))
    {
        return;
    }

    const FRotator ControlRotation = Controller->GetControlRotation();
    const FRotator YawRotation(0.0f, ControlRotation.Yaw, 0.0f);
    AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), Value);
}

void AWMPlayerCharacter::MoveRight(const float Value)
{
    if (!Controller || FMath::IsNearlyZero(Value))
    {
        return;
    }

    const FRotator ControlRotation = Controller->GetControlRotation();
    const FRotator YawRotation(0.0f, ControlRotation.Yaw, 0.0f);
    AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), Value);
}

void AWMPlayerCharacter::Turn(const float Value)
{
    AddControllerYawInput(Value);
}

void AWMPlayerCharacter::LookUp(const float Value)
{
    AddControllerPitchInput(Value);
}

void AWMPlayerCharacter::PlaceBuild()
{
    if (BuildingComponent)
    {
        BuildingComponent->TryPlaceCurrentPiece();
    }
}

void AWMPlayerCharacter::RotateBuildClockwise()
{
    if (BuildingComponent)
    {
        BuildingComponent->RotatePreview(1.0f);
    }
}

void AWMPlayerCharacter::RotateBuildCounterClockwise()
{
    if (BuildingComponent)
    {
        BuildingComponent->RotatePreview(-1.0f);
    }
}

void AWMPlayerCharacter::RemoveBuild()
{
    if (BuildingComponent)
    {
        BuildingComponent->TryRemoveTargetPiece();
    }
}

void AWMPlayerCharacter::UndoBuild()
{
    if (BuildingComponent)
    {
        BuildingComponent->UndoLastAction();
    }
}

void AWMPlayerCharacter::RedoBuild()
{
    if (BuildingComponent)
    {
        BuildingComponent->RedoLastAction();
    }
}

void AWMPlayerCharacter::SavePrototypeWorld()
{
    if (BuildingComponent)
    {
        BuildingComponent->SaveWorld();
    }
}

void AWMPlayerCharacter::LoadPrototypeWorld()
{
    if (BuildingComponent)
    {
        BuildingComponent->LoadWorld();
    }
}

void AWMPlayerCharacter::HandleTouchPressed(ETouchIndex::Type FingerIndex, FVector Location)
{
    PlaceBuild();
}
