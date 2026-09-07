#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "WMPlayerCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UWMBuildingComponent;

UCLASS()
class WORLDMAKERS_API AWMPlayerCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AWMPlayerCharacter();

    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Camera")
    TObjectPtr<USpringArmComponent> CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Camera")
    TObjectPtr<UCameraComponent> FollowCamera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Building")
    TObjectPtr<UWMBuildingComponent> BuildingComponent;

private:
    void MoveForward(float Value);
    void MoveRight(float Value);
    void Turn(float Value);
    void LookUp(float Value);

    void PlaceBuild();
    void RotateBuildClockwise();
    void RotateBuildCounterClockwise();
    void RemoveBuild();
    void UndoBuild();
    void RedoBuild();
    void SavePrototypeWorld();
    void LoadPrototypeWorld();
    void HandleTouchPressed(ETouchIndex::Type FingerIndex, FVector Location);
};
