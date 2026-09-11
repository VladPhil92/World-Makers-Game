#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WMFirstPersonContextWidget.generated.h"

class UBorder;
class UTextBlock;

UCLASS()
class WORLDMAKERS_API UWMFirstPersonContextWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeOnInitialized() override;

    void SetContext(FName ModeId, FName ActionId, bool bReducedMotion);
    void SetContextAlpha(float Alpha);
    void ClearContext();

    UFUNCTION(BlueprintPure, Category = "World Makers|First Person")
    FName GetActiveModeId() const { return ActiveModeId; }

    UFUNCTION(BlueprintPure, Category = "World Makers|First Person")
    FName GetActiveActionId() const { return ActiveActionId; }

private:
    FText ResolveModeLabel(FName ModeId) const;
    FText ResolveActionLabel(FName ActionId) const;
    FLinearColor ResolveModeAccent(FName ModeId) const;

    UPROPERTY(Transient)
    TObjectPtr<UBorder> ContextCard;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> ModeText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> ActionText;

    FName ActiveModeId = NAME_None;
    FName ActiveActionId = NAME_None;
    bool bContextReducedMotion = false;
};
