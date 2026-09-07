#include "Economy/WMTrustEconomyTypes.h"

bool FWMEntitlementSnapshot::HasEntitlement(const FName EntitlementId) const
{
    return !EntitlementId.IsNone() && EntitlementIds.Contains(EntitlementId);
}
