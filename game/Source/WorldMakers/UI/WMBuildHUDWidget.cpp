#include "UI/WMBuildHUDWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Building/WMBuildCatalogSettings.h"
#include "Building/WMBuildingComponent.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

#define LOCTEXT_NAMESPACE "WorldMakersBuildHUD"

void UWMBuildHUDWidget::BindBuildingComponent(UWMBuildingComponent* InBuildingComponent)
{
    BuildingComponent = InBuildingComponent;
    RefreshStatus();
}

void UWMBuildHUDWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    if (!WidgetTree)
    {
        return;
    }

    UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("BuildHUDRoot"));
    WidgetTree->RootWidget = RootCanvas;

    UVerticalBox* ActionStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("BuildActionStack"));
    UCanvasPanelSlot* StackSlot = RootCanvas->AddChildToCanvas(ActionStack);
    StackSlot->SetAnchors(FAnchors(0.5f, 1.0f));
    StackSlot->SetAlignment(FVector2D(0.5f, 1.0f));
    StackSlot->SetPosition(FVector2D(0.0f, -24.0f));
    StackSlot->SetAutoSize(true);
    StackSlot->SetZOrder(10);

    StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BuildStatus"));
    StatusText->SetJustification(ETextJustify::Center);
    StatusText->SetAutoWrapText(true);
    if (UVerticalBoxSlot* StatusSlot = ActionStack->AddChildToVerticalBox(StatusText))
    {
        StatusSlot->SetHorizontalAlignment(HAlign_Fill);
        StatusSlot->SetPadding(FMargin(8.0f, 4.0f, 8.0f, 8.0f));
    }

    UHorizontalBox* PrimaryRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("BuildPrimaryRow"));
    ActionStack->AddChildToVerticalBox(PrimaryRow);

    UButton* PreviousButton = CreateActionButton(PrimaryRow, TEXT("PreviousPieceButton"), LOCTEXT("PreviousPiece", "Previous"));
    UButton* NextButton = CreateActionButton(PrimaryRow, TEXT("NextPieceButton"), LOCTEXT("NextPiece", "Next"));
    UButton* RotateLeftButton = CreateActionButton(PrimaryRow, TEXT("RotateLeftButton"), LOCTEXT("RotateLeft", "Rotate left"));
    UButton* RotateRightButton = CreateActionButton(PrimaryRow, TEXT("RotateRightButton"), LOCTEXT("RotateRight", "Rotate right"));
    ConfirmButton = CreateActionButton(PrimaryRow, TEXT("ConfirmBuildButton"), LOCTEXT("Build", "Build"));
    ConfirmText = Cast<UTextBlock>(ConfirmButton ? ConfirmButton->GetChildAt(0) : nullptr);

    UHorizontalBox* EditRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("BuildEditRow"));
    if (UVerticalBoxSlot* EditRowSlot = ActionStack->AddChildToVerticalBox(EditRow))
    {
        EditRowSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));
    }

    UButton* MoveButton = CreateActionButton(EditRow, TEXT("MoveButton"), LOCTEXT("Move", "Move"));
    UButton* RemoveButton = CreateActionButton(EditRow, TEXT("RemoveButton"), LOCTEXT("Remove", "Remove"));
    UButton* UndoButton = CreateActionButton(EditRow, TEXT("UndoButton"), LOCTEXT("Undo", "Undo"));
    UButton* RedoButton = CreateActionButton(EditRow, TEXT("RedoButton"), LOCTEXT("Redo", "Redo"));
    CancelButton = CreateActionButton(EditRow, TEXT("CancelButton"), LOCTEXT("Cancel", "Cancel"));

    PreviousButton->OnClicked.AddDynamic(this, &UWMBuildHUDWidget::HandlePreviousPiece);
    NextButton->OnClicked.AddDynamic(this, &UWMBuildHUDWidget::HandleNextPiece);
    RotateLeftButton->OnClicked.AddDynamic(this, &UWMBuildHUDWidget::HandleRotateLeft);
    RotateRightButton->OnClicked.AddDynamic(this, &UWMBuildHUDWidget::HandleRotateRight);
    ConfirmButton->OnClicked.AddDynamic(this, &UWMBuildHUDWidget::HandleConfirm);
    MoveButton->OnClicked.AddDynamic(this, &UWMBuildHUDWidget::HandleMove);
    RemoveButton->OnClicked.AddDynamic(this, &UWMBuildHUDWidget::HandleRemove);
    UndoButton->OnClicked.AddDynamic(this, &UWMBuildHUDWidget::HandleUndo);
    RedoButton->OnClicked.AddDynamic(this, &UWMBuildHUDWidget::HandleRedo);
    CancelButton->OnClicked.AddDynamic(this, &UWMBuildHUDWidget::HandleCancel);

    RefreshStatus();
}

void UWMBuildHUDWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    RefreshStatus();
}

UButton* UWMBuildHUDWidget::CreateActionButton(UHorizontalBox* Row, const FName WidgetName, const FText& Label)
{
    if (!WidgetTree || !Row)
    {
        return nullptr;
    }

    USizeBox* TouchTarget = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), FName(*(WidgetName.ToString() + TEXT("Target"))));
    TouchTarget->SetWidthOverride(TouchTargetWidth);
    TouchTarget->SetHeightOverride(TouchTargetHeight);

    UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), WidgetName);
    Button->IsFocusable = false;

    UTextBlock* LabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(*(WidgetName.ToString() + TEXT("Label"))));
    LabelText->SetText(Label);
    LabelText->SetJustification(ETextJustify::Center);

    Button->AddChild(LabelText);
    TouchTarget->AddChild(Button);

    if (UHorizontalBoxSlot* Slot = Row->AddChildToHorizontalBox(TouchTarget))
    {
        Slot->SetPadding(FMargin(4.0f));
        Slot->SetHorizontalAlignment(HAlign_Center);
        Slot->SetVerticalAlignment(VAlign_Center);
    }

    return Button;
}

FText UWMBuildHUDWidget::ResolveSelectedPieceLabel() const
{
    if (!BuildingComponent.IsValid())
    {
        return LOCTEXT("PieceUnavailable", "Building piece");
    }

    const FName PieceId = BuildingComponent->GetSelectedPieceId();
    if (PieceId == TEXT("prototype.cube"))
    {
        return LOCTEXT("PieceCube", "Cube");
    }
    if (PieceId == TEXT("prototype.floor"))
    {
        return LOCTEXT("PieceFloor", "Floor");
    }
    if (PieceId == TEXT("prototype.wall"))
    {
        return LOCTEXT("PieceWall", "Wall");
    }
    if (PieceId == TEXT("prototype.pillar"))
    {
        return LOCTEXT("PiecePillar", "Pillar");
    }

    return LOCTEXT("PieceGeneric", "Building piece");
}

void UWMBuildHUDWidget::RefreshStatus()
{
    if (!StatusText || !ConfirmButton || !CancelButton)
    {
        return;
    }

    if (!BuildingComponent.IsValid())
    {
        StatusText->SetText(LOCTEXT("BuildUnavailable", "Building tools are unavailable."));
        ConfirmButton->SetIsEnabled(false);
        CancelButton->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    const bool bMoveInProgress = BuildingComponent->IsMoveInProgress();
    const bool bPlacementValid = BuildingComponent->IsPreviewPlacementValid();
    const FText StateText = bMoveInProgress
        ? (bPlacementValid ? LOCTEXT("MoveReady", "Ready to move") : LOCTEXT("MoveBlocked", "Choose an open place"))
        : (bPlacementValid ? LOCTEXT("BuildReady", "Ready to build") : LOCTEXT("BuildBlocked", "Find an open place"));

    StatusText->SetText(FText::Format(LOCTEXT("BuildStatusFormat", "{0} — {1}"), ResolveSelectedPieceLabel(), StateText));
    ConfirmButton->SetIsEnabled(bPlacementValid);
    CancelButton->SetVisibility(bMoveInProgress ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

    if (ConfirmText)
    {
        ConfirmText->SetText(bMoveInProgress ? LOCTEXT("ConfirmMove", "Move here") : LOCTEXT("ConfirmBuild", "Build"));
    }
}

void UWMBuildHUDWidget::HandlePreviousPiece()
{
    if (BuildingComponent.IsValid()) BuildingComponent->CycleSelectedPiece(-1);
}

void UWMBuildHUDWidget::HandleNextPiece()
{
    if (BuildingComponent.IsValid()) BuildingComponent->CycleSelectedPiece(1);
}

void UWMBuildHUDWidget::HandleRotateLeft()
{
    if (BuildingComponent.IsValid()) BuildingComponent->RotatePreview(-1.0f);
}

void UWMBuildHUDWidget::HandleRotateRight()
{
    if (BuildingComponent.IsValid()) BuildingComponent->RotatePreview(1.0f);
}

void UWMBuildHUDWidget::HandleConfirm()
{
    if (BuildingComponent.IsValid()) BuildingComponent->TryPlaceCurrentPiece();
}

void UWMBuildHUDWidget::HandleMove()
{
    if (BuildingComponent.IsValid()) BuildingComponent->TryBeginMoveTargetPiece();
}

void UWMBuildHUDWidget::HandleRemove()
{
    if (BuildingComponent.IsValid()) BuildingComponent->TryRemoveTargetPiece();
}

void UWMBuildHUDWidget::HandleUndo()
{
    if (BuildingComponent.IsValid()) BuildingComponent->UndoLastAction();
}

void UWMBuildHUDWidget::HandleRedo()
{
    if (BuildingComponent.IsValid()) BuildingComponent->RedoLastAction();
}

void UWMBuildHUDWidget::HandleCancel()
{
    if (BuildingComponent.IsValid()) BuildingComponent->CancelMove();
}

#undef LOCTEXT_NAMESPACE
