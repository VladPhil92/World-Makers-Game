#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "WMPlayerCharacter.generated.h"

class UCameraComponent;
class UProceduralMeshComponent;
class USceneComponent;
class USpringArmComponent;
class UStaticMeshComponent;
class UWMBuildingComponent;
class UWMBuildHUDWidget;
class UWMCharacterAnimationComponent;
class UWMFirstPersonInteractionComponent;
class UWMMissionMeasurementComponent;
class UWMExplorationComponent;
class UWMInteractionComponent;

UCLASS()
class WORLDMAKERS_API AWMPlayerCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AWMPlayerCharacter();

    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;
    virtual void PawnClientRestart() override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    virtual void Tick(float DeltaSeconds) override;

    /** Legacy six-piece visual fallback retained until a production Skeletal Mesh is authored and certified. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Avatar|Legacy")
    TObjectPtr<UStaticMeshComponent> PrototypeBody;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Avatar|Legacy")
    TObjectPtr<UStaticMeshComponent> PrototypeHead;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Avatar|Legacy")
    TObjectPtr<UStaticMeshComponent> PrototypeLeftArm;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Avatar|Legacy")
    TObjectPtr<UStaticMeshComponent> PrototypeRightArm;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Avatar|Legacy")
    TObjectPtr<UStaticMeshComponent> PrototypeLeftLeg;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Avatar|Legacy")
    TObjectPtr<UStaticMeshComponent> PrototypeRightLeg;

    /** V4 source-visible hierarchy. Bone IDs mirror the authored skeletal handoff contract. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Avatar|Rig")
    TObjectPtr<USceneComponent> AvatarRigRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Avatar|Rig")
    TObjectPtr<USceneComponent> AvatarPelvisRig;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Avatar|Rig")
    TObjectPtr<UProceduralMeshComponent> AvatarSpineRig;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Avatar|Rig")
    TObjectPtr<USceneComponent> AvatarChestRig;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Avatar|Rig")
    TObjectPtr<USceneComponent> AvatarNeckRig;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Avatar|Rig")
    TObjectPtr<UProceduralMeshComponent> AvatarHeadRig;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Avatar|Rig")
    TObjectPtr<USceneComponent> AvatarJawRig;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Avatar|Rig")
    TObjectPtr<UProceduralMeshComponent> AvatarUpperArmLeftRig;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Avatar|Rig")
    TObjectPtr<UProceduralMeshComponent> AvatarLowerArmLeftRig;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Avatar|Rig")
    TObjectPtr<UProceduralMeshComponent> AvatarHandLeftRig;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Avatar|Rig")
    TObjectPtr<UProceduralMeshComponent> AvatarUpperArmRightRig;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Avatar|Rig")
    TObjectPtr<UProceduralMeshComponent> AvatarLowerArmRightRig;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Avatar|Rig")
    TObjectPtr<UProceduralMeshComponent> AvatarHandRightRig;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Avatar|Rig")
    TObjectPtr<UProceduralMeshComponent> AvatarThighLeftRig;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Avatar|Rig")
    TObjectPtr<UProceduralMeshComponent> AvatarCalfLeftRig;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Avatar|Rig")
    TObjectPtr<UProceduralMeshComponent> AvatarFootLeftRig;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Avatar|Rig")
    TObjectPtr<UProceduralMeshComponent> AvatarThighRightRig;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Avatar|Rig")
    TObjectPtr<UProceduralMeshComponent> AvatarCalfRightRig;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Avatar|Rig")
    TObjectPtr<UProceduralMeshComponent> AvatarFootRightRig;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Avatar|Art")
    TObjectPtr<UProceduralMeshComponent> AvatarHairArt;

    UFUNCTION(BlueprintCallable, Category = "World Makers|Avatar")
    void RefreshAvatarVisualPath();

    UFUNCTION(BlueprintPure, Category = "World Makers|Avatar")
    bool IsProceduralAvatarActive() const { return bProceduralAvatarActive; }

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

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Interaction")
    TObjectPtr<UWMInteractionComponent> InteractionComponent;

    /** V5 animation state/read-model bridge for both procedural art and the future production AnimBP. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Animation")
    TObjectPtr<UWMCharacterAnimationComponent> CharacterAnimationComponent;

    /** First-person presentation kit. Gameplay authority remains with building/mission/interaction components. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|First Person")
    TObjectPtr<UWMFirstPersonInteractionComponent> FirstPersonInteractionComponent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Makers|Input", meta = (ClampMin = "4.0", ClampMax = "128.0"))
    float TouchDragDeadZonePx = 24.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Makers|Input", meta = (ClampMin = "0.01", ClampMax = "1.0"))
    float TouchLookDegreesPerPixel = 0.08f;

private:
    void MoveForward(float Value);
    void MoveRight(float Value);
    void Turn(float Value);
    void LookUp(float Value);
    void StartJump();
    void EndJump();

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
    void ObserveWorld();
    void EnsureBuildHUD();
    void ApplyVisualProfileToCamera();
    void BuildProceduralAvatarArt();
    void UpdatePrototypeMotion(float DeltaSeconds);
    void SetLegacyAvatarVisible(bool bVisible);
    void SetProceduralAvatarVisible(bool bVisible);

    void HandleTouchPressed(ETouchIndex::Type FingerIndex, FVector Location);
    void HandleTouchRepeat(ETouchIndex::Type FingerIndex, FVector Location);
    void HandleTouchReleased(ETouchIndex::Type FingerIndex, FVector Location);

    UPROPERTY(Transient)
    TObjectPtr<UWMBuildHUDWidget> BuildHUD;

    float PrototypeMotionPhase = 0.0f;
    bool bProceduralAvatarReady = false;
    bool bProceduralAvatarActive = false;
    bool bTouchTracking = false;
    bool bTouchDragging = false;
    ETouchIndex::Type ActiveTouchFinger = ETouchIndex::Touch1;
    FVector2D TouchStartScreen = FVector2D::ZeroVector;
    FVector2D TouchLastScreen = FVector2D::ZeroVector;
};
