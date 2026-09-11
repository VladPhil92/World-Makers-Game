#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Performance/WMVisualCertificationTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMVisualCertificationPassingSampleTest,
    "WorldMakers.Visual.Certification.PassingSampleRequiresMeasuredBudgetCompliance",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMVisualCertificationPassingSampleTest::RunTest(const FString& Parameters)
{
    FWMVisualCertificationBudget Budget;
    FWMVisualCertificationSample Sample;
    Sample.FrameSamples = 1800;
    Sample.P95FrameTimeMs = 31.0f;
    Sample.GameThreadP95Ms = 18.0f;
    Sample.RenderThreadP95Ms = 19.0f;
    Sample.GpuP95Ms = 25.0f;
    Sample.PeakDrawCalls = 620;
    Sample.PeakVisibleTriangles = 680000;
    Sample.PeakResidentTextureMB = 470;
    Sample.PeakActiveVfx = 8;

    const FWMVisualCertificationVerdict Verdict = FWMVisualCertificationEvaluator::Evaluate(Sample, Budget);
    TestTrue(TEXT("Measured sample passes all ceilings"), Verdict.bWithinBudget);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMVisualCertificationInsufficientSamplesTest,
    "WorldMakers.Visual.Certification.FrameSampleFloorFailsClosed",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMVisualCertificationInsufficientSamplesTest::RunTest(const FString& Parameters)
{
    FWMVisualCertificationBudget Budget;
    FWMVisualCertificationSample Sample;
    Sample.FrameSamples = 1799;
    Sample.P95FrameTimeMs = 20.0f;
    Sample.GameThreadP95Ms = 10.0f;
    Sample.RenderThreadP95Ms = 10.0f;
    Sample.GpuP95Ms = 10.0f;
    Sample.PeakDrawCalls = 100;
    Sample.PeakVisibleTriangles = 100000;
    Sample.PeakResidentTextureMB = 128;
    Sample.PeakActiveVfx = 1;

    const FWMVisualCertificationVerdict Verdict = FWMVisualCertificationEvaluator::Evaluate(Sample, Budget);
    TestFalse(TEXT("Too few samples cannot certify"), Verdict.bWithinBudget);
    TestFalse(TEXT("Sample floor is explicit"), Verdict.bEnoughFrameSamples);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMVisualCertificationSingleMetricFailureTest,
    "WorldMakers.Visual.Certification.AnyVisualBudgetFailureBlocksVerdict",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMVisualCertificationSingleMetricFailureTest::RunTest(const FString& Parameters)
{
    FWMVisualCertificationBudget Budget;
    FWMVisualCertificationSample Sample;
    Sample.FrameSamples = 2000;
    Sample.P95FrameTimeMs = 32.0f;
    Sample.GameThreadP95Ms = 20.0f;
    Sample.RenderThreadP95Ms = 20.0f;
    Sample.GpuP95Ms = 29.0f;
    Sample.PeakDrawCalls = Budget.MaxDrawCalls + 1;
    Sample.PeakVisibleTriangles = 700000;
    Sample.PeakResidentTextureMB = 500;
    Sample.PeakActiveVfx = 8;

    const FWMVisualCertificationVerdict Verdict = FWMVisualCertificationEvaluator::Evaluate(Sample, Budget);
    TestFalse(TEXT("One exceeded ceiling blocks certification"), Verdict.bWithinBudget);
    TestFalse(TEXT("Draw-call failure is visible"), Verdict.bDrawCallsWithinBudget);
    return true;
}

#endif
