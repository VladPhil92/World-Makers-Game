#pragma once

#include "CoreMinimal.h"
#include "WMTrustEconomyTypes.generated.h"

UENUM(BlueprintType)
enum class EWMGameplayRewardType : uint8
{
    CreativeUnlock,
    ToolUnlock,
    MasteryBadge,
    NarrativeUnlock,
    Discovery
};

/**
 * Runtime representation of an earned gameplay reward.
 * Monetary properties are deliberately methods with locked false semantics,
 * not editable fields. Paid commerce is outside the child runtime boundary.
 */
USTRUCT(BlueprintType)
struct WORLDMAKERS_API FWMGameplayRewardGrant
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Makers|Trust Economy")
    FName RewardId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Makers|Trust Economy")
    EWMGameplayRewardType Type = EWMGameplayRewardType::CreativeUnlock;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Makers|Trust Economy")
    FName PayloadId;

    bool IsPurchasable() const { return false; }
    bool IsTransferable() const { return false; }
    bool IsConvertibleToMoney() const { return false; }
};

/** Read-only family entitlement projection consumed by gameplay. */
USTRUCT(BlueprintType)
struct WORLDMAKERS_API FWMEntitlementSnapshot
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Makers|Entitlements")
    TArray<FName> EntitlementIds;

    bool HasEntitlement(FName EntitlementId) const;
};
