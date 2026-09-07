#if WITH_DEV_AUTOMATION_TESTS

#include "Economy/WMTrustEconomyTypes.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMTrustEconomyRewardInvariantTest,
    "WorldMakers.TrustEconomy.Reward.Invariants",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMTrustEconomyRewardInvariantTest::RunTest(const FString& Parameters)
{
    const FWMGameplayRewardGrant Reward;
    TestFalse(TEXT("Gameplay rewards are never purchasable"), Reward.IsPurchasable());
    TestFalse(TEXT("Gameplay rewards are never transferable"), Reward.IsTransferable());
    TestFalse(TEXT("Gameplay rewards are never convertible to money"), Reward.IsConvertibleToMoney());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMTrustEconomyEntitlementProjectionTest,
    "WorldMakers.TrustEconomy.Entitlement.ReadOnlyProjection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMTrustEconomyEntitlementProjectionTest::RunTest(const FString& Parameters)
{
    FWMEntitlementSnapshot Snapshot;
    Snapshot.EntitlementIds.Add(TEXT("entitlement.expansion.caribbean-explorer-01"));

    TestTrue(
        TEXT("Authorized entitlement is readable"),
        Snapshot.HasEntitlement(TEXT("entitlement.expansion.caribbean-explorer-01")));
    TestFalse(
        TEXT("Unknown entitlement is not minted by the client projection"),
        Snapshot.HasEntitlement(TEXT("entitlement.expansion.unknown")));
    TestFalse(TEXT("None is never an entitlement"), Snapshot.HasEntitlement(NAME_None));
    return true;
}

#endif
