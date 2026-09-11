#include "Thought/WMCitizenshipRuntime.h"

namespace
{
    int32 UniqueValidCount(const TArray<FName>& Values)
    {
        TSet<FName> Unique;
        for (const FName Value : Values)
        {
            if (!Value.IsNone())
            {
                Unique.Add(Value);
            }
        }
        return Unique.Num();
    }

    float Ratio(int32 Passed, int32 Total)
    {
        return Total > 0 ? FMath::Clamp(static_cast<float>(Passed) / static_cast<float>(Total), 0.0f, 1.0f) : 0.0f;
    }
}

bool FWMXXIICitizenshipReasoningPolicy::IsSane() const
{
    const auto InRange = [](int32 Value) { return Value >= 0 && Value <= 8; };
    return InRange(MinReasons)
        && InRange(MinHumanPerspectives)
        && InRange(MinNonHumanPerspectives)
        && InRange(MinCommons)
        && InRange(MinTradeoffs)
        && InRange(MinUncertainties)
        && InRange(MinRevisions);
}

bool FWMCitizenshipRuntime::EvaluateDeliberation(
    const FWMXXIICitizenshipReasoningPolicy& Policy,
    const FWMXXIICitizenshipReasoningInput& Input,
    FName EvidenceEventId,
    FWMXXIICitizenshipEvidenceResult& OutResult)
{
    OutResult = {};
    OutResult.PrimitiveId = TEXT("reason-through-dilemma");
    OutResult.EvidenceEventId = EvidenceEventId;

    if (!Policy.IsSane() || EvidenceEventId.IsNone())
    {
        return false;
    }

    int32 Passed = 0;
    constexpr int32 Total = 9;
    Passed += UniqueValidCount(Input.ReasonIds) >= Policy.MinReasons ? 1 : 0;
    Passed += UniqueValidCount(Input.HumanPerspectiveIds) >= Policy.MinHumanPerspectives ? 1 : 0;
    Passed += UniqueValidCount(Input.NonHumanPerspectiveIds) >= Policy.MinNonHumanPerspectives ? 1 : 0;
    Passed += UniqueValidCount(Input.CommonsIds) >= Policy.MinCommons ? 1 : 0;
    Passed += UniqueValidCount(Input.TradeoffIds) >= Policy.MinTradeoffs ? 1 : 0;
    Passed += UniqueValidCount(Input.UncertaintyIds) >= Policy.MinUncertainties ? 1 : 0;
    Passed += Input.RevisionCount >= Policy.MinRevisions ? 1 : 0;
    Passed += (!Policy.bRequirePowerAsymmetryRecognition || Input.bRecognizedPowerAsymmetry) ? 1 : 0;
    Passed += (!Policy.bRequireReversibilityCheck || Input.bCheckedReversibility) ? 1 : 0;

    OutResult.CoverageScore = Ratio(Passed, Total);
    OutResult.bAccepted = Passed == Total;
    return OutResult.bAccepted;
}

bool FWMCitizenshipRuntime::EvaluateResponsibleAI(
    const FWMAIGovernanceReviewInput& Input,
    FName EvidenceEventId,
    FWMXXIICitizenshipEvidenceResult& OutResult)
{
    OutResult = {};
    OutResult.PrimitiveId = TEXT("construct-to-constraint");
    OutResult.EvidenceEventId = EvidenceEventId;

    if (EvidenceEventId.IsNone())
    {
        return false;
    }

    const bool Safeguards[] = {
        Input.bHumanAccountability,
        Input.bBoundedAuthority,
        Input.bAuditability,
        Input.bPrivacyAndFairness,
        Input.bFailSafe,
        Input.bEcologicalCostReviewed,
    };

    int32 Passed = 0;
    for (const bool bSafeguard : Safeguards)
    {
        Passed += bSafeguard ? 1 : 0;
    }

    OutResult.CoverageScore = Ratio(Passed, UE_ARRAY_COUNT(Safeguards));
    OutResult.bAccepted = Passed == UE_ARRAY_COUNT(Safeguards);
    return OutResult.bAccepted;
}

bool FWMCitizenshipRuntime::EvaluateSustainableTechnology(
    const FWMSustainableTechnologyReviewInput& Input,
    FName EvidenceEventId,
    FWMXXIICitizenshipEvidenceResult& OutResult)
{
    OutResult = {};
    OutResult.PrimitiveId = TEXT("model-system");
    OutResult.EvidenceEventId = EvidenceEventId;

    if (EvidenceEventId.IsNone())
    {
        return false;
    }

    const bool Dimensions[] = {
        Input.bDemandModeled,
        Input.bGenerationModeled,
        Input.bStorageOrFlexibilityConsidered,
        Input.bLifecycleMaterialsConsidered,
        Input.bWaterLandHabitatConsidered,
        Input.bResilienceConsidered,
    };

    int32 Passed = 0;
    for (const bool bDimension : Dimensions)
    {
        Passed += bDimension ? 1 : 0;
    }

    OutResult.CoverageScore = Ratio(Passed, UE_ARRAY_COUNT(Dimensions));
    OutResult.bAccepted = Passed == UE_ARRAY_COUNT(Dimensions);
    return OutResult.bAccepted;
}
