#pragma once

#include "CoreMinimal.h"
#include "Visual/WMVisualProfileSettings.h"
#include "WMAvatarArtTypes.generated.h"

USTRUCT(BlueprintType)
struct FWMAvatarProportions
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "120.0", ClampMax = "190.0"))
    float HeightCm = 158.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "24.0", ClampMax = "38.0"))
    float HeadHeightCm = 31.6f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "24.0", ClampMax = "48.0"))
    float ShoulderWidthCm = 37.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "18.0", ClampMax = "38.0"))
    float HipWidthCm = 27.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "12.0", ClampMax = "30.0"))
    float TorsoDepthCm = 19.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "28.0", ClampMax = "60.0"))
    float TorsoHeightCm = 43.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "15.0", ClampMax = "45.0"))
    float UpperArmLengthCm = 27.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "15.0", ClampMax = "45.0"))
    float LowerArmLengthCm = 24.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "20.0", ClampMax = "55.0"))
    float ThighLengthCm = 35.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "20.0", ClampMax = "55.0"))
    float CalfLengthCm = 34.0f;

    float GetHeadsTall() const;
    bool IsSane() const;
};

USTRUCT(BlueprintType)
struct FWMAvatarPalette
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FLinearColor Skin = FLinearColor(0.66f, 0.40f, 0.27f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FLinearColor Hair = FLinearColor(0.12f, 0.075f, 0.055f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FLinearColor Top = FLinearColor(0.18f, 0.49f, 0.61f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FLinearColor Bottom = FLinearColor(0.18f, 0.23f, 0.34f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FLinearColor Footwear = FLinearColor(0.78f, 0.55f, 0.26f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FLinearColor Accent = FLinearColor(0.576f, 0.443f, 0.878f, 1.0f);
};

USTRUCT(BlueprintType)
struct FWMAvatarArtBudget
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 FacetSides = 6;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 HeadSegments = 8;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 MaxTriangles = 6500;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 MaxSkinnedBones = 48;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 MaxSkinInfluences = 4;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 MaxMaterialSlots = 3;

    bool IsSane() const;
};

UCLASS(Config = Game, DefaultConfig)
class WORLDMAKERS_API UWMAvatarVisualSettings : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "World Makers|Avatar")
    FString ProfileName = TEXT("ChildExplorerV1");

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "World Makers|Avatar")
    bool bUseProceduralAvatarArt = true;

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "World Makers|Avatar")
    FWMAvatarProportions Proportions;

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "World Makers|Avatar")
    FWMAvatarPalette Palette;

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "World Makers|Avatar")
    FWMAvatarArtBudget LowBudget;

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "World Makers|Avatar")
    FWMAvatarArtBudget MidBudget;

    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "World Makers|Avatar")
    FWMAvatarArtBudget HighBudget;

    const FWMAvatarArtBudget& GetBudget(EWMVisualQualityTier Tier) const;
};

struct FWMAvatarRigJointSpec
{
    FName JointId;
    FName ParentJointId;
};

/** Stable V4 joint and customization-slot vocabulary used by V5 animation and authored asset handoff. */
class WORLDMAKERS_API FWMAvatarRigContract
{
public:
    static const TArray<FWMAvatarRigJointSpec>& GetJoints();
    static const TArray<FName>& GetCustomizationSlots();
    static bool IsSane();
    static bool ContainsJoint(FName JointId);
    static FName GetParentJoint(FName JointId);
};
