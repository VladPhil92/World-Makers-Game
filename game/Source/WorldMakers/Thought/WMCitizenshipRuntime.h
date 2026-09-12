#pragma once

#include "CoreMinimal.h"

struct WORLDMAKERS_API FWMXXIICitizenshipReasoningPolicy
{
    int32 MinReasons = 2;
    int32 MinHumanPerspectives = 1;
    int32 MinNonHumanPerspectives = 1;
    int32 MinCommons = 1;
    int32 MinTradeoffs = 1;
    int32 MinUncertainties = 1;
    int32 MinRevisions = 1;
    bool bRequirePowerAsymmetryRecognition = true;
    bool bRequireReversibilityCheck = true;

    bool IsSane() const;
};

struct WORLDMAKERS_API FWMXXIICitizenshipReasoningInput
{
    TArray<FName> ReasonIds;
    TArray<FName> HumanPerspectiveIds;
    TArray<FName> NonHumanPerspectiveIds;
    TArray<FName> CommonsIds;
    TArray<FName> TradeoffIds;
    TArray<FName> UncertaintyIds;
    int32 RevisionCount = 0;
    bool bRecognizedPowerAsymmetry = false;
    bool bCheckedReversibility = false;
};

struct WORLDMAKERS_API FWMXXIICitizenshipEvidenceResult
{
    bool bAccepted = false;
    FName PrimitiveId;
    FName EvidenceEventId;
    float CoverageScore = 0.0f;
};

struct WORLDMAKERS_API FWMAIGovernanceReviewInput
{
    bool bHumanAccountability = false;
    bool bBoundedAuthority = false;
    bool bAuditability = false;
    bool bPrivacyAndFairness = false;
    bool bFailSafe = false;
    bool bEcologicalCostReviewed = false;
};

struct WORLDMAKERS_API FWMSustainableTechnologyReviewInput
{
    bool bDemandModeled = false;
    bool bGenerationModeled = false;
    bool bStorageOrFlexibilityConsidered = false;
    bool bLifecycleMaterialsConsidered = false;
    bool bWaterLandHabitatConsidered = false;
    bool bResilienceConsidered = false;
};

/**
 * Structured citizenship reasoning for World Makers.
 *
 * This runtime never receives a "correct moral option". It evaluates whether a player
 * considered reasons, affected human and non-human lives, commons, tradeoffs,
 * uncertainty, power and revision. Stable IDs only; no child-authored free text.
 */
struct WORLDMAKERS_API FWMCitizenshipRuntime
{
    static bool EvaluateDeliberation(
        const FWMXXIICitizenshipReasoningPolicy& Policy,
        const FWMXXIICitizenshipReasoningInput& Input,
        FName EvidenceEventId,
        FWMXXIICitizenshipEvidenceResult& OutResult);

    /** Evaluates governance safeguards, not whether AI has or lacks moral status. */
    static bool EvaluateResponsibleAI(
        const FWMAIGovernanceReviewInput& Input,
        FName EvidenceEventId,
        FWMXXIICitizenshipEvidenceResult& OutResult);

    /** Evaluates whole-system technology reasoning rather than a preferred energy technology. */
    static bool EvaluateSustainableTechnology(
        const FWMSustainableTechnologyReviewInput& Input,
        FName EvidenceEventId,
        FWMXXIICitizenshipEvidenceResult& OutResult);
};
