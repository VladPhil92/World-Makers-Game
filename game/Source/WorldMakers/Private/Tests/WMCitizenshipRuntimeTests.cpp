#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Thought/WMCitizenshipRuntime.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMCitizenshipDeliberationTest,
    "WorldMakers.Citizenship.Deliberation.ReasonsAcrossHumanNonHumanAndCommons",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMCitizenshipDeliberationTest::RunTest(const FString& Parameters)
{
    FWMXXIICitizenshipReasoningPolicy Policy;
    FWMXXIICitizenshipReasoningInput Input;
    Input.ReasonIds = { TEXT("reason.energy-access"), TEXT("reason.habitat-connectivity") };
    Input.HumanPerspectiveIds = { TEXT("human.river-town") };
    Input.NonHumanPerspectiveIds = { TEXT("animal.migratory-fish") };
    Input.CommonsIds = { TEXT("commons.river") };
    Input.TradeoffIds = { TEXT("tradeoff.energy-vs-habitat") };
    Input.UncertaintyIds = { TEXT("uncertainty.rainfall-variability") };
    Input.RevisionCount = 1;
    Input.bRecognizedPowerAsymmetry = true;
    Input.bCheckedReversibility = true;

    FWMXXIICitizenshipEvidenceResult Result;
    TestTrue(TEXT("Structured interspecies deliberation passes"), FWMCitizenshipRuntime::EvaluateDeliberation(
        Policy, Input, TEXT("citizenship.river.multi-species-energy-reasoning-demonstrated"), Result));
    TestEqual(TEXT("Ethics primitive is emitted"), Result.PrimitiveId, FName(TEXT("reason-through-dilemma")));
    TestTrue(TEXT("Coverage is complete"), FMath::IsNearlyEqual(Result.CoverageScore, 1.0f));

    Input.NonHumanPerspectiveIds.Reset();
    TestFalse(TEXT("Human-only reasoning cannot satisfy an interspecies case"), FWMCitizenshipRuntime::EvaluateDeliberation(
        Policy, Input, TEXT("citizenship.river.multi-species-energy-reasoning-demonstrated"), Result));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMCitizenshipNoIdeologyKeyTest,
    "WorldMakers.Citizenship.Deliberation.NoCorrectMoralOptionKey",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMCitizenshipNoIdeologyKeyTest::RunTest(const FString& Parameters)
{
    FWMXXIICitizenshipReasoningPolicy Policy;
    FWMXXIICitizenshipReasoningInput Input;
    Input.ReasonIds = { TEXT("reason.shared-decision"), TEXT("reason.reversibility") };
    Input.HumanPerspectiveIds = { TEXT("human.residents") };
    Input.NonHumanPerspectiveIds = { TEXT("animal.urban-wildlife") };
    Input.CommonsIds = { TEXT("commons.water") };
    Input.TradeoffIds = { TEXT("tradeoff.predictability-vs-adaptation") };
    Input.UncertaintyIds = { TEXT("uncertainty.ecosystem-thresholds") };
    Input.RevisionCount = 1;
    Input.bRecognizedPowerAsymmetry = true;
    Input.bCheckedReversibility = true;

    FWMXXIICitizenshipEvidenceResult Result;
    TestTrue(TEXT("Reasoning can pass without supplying any moral option identifier"), FWMCitizenshipRuntime::EvaluateDeliberation(
        Policy, Input, TEXT("citizenship.hospitality.community-design-reasoning-demonstrated"), Result));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMResponsibleAIGovernanceTest,
    "WorldMakers.Citizenship.AI.BoundedAuditableAccountableFailSafe",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMResponsibleAIGovernanceTest::RunTest(const FString& Parameters)
{
    FWMAIGovernanceReviewInput Input;
    Input.bHumanAccountability = true;
    Input.bBoundedAuthority = true;
    Input.bAuditability = true;
    Input.bPrivacyAndFairness = true;
    Input.bFailSafe = true;
    Input.bEcologicalCostReviewed = true;

    FWMXXIICitizenshipEvidenceResult Result;
    TestTrue(TEXT("Responsible AI safeguards pass as a complete governance set"), FWMCitizenshipRuntime::EvaluateResponsibleAI(
        Input, TEXT("citizenship.ai.governance-safeguards-configured"), Result));

    Input.bHumanAccountability = false;
    TestFalse(TEXT("Autonomy without accountable human responsibility is insufficient"), FWMCitizenshipRuntime::EvaluateResponsibleAI(
        Input, TEXT("citizenship.ai.governance-safeguards-configured"), Result));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMSustainableTechnologySystemTest,
    "WorldMakers.Citizenship.Technology.WholeSystemNotGreenLabel",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMSustainableTechnologySystemTest::RunTest(const FString& Parameters)
{
    FWMSustainableTechnologyReviewInput Input;
    Input.bDemandModeled = true;
    Input.bGenerationModeled = true;
    Input.bStorageOrFlexibilityConsidered = true;
    Input.bLifecycleMaterialsConsidered = true;
    Input.bWaterLandHabitatConsidered = true;
    Input.bResilienceConsidered = true;

    FWMXXIICitizenshipEvidenceResult Result;
    TestTrue(TEXT("Whole-system energy reasoning passes"), FWMCitizenshipRuntime::EvaluateSustainableTechnology(
        Input, TEXT("citizenship.energy.system-design-reasoning-demonstrated"), Result));

    Input.bLifecycleMaterialsConsidered = false;
    TestFalse(TEXT("A renewable label alone cannot satisfy lifecycle reasoning"), FWMCitizenshipRuntime::EvaluateSustainableTechnology(
        Input, TEXT("citizenship.energy.system-design-reasoning-demonstrated"), Result));
    return true;
}

#endif
