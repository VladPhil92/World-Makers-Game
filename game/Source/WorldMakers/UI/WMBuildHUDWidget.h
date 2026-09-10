#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TimerManager.h"
#include "WMBuildHUDWidget.generated.h"

class UButton;
class UHorizontalBox;
class UTextBlock;
class UWMBuildingComponent;
class UWMMissionMeasurementComponent;
class UWMInteractionComponent;

UCLASS()
class WORLDMAKERS_API UWMBuildHUDWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void BindBuildingComponent(UWMBuildingComponent* InBuildingComponent);
    void BindMissionMeasurementComponent(UWMMissionMeasurementComponent* InMissionMeasurementComponent);
    void BindInteractionComponent(UWMInteractionComponent* InInteractionComponent);

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeDestruct() override;

private:
    UButton* CreateActionButton(UHorizontalBox* Row, FName WidgetName, const FText& Label);
    FText ResolveSelectedPieceLabel() const;
    FText ResolveInteractionPromptLabel() const;
    void RefreshStatus();

    UFUNCTION()
    void HandlePreviousPiece();

    UFUNCTION()
    void HandleNextPiece();

    UFUNCTION()
    void HandleRotateLeft();

    UFUNCTION()
    void HandleRotateRight();

    UFUNCTION()
    void HandleConfirm();

    UFUNCTION()
    void HandleMeasure();

    UFUNCTION()
    void HandleResetMeasurement();

    UFUNCTION()
    void HandleNextMission();

    UFUNCTION()
    void HandleObserve();

    UFUNCTION()
    void HandleMove();

    UFUNCTION()
    void HandleRemove();

    UFUNCTION()
    void HandleUndo();

    UFUNCTION()
    void HandleRedo();

    UFUNCTION()
    void HandleCancel();

    TWeakObjectPtr<UWMBuildingComponent> BuildingComponent;
    TWeakObjectPtr<UWMMissionMeasurementComponent> MissionMeasurementComponent;
    TWeakObjectPtr<UWMInteractionComponent> InteractionComponent;
    FTimerHandle StatusRefreshTimer;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> InteractionStatusText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> MissionStatusText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> StatusText;

    UPROPERTY(Transient)
    TObjectPtr<UButton> ObserveButton;

    UPROPERTY(Transient)
    TObjectPtr<UButton> ConfirmButton;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> ConfirmText;

    UPROPERTY(Transient)
    TObjectPtr<UButton> CancelButton;

    static constexpr float TouchTargetWidth = 112.0f;
    static constexpr float TouchTargetHeight = 72.0f;
    static constexpr float StatusRefreshSeconds = 0.10f;
};
