#include "Visual/WMAvatarArtTypes.h"

float FWMAvatarProportions::GetHeadsTall() const
{
    return HeadHeightCm > KINDA_SMALL_NUMBER ? HeightCm / HeadHeightCm : 0.0f;
}

bool FWMAvatarProportions::IsSane() const
{
    const float HeadsTall = GetHeadsTall();
    return FMath::IsFinite(HeightCm) && HeightCm >= 120.0f && HeightCm <= 190.0f &&
        FMath::IsFinite(HeadHeightCm) && HeadHeightCm >= 24.0f && HeadHeightCm <= 38.0f &&
        HeadsTall >= 4.5f && HeadsTall <= 5.2f &&
        ShoulderWidthCm > HipWidthCm && ShoulderWidthCm <= 48.0f && HipWidthCm >= 18.0f &&
        TorsoDepthCm >= 12.0f && TorsoDepthCm <= 30.0f &&
        TorsoHeightCm >= 28.0f && TorsoHeightCm <= 60.0f &&
        UpperArmLengthCm > 0.0f && LowerArmLengthCm > 0.0f &&
        ThighLengthCm > 0.0f && CalfLengthCm > 0.0f;
}

bool FWMAvatarArtBudget::IsSane() const
{
    return FacetSides >= 5 && FacetSides <= 12 &&
        HeadSegments >= 6 && HeadSegments <= 16 &&
        MaxTriangles >= 1000 && MaxTriangles <= 50000 &&
        MaxSkinnedBones >= 19 && MaxSkinnedBones <= 128 &&
        MaxSkinInfluences >= 2 && MaxSkinInfluences <= 8 &&
        MaxMaterialSlots >= 1 && MaxMaterialSlots <= 6;
}

const FWMAvatarArtBudget& UWMAvatarVisualSettings::GetBudget(const EWMVisualQualityTier Tier) const
{
    switch (Tier)
    {
    case EWMVisualQualityTier::Low:
        return LowBudget;
    case EWMVisualQualityTier::High:
        return HighBudget;
    case EWMVisualQualityTier::Mid:
    default:
        return MidBudget;
    }
}

const TArray<FWMAvatarRigJointSpec>& FWMAvatarRigContract::GetJoints()
{
    static const TArray<FWMAvatarRigJointSpec> Joints = {
        {FName(TEXT("root")), NAME_None},
        {FName(TEXT("pelvis")), FName(TEXT("root"))},
        {FName(TEXT("spine")), FName(TEXT("pelvis"))},
        {FName(TEXT("chest")), FName(TEXT("spine"))},
        {FName(TEXT("neck")), FName(TEXT("chest"))},
        {FName(TEXT("head")), FName(TEXT("neck"))},
        {FName(TEXT("jaw")), FName(TEXT("head"))},
        {FName(TEXT("upperarm_l")), FName(TEXT("chest"))},
        {FName(TEXT("lowerarm_l")), FName(TEXT("upperarm_l"))},
        {FName(TEXT("hand_l")), FName(TEXT("lowerarm_l"))},
        {FName(TEXT("upperarm_r")), FName(TEXT("chest"))},
        {FName(TEXT("lowerarm_r")), FName(TEXT("upperarm_r"))},
        {FName(TEXT("hand_r")), FName(TEXT("lowerarm_r"))},
        {FName(TEXT("thigh_l")), FName(TEXT("pelvis"))},
        {FName(TEXT("calf_l")), FName(TEXT("thigh_l"))},
        {FName(TEXT("foot_l")), FName(TEXT("calf_l"))},
        {FName(TEXT("thigh_r")), FName(TEXT("pelvis"))},
        {FName(TEXT("calf_r")), FName(TEXT("thigh_r"))},
        {FName(TEXT("foot_r")), FName(TEXT("calf_r"))},
    };
    return Joints;
}

const TArray<FName>& FWMAvatarRigContract::GetCustomizationSlots()
{
    static const TArray<FName> Slots = {
        FName(TEXT("body")),
        FName(TEXT("hair")),
        FName(TEXT("top")),
        FName(TEXT("bottom")),
        FName(TEXT("footwear")),
        FName(TEXT("head-accessory")),
        FName(TEXT("back-accessory")),
        FName(TEXT("hand-prop")),
    };
    return Slots;
}

bool FWMAvatarRigContract::ContainsJoint(const FName JointId)
{
    return GetJoints().ContainsByPredicate([JointId](const FWMAvatarRigJointSpec& Joint)
    {
        return Joint.JointId == JointId;
    });
}

FName FWMAvatarRigContract::GetParentJoint(const FName JointId)
{
    const FWMAvatarRigJointSpec* Joint = GetJoints().FindByPredicate([JointId](const FWMAvatarRigJointSpec& Candidate)
    {
        return Candidate.JointId == JointId;
    });
    return Joint ? Joint->ParentJointId : NAME_None;
}

bool FWMAvatarRigContract::IsSane()
{
    const TArray<FWMAvatarRigJointSpec>& Joints = GetJoints();
    if (Joints.Num() != 19 || GetCustomizationSlots().Num() < 8)
    {
        return false;
    }

    TSet<FName> Seen;
    for (const FWMAvatarRigJointSpec& Joint : Joints)
    {
        if (Joint.JointId.IsNone() || Seen.Contains(Joint.JointId))
        {
            return false;
        }
        if (!Joint.ParentJointId.IsNone() && !Seen.Contains(Joint.ParentJointId))
        {
            return false;
        }
        Seen.Add(Joint.JointId);
    }
    return Seen.Contains(FName(TEXT("head"))) && Seen.Contains(FName(TEXT("hand_l"))) && Seen.Contains(FName(TEXT("hand_r"))) &&
        Seen.Contains(FName(TEXT("foot_l"))) && Seen.Contains(FName(TEXT("foot_r")));
}
