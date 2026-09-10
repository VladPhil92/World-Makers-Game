#include "UI/WMChildJourneyWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "UI/WMChildJourneySubsystem.h"
#include "UI/WMChildJourneyTypes.h"

#define LOCTEXT_NAMESPACE "WorldMakersChildJourneyWidget"

void UWMChildJourneyWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (!WidgetTree) return;

    USizeBox* RootSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MyAdventuresSize"));
    RootSize->SetWidthOverride(PanelWidth);
    RootSize->SetHeightOverride(PanelHeight);
    WidgetTree->RootWidget = RootSize;

    UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MyAdventuresPanel"));
    Panel->SetPadding(FMargin(20.0f));
    RootSize->AddChild(Panel);

    UVerticalBox* Stack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MyAdventuresStack"));
    Panel->SetContent(Stack);

    UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MyAdventuresTitle"));
    Title->SetText(LOCTEXT("MyAdventures", "My Adventures"));
    Title->SetJustification(ETextJustify::Center);
    Stack->AddChildToVerticalBox(Title);

    UTextBlock* Subtitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MyAdventuresSubtitle"));
    Subtitle->SetText(LOCTEXT("CalmChoice", "Choose what you want to explore. You can stop whenever you like."));
    Subtitle->SetAutoWrapText(true);
    Subtitle->SetJustification(ETextJustify::Center);
    if (UVerticalBoxSlot* Slot = Stack->AddChildToVerticalBox(Subtitle)) Slot->SetPadding(FMargin(4.0f, 8.0f, 4.0f, 12.0f));

    LocationText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("AdventureLocation"));
    LocationText->SetJustification(ETextJustify::Center);
    Stack->AddChildToVerticalBox(LocationText);

    EcosystemText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("AdventureEcosystem"));
    EcosystemText->SetJustification(ETextJustify::Center);
    EcosystemText->SetAutoWrapText(true);
    if (UVerticalBoxSlot* Slot = Stack->AddChildToVerticalBox(EcosystemText)) Slot->SetPadding(FMargin(4.0f, 4.0f, 4.0f, 12.0f));

    CardList = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("AdventureCardList"));
    if (UVerticalBoxSlot* Slot = Stack->AddChildToVerticalBox(CardList))
    {
        Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        Slot->SetPadding(FMargin(0.0f, 4.0f));
    }

    UHorizontalBox* Footer = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("MyAdventuresFooter"));
    if (UVerticalBoxSlot* Slot = Stack->AddChildToVerticalBox(Footer))
    {
        Slot->SetHorizontalAlignment(HAlign_Center);
        Slot->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 0.0f));
    }

    ContinueButton = CreateFooterButton(Footer, TEXT("ContinueAdventureButton"), LOCTEXT("ContinueAdventure", "Continue Adventure"));
    UButton* CloseButton = CreateFooterButton(Footer, TEXT("CloseAdventuresButton"), LOCTEXT("CloseAdventures", "Close"));
    if (ContinueButton) ContinueButton->OnClicked.AddDynamic(this, &UWMChildJourneyWidget::HandleContinueAdventure);
    if (CloseButton) CloseButton->OnClicked.AddDynamic(this, &UWMChildJourneyWidget::HandleClose);

    if (UWorld* World = GetWorld())
    {
        JourneySubsystem = World->GetSubsystem<UWMChildJourneySubsystem>();
        World->GetTimerManager().SetTimer(RefreshTimer, this, &UWMChildJourneyWidget::RefreshJourney, RefreshSeconds, true);
    }

    RefreshJourney();
}

void UWMChildJourneyWidget::NativeDestruct()
{
    if (UWorld* World = GetWorld()) World->GetTimerManager().ClearTimer(RefreshTimer);
    Super::NativeDestruct();
}

UButton* UWMChildJourneyWidget::CreateFooterButton(UHorizontalBox* Row, const FName WidgetName, const FText& Label)
{
    if (!WidgetTree || !Row) return nullptr;

    USizeBox* Target = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), FName(*(WidgetName.ToString() + TEXT("Target"))));
    Target->SetWidthOverride(TouchTargetWidth);
    Target->SetHeightOverride(TouchTargetHeight);

    UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), WidgetName);
    Button->IsFocusable = false;
    UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(*(WidgetName.ToString() + TEXT("Label"))));
    Text->SetText(Label);
    Text->SetJustification(ETextJustify::Center);
    Button->AddChild(Text);
    Target->AddChild(Button);

    if (UHorizontalBoxSlot* Slot = Row->AddChildToHorizontalBox(Target))
    {
        Slot->SetPadding(FMargin(4.0f));
        Slot->SetVerticalAlignment(VAlign_Center);
    }
    return Button;
}

FText UWMChildJourneyWidget::BuildCardText(const FWMChildAdventureCard& Card) const
{
    const FText Title = UWMChildJourneySubsystem::ResolveChildTitle(Card.SourceId);
    const FText Description = UWMChildJourneySubsystem::ResolveChildDescription(Card.SourceId);
    const FText State = UWMChildJourneySubsystem::ResolveChildStateLabel(Card.State);

    if (Card.State == EWMChildAdventureState::InProgress && Card.Kind == EWMChildAdventureKind::Mission && Card.TotalUnits == 100)
    {
        return FText::Format(
            LOCTEXT("MissionCardPercent", "{0}\n{1}\n{2}: {3}"),
            Title,
            Description,
            State,
            FText::AsPercent(Card.ProgressFraction));
    }

    if (Card.State == EWMChildAdventureState::InProgress && Card.TotalUnits > 1)
    {
        return FText::Format(
            LOCTEXT("CountCardProgress", "{0}\n{1}\n{2}: {3} of {4}"),
            Title,
            Description,
            State,
            FText::AsNumber(Card.CurrentUnits),
            FText::AsNumber(Card.TotalUnits));
    }

    return FText::Format(
        LOCTEXT("AdventureCard", "{0}\n{1}\n{2}"),
        Title,
        Description,
        State);
}

void UWMChildJourneyWidget::RefreshJourney()
{
    if (!JourneySubsystem.IsValid())
    {
        if (UWorld* World = GetWorld()) JourneySubsystem = World->GetSubsystem<UWMChildJourneySubsystem>();
    }
    if (!JourneySubsystem.IsValid() || !CardList || !LocationText || !EcosystemText || !ContinueButton) return;

    const FWMChildJourneySnapshot Snapshot = JourneySubsystem->GetSnapshot();
    LocationText->SetText(FText::Format(
        LOCTEXT("ExploringLocation", "Exploring: {0}"),
        UWMChildJourneySubsystem::ResolveZoneLabel(Snapshot.ZoneId)));
    EcosystemText->SetText(UWMChildJourneySubsystem::ResolveEcosystemLabel(Snapshot.EcosystemReactionId));

    CardList->ClearChildren();
    bool bHasContinuableMission = false;
    for (const FWMChildAdventureCard& Card : Snapshot.Cards)
    {
        if (Card.State == EWMChildAdventureState::Hidden || !Card.IsSane()) continue;

        UTextBlock* CardText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        CardText->SetText(BuildCardText(Card));
        CardText->SetAutoWrapText(true);
        CardText->SetJustification(ETextJustify::Left);
        CardList->AddChild(CardText);
        CardText->SetRenderTransformPivot(FVector2D(0.0f, 0.0f));

        bHasContinuableMission = bHasContinuableMission ||
            (Card.Kind == EWMChildAdventureKind::Mission && Card.bSelectable &&
             (Card.State == EWMChildAdventureState::Ready || Card.State == EWMChildAdventureState::InProgress));
    }

    ContinueButton->SetIsEnabled(bHasContinuableMission);
}

void UWMChildJourneyWidget::SetPanelOpen(const bool bOpen)
{
    SetVisibility(bOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    if (bOpen) RefreshJourney();
}

bool UWMChildJourneyWidget::IsPanelOpen() const
{
    return GetVisibility() != ESlateVisibility::Collapsed && GetVisibility() != ESlateVisibility::Hidden;
}

void UWMChildJourneyWidget::HandleContinueAdventure()
{
    if (JourneySubsystem.IsValid()) JourneySubsystem->ActivateRecommendedMission();
    RefreshJourney();
}

void UWMChildJourneyWidget::HandleClose()
{
    SetPanelOpen(false);
}

#undef LOCTEXT_NAMESPACE
