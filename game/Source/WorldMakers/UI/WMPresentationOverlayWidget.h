#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WMPresentationOverlayWidget.generated.h"

class UBorder;
class UTextBlock;

UCLASS()
class WORLDMAKERS_API UWMPresentationOverlayWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "World Makers|Presentation")
    bool ShowCue(FName CueId, float DurationSeconds, bool bReducedMotion);

    UFUNCTION(BlueprintCallable, Category = "World Makers|Presentation")
    void DismissCue();

    UFUNCTION(BlueprintPure, Category = "World Makers|Presentation")
    FName GetActiveCueId() const { return ActiveCueId; }

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    FText ResolveCueText(FName CueId) const;

    UPROPERTY(Transient)
    TObjectPtr<UBorder> CueCard;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> CueText;

    FName ActiveCueId = NAME_None;
    float CueAgeSeconds = 0.0f;
    float CueDurationSeconds = 0.0f;
    bool bCueReducedMotion = false;
};
