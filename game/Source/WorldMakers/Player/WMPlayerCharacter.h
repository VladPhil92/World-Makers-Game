#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "WMPlayerCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UStaticMeshComponent;
class UWMBuildingComponent;
class UWMBuildHUDWidget;
class UWMMissionMeasurementComponent;
class UWMExplorationComponent;

UCLASS()
class WORLDMAKERS_API AWMPlayerCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AWMPlayerCharacter();

    virtual void BeginPlay() override;
    virtual void PawnClientRestart() override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Avatar")
    TObjectPtr<UStaticMeshComponent> PrototypeBody;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Avatar")
    TObjectPtr<UStaticMeshComponent> PrototypeHead;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Avatar")
    TObjectPtr<UStaticMeshComponent> PrototypeLeftArm;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Avatar")
    TObjectPtr<UStaticMeshComponent> PrototypeRightArm;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Avatar")
    TObjectPtr<UStaticMeshComponent> PrototypeLeftLeg;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Avatar")
    TObjectPtr<UStaticMeshComponent> PrototypeRightLeg;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Camera")
    TObjectPtr<USpringArmComponent> CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Camera")
    TObjectPtr<UCameraComponent> FollowCamera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Building")
    TObjectPtr<UWMBuildingComponent> BuildingComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Missions")
    TObjectPtr<UWMMissionMeasurementComponent> MissionMeasurementComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Exploration")
    TObjectPtr<UWMExplorationComponent> ExplorationComponent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Makers|Input", meta = (ClampMin = "4.0", ClampMax = "128.0"))
    float TouchDragDeadZonePx = 24.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Makers|Input", meta = (ClampMin = "0.01", ClampMax = "1.0"))
    float TouchLookDegreesPerPixel = 0.08f;

private:
    void MoveForward(float Value);
    void MoveRight(float Value);
    void Turn(float Value);
    void LookUp(float Value);

    void PlaceBuild();
    void RotateBuildClockwise();
    void RotateBuildCounterClockwise();
    void RemoveBuild();
    void MoveBuild();
    void CycleBuildPiece();
    void CancelBuildEdit();
    void UndoBuild();
    void RedoBuild();
    void SavePrototypeWorld();
    void LoadPrototypeWorld();
    void CaptureMissionMeasurementPoint();
    void ResetMissionMeasurement();
    void CycleMission();
    void EnsureBuildHUD();

    void HandleTouchPressed(ETouchIndex::Type FingerIndex, FVector Location);
    void HandleTouchRepeat(ETouchIndex::Type FingerIndex, FVector Location);
    void HandleTouchReleased(ETouchIndex::Type FingerIndex, FVector Location);

    UPROPERTY(Transient)
    TObjectPtr<UWMBuildHUDWidget> BuildHUD;

    bool bTouchTracking = false;
    bool bTouchDragging = false;
    ETouchIndex::Type ActiveTouchFinger = ETouchIndex::Touch1;
    FVector2D TouchStartScreen = FVector2D::ZeroVector;
    FVector2D TouchLastScreen = FVector2D::ZeroVector;
};