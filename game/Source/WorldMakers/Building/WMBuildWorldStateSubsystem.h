#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WMBuildWorldStateSubsystem.generated.h"

USTRUCT(BlueprintType)
struct WORLDMAKERS_API FWMPlacedBuildPieceSnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Building")
    FName PieceId;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Building")
    FVector LocationCm = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Building")
    float YawDegrees = 0.0f;

    bool IsSane() const;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWMBuildWorldChangedSignature, int32, Revision);

/**
 * Neutral Building-domain projection of placed pieces.
 * Consumers may observe stable IDs/geometry without Building importing their domain.
 */
UCLASS()
class WORLDMAKERS_API UWMBuildWorldStateSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    void PublishSnapshot(const TArray<FWMPlacedBuildPieceSnapshot>& InPieces);

    UFUNCTION(BlueprintPure, Category = "World Makers|Building")
    TArray<FWMPlacedBuildPieceSnapshot> GetPlacedPieces() const { return PlacedPieces; }

    UFUNCTION(BlueprintPure, Category = "World Makers|Building")
    int32 GetRevision() const { return Revision; }

    UPROPERTY(BlueprintAssignable, Category = "World Makers|Building")
    FWMBuildWorldChangedSignature OnBuildWorldChanged;

private:
    TArray<FWMPlacedBuildPieceSnapshot> PlacedPieces;
    int32 Revision = 0;
};
