#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Performance/WMPerformanceProfileTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMPerformanceProfileCatalogTest,
    "WorldMakers.Performance.Profiles.CatalogAndAllowlist",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMPerformanceProfileCatalogTest::RunTest(const FString& Parameters)
{
    const FString Path = FPaths::Combine(
        FPaths::ProjectContentDir(),
        TEXT("WorldMakers/Performance/tablet-performance-profiles.json"));
    FString Json;
    TestTrue(TEXT("Packaged performance profile catalog loads"), FFileHelper::LoadFileToString(Json, *Path));

    FWMPerformanceProfileCatalog Catalog;
    FString Error;
    TestTrue(TEXT("Performance profile catalog parses"), FWMPerformanceProfileCatalog::TryParseJson(Json, Catalog, Error));
    TestTrue(TEXT("Catalog is sane"), Catalog.IsSane());

    const FWMPerformanceProfileDefinition* Low = Catalog.FindProfile(FName(TEXT("performance.tablet.low")));
    const FWMPerformanceProfileDefinition* Medium = Catalog.FindProfile(FName(TEXT("performance.tablet.medium")));
    const FWMPerformanceProfileDefinition* High = Catalog.FindProfile(FName(TEXT("performance.tablet.high")));
    TestNotNull(TEXT("Tablet Low exists"), Low);
    TestNotNull(TEXT("Tablet Medium exists"), Medium);
    TestNotNull(TEXT("Tablet High exists"), High);

    if (Low && Medium && High)
    {
        TestEqual(TEXT("Tablet Low target"), Low->Budget.TargetFps, 30);
        TestEqual(TEXT("Tablet Medium target"), Medium->Budget.TargetFps, 30);
        TestEqual(TEXT("Tablet High target"), High->Budget.TargetFps, 60);
        TestTrue(TEXT("Higher tier increases screen percentage"), Low->Scalability.ScreenPercentage < Medium->Scalability.ScreenPercentage && Medium->Scalability.ScreenPercentage < High->Scalability.ScreenPercentage);

        const TMap<FString, FString> Assignments = Medium->Scalability.BuildAllowlistedCVarAssignments();
        TestEqual(TEXT("Allowlist has fixed size"), Assignments.Num(), 12);
        TestTrue(TEXT("Screen percentage CVar is allowlisted"), Assignments.Contains(FString(TEXT("r.ScreenPercentage"))));
        TestTrue(TEXT("Texture pool CVar is allowlisted"), Assignments.Contains(FString(TEXT("r.Streaming.PoolSize"))));
        TestFalse(TEXT("Arbitrary command cannot enter allowlist"), Assignments.Contains(FString(TEXT("wm.ArbitraryCommand"))));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMPerformanceCaptureBudgetTest,
    "WorldMakers.Performance.Capture.BudgetEvaluation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMPerformanceCaptureBudgetTest::RunTest(const FString& Parameters)
{
    FWMPerformanceBudget Budget;
    Budget.TargetFps = 30;
    Budget.FrameTimeBudgetMs = 33.34f;
    Budget.MaxWorldActors = 1000;
    Budget.MaxPlacedBuildPieces = 300;
    Budget.MaxActiveInteractables = 24;

    FWMPerformanceCaptureAccumulator Healthy;
    Healthy.AddFrameTimeMs(16.0f);
    Healthy.AddFrameTimeMs(20.0f);
    Healthy.AddFrameTimeMs(30.0f);
    Healthy.AddFrameTimeMs(31.0f);
    Healthy.ObserveStructuralCounts(700, 120, 12);
    const FWMPerformanceCaptureSummary HealthySummary = Healthy.BuildSummary(FName(TEXT("performance.tablet.medium")), Budget);
    TestTrue(TEXT("Healthy sample stays within budget"), HealthySummary.bWithinBudget);
    TestEqual(TEXT("Healthy sample count"), HealthySummary.FrameSampleCount, 4);
    TestEqual(TEXT("Structural maxima retained"), HealthySummary.MaxWorldActors, 700);

    FWMPerformanceCaptureAccumulator OverBudget;
    OverBudget.AddFrameTimeMs(16.0f);
    OverBudget.AddFrameTimeMs(17.0f);
    OverBudget.AddFrameTimeMs(45.0f);
    OverBudget.AddFrameTimeMs(50.0f);
    OverBudget.ObserveStructuralCounts(1200, 350, 31);
    const FWMPerformanceCaptureSummary OverSummary = OverBudget.BuildSummary(FName(TEXT("performance.tablet.medium")), Budget);
    TestFalse(TEXT("Over-budget sample fails overall evaluation"), OverSummary.bWithinBudget);
    TestFalse(TEXT("Frame time failure is explicit"), OverSummary.bFrameTimeWithinBudget);
    TestFalse(TEXT("Actor failure is explicit"), OverSummary.bActorCountWithinBudget);
    TestFalse(TEXT("Build-piece failure is explicit"), OverSummary.bBuildPieceCountWithinBudget);
    TestFalse(TEXT("Interactable failure is explicit"), OverSummary.bInteractableCountWithinBudget);
    return true;
}

#endif
