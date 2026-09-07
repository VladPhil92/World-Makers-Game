#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WMBuildingComponent.generated.h"

class AWMBuildPieceActor;

UCLASS(ClassGroup = (WorldMakers), meta = (BlueprintSpawnableComponent))
class WORLDMAKERS_API UWMBuildingComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UWMBuildingComponent();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintCallable, Category = "World Makers|Building")
    bool TryPlaceCurrentPiece();

    UFUNCTION(BlueprintCallable, Category = "World Makers|Building")
    bool TryRemoveTargetPiece();

    UFUNCTION(BlueprintCallable, Category = "World Makers|Building")
    void RotatePreview(float Direction = 1.0f);

    UFUNCTION(BlueprintCallable, Category = "World Makers|Building")
    bool UndoLastAction();

    UFUNCTION(BlueprintCallable, Category = "World Makers|Building")
    bool RedoLastAction();

    UFUNCTION(BlueprintCallable, Category = "World Makers|Building")
    bool SaveWorld(const FString& SlotName = TEXT("WorldMakersPrototype"));

    UFUNCTION(BlueprintCallable, Category = "World Makers|Building")
    bool LoadWorld(const FString& SlotName = TEXT("WorldMakersPrototype"));

    UFUNCTION(BlueprintPure, Category = "World Makers|Building")
    bool IsPreviewPlacementValid() const { return bHasPlacementTarget; }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Makers|Building", meta = (ClampMin = "25.0"))
    float GridSize = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Makers|Building", meta = (ClampMin = "15.0", ClampMax = "180.0"))
    float RotationStepDegrees = 90.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Makers|Building", meta = (ClampMin = "200.0"))
    float BuildDistance = 1500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Makers|Building", meta = (ClampMin = "0.10", ClampMax = "0.49"))
    float PlacementClearanceRatio = 0.45f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Makers|Building", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MinPlacementSurfaceUpDot = 0.65f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Makers|Building")
    TSubclassOf<AWMBuildPieceActor> BuildPieceClass;

protected:
    UPROPERTY(Transient)
    TObjectPtr<AWMBuildPieceActor> PreviewActor;

private:
    enum class EWMBuildCommandType : uint8
    {
        Place,
        Remove
    };

    struct FWMBuildCommand
    {
        EWMBuildCommandType Type = EWMBuildCommandType::Place;
        FTransform Transform = FTransform::Identity;
        FName PieceId = NAME_None;
        TWeakObjectPtr<AWMBuildPieceActor> ActiveActor;
    };

    bool UpdatePreviewTransform();
    bool GetViewTrace(FHitResult& OutHit, bool bIgnorePreview) const;
    bool IsPlacementValid(const FTransform& CandidateTransform, const AActor* SupportingActor) const;
    AWMBuildPieceActor* SpawnPlacedPiece(const FTransform& Transform, FName PieceId);
    void EnsurePreviewActor();
    void PushCommand(const FWMBuildCommand& Command);
    void DestroyAllPlacedPieces();

    float CurrentYaw = 0.0f;
    bool bHasPlacementTarget = false;
    TArray<FWMBuildCommand> UndoStack;
    TArray<FWMBuildCommand> RedoStack;
    static constexpr int32 MaxHistoryEntries = 50;
    static constexpr int32 MaxSavedPieces = 5000;
};
