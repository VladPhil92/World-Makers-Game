#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WMBuildPieceActor.generated.h"

class UStaticMeshComponent;
class USceneComponent;
struct FWMBuildPieceSpec;

UCLASS(Blueprintable)
class WORLDMAKERS_API AWMBuildPieceActor : public AActor
{
    GENERATED_BODY()

public:
    AWMBuildPieceActor();

    UFUNCTION(BlueprintCallable, Category = "World Makers|Building")
    void SetPreviewState(bool bPreview);

    UFUNCTION(BlueprintCallable, Category = "World Makers|Building")
    void SetPreviewValidity(bool bValid);

    UFUNCTION(BlueprintCallable, Category = "World Makers|Building")
    void ApplyPieceSpec(const FWMBuildPieceSpec& Spec);

    UFUNCTION(BlueprintPure, Category = "World Makers|Building")
    bool IsPreview() const { return bIsPreview; }

    UFUNCTION(BlueprintPure, Category = "World Makers|Building")
    bool IsPreviewValidityState() const { return bPreviewPlacementValid; }

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Makers|Building")
    FName PieceId = TEXT("prototype.cube");

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Building")
    FVector PieceDimensionsCm = FVector(100.0f);

    static const FName PlacedBuildTag;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Building")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Building")
    TObjectPtr<UStaticMeshComponent> Mesh;

private:
    void ApplyPlacedSurface();

    FString PieceCategory = TEXT("Block");
    bool bIsPreview = false;
    bool bPreviewPlacementValid = false;
};
