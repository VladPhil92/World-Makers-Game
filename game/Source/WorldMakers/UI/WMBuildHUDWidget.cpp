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
#include "Engine/World.h"
#include "Environment/WMInteractionComponent.h"
#include "Mission/WMMissionMeasurementComponent.h"
#include "Mission/WMMissionRuntimeSubsystem.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "WorldMakersBuildHUD"

void UWMBuildHUDWidget::BindBuildingComponent(UWMBuildingComponent* InBuildingComponent)
{
    BuildingComponent = InBuildingComponent;
    RefreshStatus();
}

void UWMBuildHUDWidget::BindMissionMeasurementComponent(UWMMissionMeasurementComponent* InMissionMeasurementComponent)
{
    MissionMeasurementComponent = InMissionMeasurementComponent;
    RefreshStatus();
}

void UWMBuildHUDWidget::BindInteractionComponent(UWMInteractionComponent* InInteractionComponent)
{
    InteractionComponent = InInteractionComponent;
    RefreshStatus();
}

void UWMBuildHUDWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (!WidgetTree) return;

    UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("BuildHUDRoot"));
    WidgetTree->RootWidget = RootCanvas;

    UVerticalBox* ActionStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("BuildActionStack"));
    UCanvasPanelSlot* StackSlot = RootCanvas->AddChildToCanvas(ActionStack);
    StackSlot->SetAnchors(FAnchors(0.5f, 1.0f));
    StackSlot->SetAlignment(FVector2D(0.5f, 1.0f));
    StackSlot->SetPosition(FVector2D(0.0f, -24.0f));
    StackSlot->SetAutoSize(true);
    StackSlot->SetZOrder(10);

    InteractionStatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("InteractionStatus"));
    InteractionStatusText->SetJustification(ETextJustify::Center);
    InteractionStatusText->SetAutoWrapText(true);
    if (UVerticalBoxSlot* InteractionStatusSlot = ActionStack->AddChildToVerticalBox(InteractionStatusText))
    {
        InteractionStatusSlot->SetHorizontalAlignment(HAlign_Fill);
        InteractionStatusSlot->SetPadding(FMargin(8.0f, 4.0f));
    }

    UHorizontalBox* InteractionRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("InteractionActionRow"));
    if (UVerticalBoxSlot* InteractionRowSlot = ActionStack->AddChildToVerticalBox(InteractionRow))
    {
        InteractionRowSlot->SetHorizontalAlignment(HAlign_Center);
    }
    ObserveButton = CreateActionButton(InteractionRow, TEXT("ObserveWorldButton"), LOCTEXT("ObserveWorld", "Observe"));

    MissionStatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MissionStatus"));
    MissionStatusText->SetJustification(ETextJustify::Center);
    MissionStatusText->SetAutoWrapText(true);
    if (UVerticalBoxSlot* MissionStatusSlot = ActionStack->AddChildToVerticalBox(MissionStatusText))
    {
        MissionStatusSlot->SetHorizontalAlignment(HAlign_Fill);
        MissionStatusSlot->SetPadding(FMargin(8.0f, 4.0f, 8.0f, 4.0f));
    }

    StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BuildStatus"));
    StatusText->SetJustification(ETextJustify::Center);
    StatusText->SetAutoWrapText(true);
    if (UVerticalBoxSlot* StatusSlot = ActionStack->AddChildToVerticalBox(StatusText))
    {
        StatusSlot->SetHorizontalAlignment(HAlign_Fill);
        StatusSlot->SetPadding(FMargin(8.0f, 4.0f, 8.0f, 8.0f));
    }

    UHorizontalBox* MissionRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("MissionActionRow"));
    ActionStack->AddChildToVerticalBox(MissionRow);
    UButton* MeasureButton = CreateActionButton(MissionRow, TEXT("MeasureTargetButton"), LOCTEXT("MeasurePoint", "Measure"));
    UButton* ResetMeasurementButton = CreateActionButton(MissionRow, TEXT("ResetMeasurementButton"), LOCTEXT("ResetMeasurement", "Reset measure"));
    UButton* NextMissionButton = CreateActionButton(MissionRow, TEXT("NextMissionButton"), LOCTEXT("NextMission", "Next mission"));

    UHorizontalBox* PrimaryRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("BuildPrimaryRow"));
    ActionStack->AddChildToVerticalBox(PrimaryRow);
    UButton* PreviousButton = CreateActionButton(PrimaryRow, TEXT("PreviousPieceButton"), LOCTEXT("PreviousPiece", "Previous"));
    UButton* NextButton = CreateActionButton(PrimaryRow, TEXT("NextPieceButton"), LOCTEXT("NextPiece", "Next"));
    UButton* RotateLeftButton = CreateActionButton(PrimaryRow, TEXT("RotateLeftButton"), LOCTEXT("RotateLeft", "Rotate left"));
    UButton* RotateRightButton = CreateActionButton(PrimaryRow, TEXT("RotateRightButton"), LOCTEXT("RotateRight", "Rotate right"));
    ConfirmButton = CreateActionButton(PrimaryRow, TEXT("ConfirmBuildButton"), LOCTEXT("Build", "Build"));
    ConfirmText = Cast<UTextBlock>(ConfirmButton ? ConfirmButton->GetChildAt(0) : nullptr);

    UHorizontalBox* EditRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("BuildEditRow"));
    if (UVerticalBoxSlot* EditRowSlot = ActionStack->AddChildToVerticalBox(EditRow)) EditRowSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));
    UButton* MoveButton = CreateActionButton(EditRow, TEXT("MoveButton"), LOCTEXT("Move", "Move"));
    UButton* RemoveButton = CreateActionButton(EditRow, TEXT("RemoveButton"), LOCTEXT("Remove", "Remove"));
    UButton* UndoButton = CreateActionButton(EditRow, TEXT("UndoButton"), LOCTEXT("Undo", "Undo"));
    UButton* RedoButton = CreateActionButton(EditRow, TEXT("RedoButton"), LOCTEXT("Redo", "Redo"));
    CancelButton = CreateActionButton(EditRow, TEXT("CancelButton"), LOCTEXT("Cancel", "Cancel"));

    if (ObserveButton) ObserveButton->OnClicked.AddDynamic(this, &UWMBuildHUDWidget::HandleObserve);
    MeasureButton->OnClicked.AddDynamic(this, &UWMBuildHUDWidget::HandleMeasure);
    ResetMeasurementButton->OnClicked.AddDynamic(this, &UWMBuildHUDWidget::HandleResetMeasurement);
    NextMissionButton->OnClicked.AddDynamic(this, &UWMBuildHUDWidget::HandleNextMission);
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
    if (UWorld* World = GetWorld()) World->GetTimerManager().SetTimer(StatusRefreshTimer, this, &UWMBuildHUDWidget::RefreshStatus, StatusRefreshSeconds, true);
}

void UWMBuildHUDWidget::NativeDestruct()
{
    if (UWorld* World = GetWorld()) World->GetTimerManager().ClearTimer(StatusRefreshTimer);
    Super::NativeDestruct();
}

UButton* UWMBuildHUDWidget::CreateActionButton(UHorizontalBox* Row, const FName WidgetName, const FText& Label)
{
    if (!WidgetTree || !Row) return nullptr;
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
    if (!BuildingComponent.IsValid()) return LOCTEXT("PieceUnavailable", "Building piece");
    const FName PieceId = BuildingComponent->GetSelectedPieceId();
    if (PieceId == TEXT("prototype.cube")) return LOCTEXT("PieceCube", "Cube");
    if (PieceId == TEXT("prototype.floor")) return LOCTEXT("PieceFloor", "Floor");
    if (PieceId == TEXT("prototype.wall")) return LOCTEXT("PieceWall", "Wall");
    if (PieceId == TEXT("prototype.pillar")) return LOCTEXT("PiecePillar", "Pillar");
    return LOCTEXT("PieceGeneric", "Building piece");
}

FText UWMBuildHUDWidget::ResolveInteractionPromptLabel() const
{
    if (!InteractionComponent.IsValid()) return LOCTEXT("InteractionUnavailable", "Observe");
    const FName PromptKey = InteractionComponent->FocusedPromptKey;
    if (PromptKey == TEXT("interaction.rainforest.ceiba.observe")) return LOCTEXT("ObserveCeiba", "Observe the ceiba");
    if (PromptKey == TEXT("interaction.rainforest.bromeliad-cluster.observe")) return LOCTEXT("ObserveBromeliads", "Observe the bromeliads");
    if (PromptKey == TEXT("interaction.rainforest.water-edge.observe")) return LOCTEXT("ObserveWater", "Observe the water");
    return LOCTEXT("ObserveFocused", "Observe what you found");
}

void UWMBuildHUDWidget::RefreshStatus()
{
    if (!StatusText || !MissionStatusText || !InteractionStatusText || !ObserveButton || !ConfirmButton || !CancelButton) return;

    if (InteractionComponent.IsValid())
    {
        InteractionComponent->RefreshFocus();
        if (InteractionComponent->CanInteractNow())
        {
            InteractionStatusText->SetText(FText::Format(
                LOCTEXT("InteractionReady", "{0} — tap Observe."),
                ResolveInteractionPromptLabel()));
            ObserveButton->SetVisibility(ESlateVisibility::Visible);
            ObserveButton->SetIsEnabled(true);
        }
        else
        {
            InteractionStatusText->SetText(LOCTEXT("InteractionExplore", "Look around carefully. Something nearby may be worth observing."));
            ObserveButton->SetVisibility(ESlateVisibility::Collapsed);
            ObserveButton->SetIsEnabled(false);
        }
    }
    else
    {
        InteractionStatusText->SetText(FText::GetEmpty());
        ObserveButton->SetVisibility(ESlateVisibility::Collapsed);
        ObserveButton->SetIsEnabled(false);
    }

    if (UWorld* World = GetWorld())
    {
        if (UWMMissionRuntimeSubsystem* Missions = World->GetSubsystem<UWMMissionRuntimeSubsystem>())
        {
            const FText MissionIdText = FText::FromString(Missions->GetActiveMissionId().ToString());
            if (Missions->GetMissionState() == EWMMissionRuntimeState::Completed)
            {
                MissionStatusText->SetText(FText::Format(
                    LOCTEXT("MissionComplete", "{0}: mission complete — choose Next mission to continue."),
                    MissionIdText));
            }
            else if (Missions->GetActiveEvaluator() == FName(TEXT("observe-ecosystem")))
            {
                const int32 RequiredCount = Missions->GetRequiredObservationIds().Num();
                const int32 RecordedCount = Missions->GetRecordedObservationCount();
                MissionStatusText->SetText(FText::Format(
                    LOCTEXT("ScienceObserveProgress", "Rainforest mission — observe ecosystem clues: {0}/{1}. Look carefully, then tap Observe."),
                    FText::AsNumber(RecordedCount),
                    FText::AsNumber(RequiredCount)));
            }
            else if (!MissionMeasurementComponent.IsValid())
            {
                MissionStatusText->SetText(LOCTEXT("MissionMeasurementUnavailable", "Mission measurement tools are unavailable."));
            }
            else
            {
                const EWMMissionMeasurementState MeasurementState = MissionMeasurementComponent->GetMeasurementState();
                if (MeasurementState == EWMMissionMeasurementState::AwaitingSecondPoint)
                {
                    MissionStatusText->SetText(FText::Format(
                        LOCTEXT("MissionMeasureSecond", "{0}: point A selected. Aim at point B inside the mission zone and tap Measure."),
                        MissionIdText));
                }
                else if (MeasurementState == EWMMissionMeasurementState::Complete || Missions->HasMeasurementEvidence())
                {
                    const int32 MeasuredCm = FMath::RoundToInt(Missions->GetLastMeasuredSpanCm());
                    const float MeasuredMeters = Missions->GetLastMeasuredSpanCm() / 100.0f;
                    MissionStatusText->SetText(FText::Format(
                        LOCTEXT("MissionMeasuredBuildSpan", "{0} — Measured: {1} cm ({2} m). Build or adjust to about {3} cm."),
                        MissionIdText,
                        FText::AsNumber(MeasuredCm),
                        FText::AsNumber(MeasuredMeters),
                        FText::AsNumber(FMath::RoundToInt(Missions->GetTargetSpanCm()))));
                }
                else
                {
                    MissionStatusText->SetText(FText::Format(
                        LOCTEXT("MissionMeasureFirst", "{0}: aim at point A inside the mission zone and tap Measure."),
                        MissionIdText));
                }
            }
        }
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
    if (ConfirmText) ConfirmText->SetText(bMoveInProgress ? LOCTEXT("ConfirmMove", "Move here") : LOCTEXT("ConfirmBuild", "Build"));
}

void UWMBuildHUDWidget::HandlePreviousPiece() { if (BuildingComponent.IsValid()) BuildingComponent->CycleSelectedPiece(-1); RefreshStatus(); }
void UWMBuildHUDWidget::HandleNextPiece() { if (BuildingComponent.IsValid()) BuildingComponent->CycleSelectedPiece(1); RefreshStatus(); }
void UWMBuildHUDWidget::HandleRotateLeft() { if (BuildingComponent.IsValid()) BuildingComponent->RotatePreview(-1.0f); RefreshStatus(); }
void UWMBuildHUDWidget::HandleRotateRight() { if (BuildingComponent.IsValid()) BuildingComponent->RotatePreview(1.0f); RefreshStatus(); }
void UWMBuildHUDWidget::HandleConfirm() { if (BuildingComponent.IsValid()) BuildingComponent->TryPlaceCurrentPiece(); RefreshStatus(); }
void UWMBuildHUDWidget::HandleMeasure() { if (MissionMeasurementComponent.IsValid()) MissionMeasurementComponent->CapturePointFromView(); RefreshStatus(); }
void UWMBuildHUDWidget::HandleResetMeasurement() { if (MissionMeasurementComponent.IsValid()) MissionMeasurementComponent->ResetMeasurement(); RefreshStatus(); }
void UWMBuildHUDWidget::HandleObserve() { if (InteractionComponent.IsValid()) InteractionComponent->TryInteractFocused(); RefreshStatus(); }

void UWMBuildHUDWidget::HandleNextMission()
{
    if (MissionMeasurementComponent.IsValid()) MissionMeasurementComponent->ResetMeasurement();
    if (UWorld* World = GetWorld())
    {
        if (UWMMissionRuntimeSubsystem* Missions = World->GetSubsystem<UWMMissionRuntimeSubsystem>()) Missions->CycleMission(1);
    }
    RefreshStatus();
}

void UWMBuildHUDWidget::HandleMove() { if (BuildingComponent.IsValid()) BuildingComponent->TryBeginMoveTargetPiece(); RefreshStatus(); }
void UWMBuildHUDWidget::HandleRemove() { if (BuildingComponent.IsValid()) BuildingComponent->TryRemoveTargetPiece(); RefreshStatus(); }
void UWMBuildHUDWidget::HandleUndo() { if (BuildingComponent.IsValid()) BuildingComponent->UndoLastAction(); RefreshStatus(); }
void UWMBuildHUDWidget::HandleRedo() { if (BuildingComponent.IsValid()) BuildingComponent->RedoLastAction(); RefreshStatus(); }
void UWMBuildHUDWidget::HandleCancel() { if (BuildingComponent.IsValid()) BuildingComponent->CancelMove(); RefreshStatus(); }

#undef LOCTEXT_NAMESPACE
