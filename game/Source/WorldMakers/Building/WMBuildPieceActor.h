#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WMBuildPieceActor.generated.h"

class UStaticMeshComponent;
class USceneComponent;

UCLASS(Blueprintable)
class WORLDMAKERS_API AWMBuildPieceActor : public AActor
{
    GENERATED_BODY()

public:
    AWMBuildPieceActor();

    UFUNCTION(BlueprintCallable, Category = "World Makers|Building")
    void SetPreviewState(bool bPreview);

    UFUNCTION(BlueprintPure, Category = "World Makers|Building")
    bool IsPreview() const { return bIsPreview; }

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Makers|Building")
    FName PieceId = TEXT("prototype.cube");

    static const FName PlacedBuildTag;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Building")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Makers|Building")
    TObjectPtr<UStaticMeshComponent> Mesh;

private:
    bool bIsPreview = false;
};
