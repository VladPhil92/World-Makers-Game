#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WMBuildHUDWidget.generated.h"

class UButton;
class UHorizontalBox;
class UTextBlock;
class UWMBuildingComponent;

UCLASS()
class WORLDMAKERS_API UWMBuildHUDWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void BindBuildingComponent(UWMBuildingComponent* InBuildingComponent);

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    UButton* CreateActionButton(UHorizontalBox* Row, FName WidgetName, const FText& Label);
    FText ResolveSelectedPieceLabel() const;
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

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> StatusText;

    UPROPERTY(Transient)
    TObjectPtr<UButton> ConfirmButton;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> ConfirmText;

    UPROPERTY(Transient)
    TObjectPtr<UButton> CancelButton;

    static constexpr float TouchTargetWidth = 112.0f;
    static constexpr float TouchTargetHeight = 72.0f;
};
