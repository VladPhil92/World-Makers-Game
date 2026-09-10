#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TimerManager.h"
#include "UI/WMChildJourneyTypes.h"
#include "WMChildJourneyWidget.generated.h"

class UButton;
class UHorizontalBox;
class UScrollBox;
class UTextBlock;
class UWMChildJourneySubsystem;

UCLASS()
class WORLDMAKERS_API UWMChildJourneyWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "World Makers|Child Journey")
    void SetPanelOpen(bool bOpen);

    UFUNCTION(BlueprintPure, Category = "World Makers|Child Journey")
    bool IsPanelOpen() const;

    UFUNCTION(BlueprintCallable, Category = "World Makers|Child Journey")
    void RefreshJourney();

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeDestruct() override;

private:
    UButton* CreateFooterButton(UHorizontalBox* Row, FName WidgetName, const FText& Label);
    FText BuildCardText(const FWMChildAdventureCard& Card) const;

    UFUNCTION()
    void HandleContinueAdventure();

    UFUNCTION()
    void HandleClose();

    TWeakObjectPtr<UWMChildJourneySubsystem> JourneySubsystem;
    FTimerHandle RefreshTimer;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> LocationText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> EcosystemText;

    UPROPERTY(Transient)
    TObjectPtr<UScrollBox> CardList;

    UPROPERTY(Transient)
    TObjectPtr<UButton> ContinueButton;

    static constexpr float PanelWidth = 520.0f;
    static constexpr float PanelHeight = 640.0f;
    static constexpr float TouchTargetWidth = 168.0f;
    static constexpr float TouchTargetHeight = 72.0f;
    static constexpr float RefreshSeconds = 0.25f;
};
