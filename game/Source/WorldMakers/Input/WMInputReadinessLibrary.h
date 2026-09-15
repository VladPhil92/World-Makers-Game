#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WMInputReadinessLibrary.generated.h"

UENUM(BlueprintType)
enum class EWMInputSemantic : uint8
{
    Move,
    Look,
    Jump,
    Interact,
    Observe,
    PlaceBuild,
    RotateBuild,
    RemoveBuild,
    MoveBuild,
    CycleBuildPiece,
    Cancel,
    Undo,
    Redo,
    Measure,
    CycleMission,
    Pause
};

USTRUCT(BlueprintType)
struct FWMTouchControlSpec
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World Makers|Input")
    FName ControlId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World Makers|Input")
    EWMInputSemantic Semantic = EWMInputSemantic::Interact;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World Makers|Input")
    FVector2D NormalizedCenter = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World Makers|Input")
    FVector2D NormalizedSize = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World Makers|Input")
    bool bAnalog = false;
};

/** Stable input semantics shared by legacy bindings, Enhanced Input assets and touch UI. */
UCLASS()
class WORLDMAKERS_API UWMInputReadinessLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category="World Makers|Input")
    static TArray<FName> GetCanonicalActionIds();

    UFUNCTION(BlueprintPure, Category="World Makers|Input")
    static TArray<FWMTouchControlSpec> GetDefaultTabletTouchLayout();

    UFUNCTION(BlueprintPure, Category="World Makers|Input")
    static FName ToCanonicalActionId(EWMInputSemantic Semantic);
};
