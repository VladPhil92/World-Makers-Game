#include "UI/WMPresentationOverlayWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Visual/WMPresentationRuntime.h"

#define LOCTEXT_NAMESPACE "WorldMakersPresentation"

void UWMPresentationOverlayWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (!WidgetTree) return;

    UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("PresentationRoot"));
    WidgetTree->RootWidget = Root;

    CueCard = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PresentationCueCard"));
    CueCard->SetPadding(FMargin(22.0f, 12.0f));
    CueCard->SetBrushColor(FLinearColor(0.035f, 0.055f, 0.075f, 0.82f));
    CueCard->SetVisibility(ESlateVisibility::Collapsed);

    UCanvasPanelSlot* CardSlot = Root->AddChildToCanvas(CueCard);
    CardSlot->SetAnchors(FAnchors(0.5f, 0.16f));
    CardSlot->SetAlignment(FVector2D(0.5f, 0.5f));
    CardSlot->SetAutoSize(true);
    CardSlot->SetZOrder(100);

    CueText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PresentationCueText"));
    CueText->SetJustification(ETextJustify::Center);
    CueText->SetAutoWrapText(true);
    CueCard->SetContent(CueText);
}

FText UWMPresentationOverlayWidget::ResolveCueText(const FName CueId) const
{
    if (CueId == TEXT("presentation.build.confirm")) return LOCTEXT("BuildConfirm", "Your idea changed the world.");
    if (CueId == TEXT("presentation.observe.focus")) return LOCTEXT("ObserveFocus", "Look closer. What changed?");
    if (CueId == TEXT("presentation.science.focus")) return LOCTEXT("ScienceFocus", "Watch the system respond.");
    if (CueId == TEXT("presentation.adventure.reveal")) return LOCTEXT("AdventureReveal", "A new path awakens.");
    if (CueId == TEXT("presentation.mission.changed")) return LOCTEXT("MissionChanged", "A new challenge is ready.");
    if (CueId == TEXT("presentation.ecology.changed")) return LOCTEXT("EcologyChanged", "The living world is responding.");
    return FText::GetEmpty();
}

bool UWMPresentationOverlayWidget::ShowCue(const FName CueId, const float DurationSeconds, const bool bReducedMotion)
{
    if (!CueCard || !CueText || !FWMPresentationRuntime::IsSupportedCue(CueId)) return false;

    const float SafeDuration = FWMPresentationRuntime::ClampPulseDuration(DurationSeconds, bReducedMotion);
    if (SafeDuration <= 0.0f) return false;

    const FText Label = ResolveCueText(CueId);
    if (Label.IsEmpty()) return false;

    ActiveCueId = CueId;
    CueAgeSeconds = 0.0f;
    CueDurationSeconds = SafeDuration;
    bCueReducedMotion = bReducedMotion;
    CueText->SetText(Label);
    CueCard->SetVisibility(ESlateVisibility::HitTestInvisible);
    return true;
}

void UWMPresentationOverlayWidget::DismissCue()
{
    ActiveCueId = NAME_None;
    CueAgeSeconds = 0.0f;
    CueDurationSeconds = 0.0f;
    if (CueCard)
    {
        CueCard->SetRenderOpacity(0.0f);
        CueCard->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void UWMPresentationOverlayWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    if (!CueCard || ActiveCueId.IsNone()) return;

    CueAgeSeconds += FMath::Max(0.0f, InDeltaTime);
    if (CueAgeSeconds >= CueDurationSeconds)
    {
        DismissCue();
        return;
    }

    const FWMUIMotionPose Pose = FWMPresentationRuntime::EvaluateUIMotion(CueAgeSeconds, CueDurationSeconds, bCueReducedMotion);
    CueCard->SetRenderOpacity(Pose.Opacity);
    CueCard->SetRenderTranslation(FVector2D(0.0f, Pose.TranslationYPx));
    CueCard->SetRenderScale(FVector2D(Pose.Scale, Pose.Scale));
}

#undef LOCTEXT_NAMESPACE
