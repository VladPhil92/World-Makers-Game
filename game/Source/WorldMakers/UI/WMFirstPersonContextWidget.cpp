#include "UI/WMFirstPersonContextWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

#define LOCTEXT_NAMESPACE "WorldMakersFirstPersonContext"

void UWMFirstPersonContextWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (!WidgetTree) return;

    UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("FirstPersonContextRoot"));
    WidgetTree->RootWidget = Root;

    ContextCard = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("FirstPersonContextCard"));
    ContextCard->SetPadding(FMargin(14.0f, 10.0f));
    ContextCard->SetBrushColor(FLinearColor(0.035f, 0.055f, 0.075f, 0.86f));
    ContextCard->SetVisibility(ESlateVisibility::Collapsed);

    UCanvasPanelSlot* CardSlot = Root->AddChildToCanvas(ContextCard);
    CardSlot->SetAnchors(FAnchors(0.5f, 0.82f));
    CardSlot->SetAlignment(FVector2D(0.5f, 0.5f));
    CardSlot->SetAutoSize(true);
    CardSlot->SetZOrder(120);

    UVerticalBox* Stack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("FirstPersonContextStack"));
    ContextCard->SetContent(Stack);

    ModeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("FirstPersonModeText"));
    ModeText->SetJustification(ETextJustify::Center);
    ModeText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    UVerticalBoxSlot* ModeSlot = Stack->AddChildToVerticalBox(ModeText);
    ModeSlot->SetHorizontalAlignment(HAlign_Center);

    ActionText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("FirstPersonActionText"));
    ActionText->SetJustification(ETextJustify::Center);
    ActionText->SetAutoWrapText(true);
    ActionText->SetColorAndOpacity(FSlateColor(FLinearColor(0.90f, 0.94f, 0.97f, 1.0f)));
    UVerticalBoxSlot* ActionSlot = Stack->AddChildToVerticalBox(ActionText);
    ActionSlot->SetPadding(FMargin(0.0f, 3.0f, 0.0f, 0.0f));
    ActionSlot->SetHorizontalAlignment(HAlign_Center);
}

void UWMFirstPersonContextWidget::SetContext(const FName ModeId, const FName ActionId, const bool bReducedMotion)
{
    if (!ContextCard || !ModeText || !ActionText || ModeId.IsNone()) return;

    ActiveModeId = ModeId;
    ActiveActionId = ActionId;
    bContextReducedMotion = bReducedMotion;

    ModeText->SetText(ResolveModeLabel(ModeId));
    ModeText->SetColorAndOpacity(FSlateColor(ResolveModeAccent(ModeId)));
    ActionText->SetText(ResolveActionLabel(ActionId));
    ContextCard->SetRenderTranslation(FVector2D::ZeroVector);
    ContextCard->SetRenderScale(FVector2D(1.0f, 1.0f));
    ContextCard->SetRenderOpacity(1.0f);
    ContextCard->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UWMFirstPersonContextWidget::SetContextAlpha(const float Alpha)
{
    if (!ContextCard || ActiveModeId.IsNone()) return;
    const float SafeAlpha = FMath::Clamp(FMath::IsFinite(Alpha) ? Alpha : 0.0f, 0.0f, 1.0f);
    ContextCard->SetRenderOpacity(SafeAlpha);
    if (!bContextReducedMotion)
    {
        ContextCard->SetRenderTranslation(FVector2D(0.0f, FMath::Lerp(8.0f, 0.0f, SafeAlpha)));
        const float Scale = FMath::Lerp(0.985f, 1.0f, SafeAlpha);
        ContextCard->SetRenderScale(FVector2D(Scale, Scale));
    }
}

void UWMFirstPersonContextWidget::ClearContext()
{
    ActiveModeId = NAME_None;
    ActiveActionId = NAME_None;
    bContextReducedMotion = false;
    if (ContextCard)
    {
        ContextCard->SetRenderOpacity(0.0f);
        ContextCard->SetVisibility(ESlateVisibility::Collapsed);
    }
}

FText UWMFirstPersonContextWidget::ResolveModeLabel(const FName ModeId) const
{
    if (ModeId == TEXT("firstperson.build")) return LOCTEXT("BuildMode", "BUILD");
    if (ModeId == TEXT("firstperson.scan")) return LOCTEXT("ScanMode", "SCAN");
    if (ModeId == TEXT("firstperson.measure")) return LOCTEXT("MeasureMode", "MEASURE");
    if (ModeId == TEXT("firstperson.observe")) return LOCTEXT("ObserveMode", "OBSERVE");
    return LOCTEXT("ExploreMode", "EXPLORE");
}

FText UWMFirstPersonContextWidget::ResolveActionLabel(const FName ActionId) const
{
    if (ActionId == TEXT("build-point")) return LOCTEXT("BuildPoint", "Aim at the place you want to shape.");
    if (ActionId == TEXT("build-confirm")) return LOCTEXT("BuildConfirm", "Structure confirmed.");
    if (ActionId == TEXT("scan-anticipate") || ActionId == TEXT("scan-hold") || ActionId == TEXT("scan-settle")) return LOCTEXT("Scan", "Analyze the world without covering it.");
    if (ActionId == TEXT("measure-focus")) return LOCTEXT("Measure", "Compare distance and scale.");
    if (ActionId == TEXT("observe-focus")) return LOCTEXT("Observe", "Look closer. What can you infer?");
    if (ActionId == TEXT("tool-raise")) return LOCTEXT("ToolRaise", "Tool ready.");
    if (ActionId == TEXT("tool-lower")) return LOCTEXT("ToolLower", "Returning to exploration.");
    return LOCTEXT("Explore", "Explore, build and investigate.");
}

FLinearColor UWMFirstPersonContextWidget::ResolveModeAccent(const FName ModeId) const
{
    if (ModeId == TEXT("firstperson.build")) return FLinearColor(0.184f, 0.502f, 0.929f, 1.0f);
    if (ModeId == TEXT("firstperson.scan")) return FLinearColor(0.561f, 0.427f, 0.878f, 1.0f);
    if (ModeId == TEXT("firstperson.measure")) return FLinearColor(0.337f, 0.800f, 0.949f, 1.0f);
    if (ModeId == TEXT("firstperson.observe")) return FLinearColor(0.949f, 0.788f, 0.298f, 1.0f);
    return FLinearColor(0.435f, 0.812f, 0.592f, 1.0f);
}

#undef LOCTEXT_NAMESPACE
