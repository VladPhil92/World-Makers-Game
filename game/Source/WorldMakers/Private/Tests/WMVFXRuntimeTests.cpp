#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Science/WMScienceVFXAdapter.h"
#include "Visual/WMProceduralVFXGeometry.h"
#include "Visual/WMVFXRuntime.h"

namespace
{
    FWMVFXEvent MakeEvent(const FName Id)
    {
        FWMVFXEvent Event;
        Event.EventId = Id;
        Event.LocationCm = FVector(100.0f, 20.0f, 50.0f);
        Event.Direction = FVector::ForwardVector;
        Event.Intensity = 1.0f;
        Event.DurationSeconds = 0.65f;
        Event.MotionScale = 1.0f;
        return Event;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMVFXSemanticStyleTest,
    "WorldMakers.Visual.VFX.SemanticStylesAreStable",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMVFXSemanticStyleTest::RunTest(const FString& Parameters)
{
    FWMVFXStyle Style;
    TestTrue(TEXT("Build place resolves"), FWMVFXRuntime::ResolveStyle(TEXT("gameplay.build.place"), Style));
    TestEqual(TEXT("Build place is ring"), static_cast<uint8>(Style.Shape), static_cast<uint8>(EWMVFXShape::Ring));
    TestEqual(TEXT("Reaction domain is chemistry"), static_cast<uint8>(FWMVFXRuntime::ResolveDomain(TEXT("science.chemistry.reaction"))), static_cast<uint8>(EWMVFXDomain::Chemistry));
    TestEqual(TEXT("Portal domain is fantasy"), static_cast<uint8>(FWMVFXRuntime::ResolveDomain(TEXT("fantasy.portal.open"))), static_cast<uint8>(EWMVFXDomain::Fantasy));
    TestFalse(TEXT("Unknown semantics are rejected"), FWMVFXRuntime::ResolveStyle(TEXT("unknown.effect"), Style));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMVFXBudgetReducedMotionTest,
    "WorldMakers.Visual.VFX.BudgetsAndReducedMotionAreBounded",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMVFXBudgetReducedMotionTest::RunTest(const FString& Parameters)
{
    FWMVFXRuntime Runtime;
    FWMVFXBudget Budget;
    Budget.MaxActiveProxyEffects = 2;
    Budget.MaxEventsPerSecond = 2;
    Budget.MaxDurationSeconds = 1.0f;
    Budget.MaxIntensity = 0.8f;

    FWMVFXEvent First = MakeEvent(TEXT("gameplay.build.place"));
    TestTrue(TEXT("First event accepted"), Runtime.TryAccept(First, 0.0, 0, Budget, true));
    TestTrue(TEXT("Reduced motion clamps travel multiplier"), First.MotionScale <= 0.35f);
    TestTrue(TEXT("Tier clamps intensity"), First.Intensity <= 0.8f);

    FWMVFXEvent Second = MakeEvent(TEXT("gameplay.build.remove"));
    TestTrue(TEXT("Second event accepted"), Runtime.TryAccept(Second, 0.2, 1, Budget, false));
    FWMVFXEvent Third = MakeEvent(TEXT("fantasy.rune.activate"));
    TestFalse(TEXT("Rate limiter rejects third event in same second"), Runtime.TryAccept(Third, 0.3, 1, Budget, false));

    FWMVFXEvent NextWindow = MakeEvent(TEXT("fantasy.rune.activate"));
    TestTrue(TEXT("New time window accepts event"), Runtime.TryAccept(NextWindow, 1.1, 0, Budget, false));
    FWMVFXEvent AtCapacity = MakeEvent(TEXT("world.observe.reveal"));
    TestFalse(TEXT("Active proxy ceiling rejects event"), Runtime.TryAccept(AtCapacity, 1.2, 2, Budget, false));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMVFXProceduralGeometryTest,
    "WorldMakers.Visual.VFX.ProceduralFallbackGeometryIsValid",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMVFXProceduralGeometryTest::RunTest(const FString& Parameters)
{
    FWMVFXMeshData Mesh;
    FWMProceduralVFXGeometry::BuildRing(Mesh, 12);
    TestTrue(TEXT("Ring geometry is sane"), Mesh.IsSane());
    const int32 RingTriangles = Mesh.Triangles.Num() / 3;

    FWMProceduralVFXGeometry::BuildBurst(Mesh, 8);
    TestTrue(TEXT("Burst geometry is sane"), Mesh.IsSane());
    TestTrue(TEXT("Burst has non-trivial triangles"), Mesh.Triangles.Num() / 3 >= 8);

    FWMProceduralVFXGeometry::BuildDirectionalChevron(Mesh);
    TestTrue(TEXT("Directional geometry is sane"), Mesh.IsSane());
    TestTrue(TEXT("Ring remains richer than chevron"), RingTriangles > Mesh.Triangles.Num() / 3);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMVFXScienceCausalityTest,
    "WorldMakers.Visual.VFX.ScienceResultsDriveSemanticsAndMagnitude",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMVFXScienceCausalityTest::RunTest(const FString& Parameters)
{
    FWMDissolutionResult Dissolution;
    Dissolution.bAccepted = true;
    Dissolution.bSaturated = true;
    Dissolution.DissolvedMassG = 35.9f;
    Dissolution.UndissolvedMassG = 14.1f;
    FWMVFXEvent Event;
    TestTrue(TEXT("Accepted dissolution maps to VFX"), FWMScienceVFXAdapter::FromDissolution(Dissolution, FVector::ZeroVector, Event));
    TestEqual(TEXT("Saturation retains scientific meaning"), Event.EventId, FName(TEXT("science.chemistry.saturation")));

    FWMReactionResult SmallReaction;
    SmallReaction.bAccepted = true;
    SmallReaction.ReactionExtentMol = 0.25f;
    FWMVFXEvent SmallEvent;
    TestTrue(TEXT("Small reaction maps"), FWMScienceVFXAdapter::FromReaction(SmallReaction, FVector::ZeroVector, SmallEvent));

    FWMReactionResult LargeReaction = SmallReaction;
    LargeReaction.ReactionExtentMol = 1.5f;
    FWMVFXEvent LargeEvent;
    TestTrue(TEXT("Large reaction maps"), FWMScienceVFXAdapter::FromReaction(LargeReaction, FVector::ZeroVector, LargeEvent));
    TestTrue(TEXT("Reaction magnitude affects visual intensity"), LargeEvent.Intensity > SmallEvent.Intensity);

    FWMVFXEvent ForceEvent;
    TestTrue(TEXT("Force vector maps"), FWMScienceVFXAdapter::FromForce(FVector(0.0f, 20.0f, 0.0f), FVector::ZeroVector, 40.0f, ForceEvent));
    TestTrue(TEXT("Force direction remains causal"), ForceEvent.Direction.Equals(FVector::RightVector, KINDA_SMALL_NUMBER));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMVFXBiologyEcologyCausalityTest,
    "WorldMakers.Visual.VFX.BiologyAndEcologyFeedbackRemainCausal",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMVFXBiologyEcologyCausalityTest::RunTest(const FString& Parameters)
{
    FWMCellStepResult Cell;
    Cell.bAccepted = true;
    Cell.EnergyProducedUnits = 0.45f;
    FWMVFXEvent Event;
    TestTrue(TEXT("Cell energy maps"), FWMScienceVFXAdapter::FromCell(Cell, FVector::ZeroVector, Event));
    TestEqual(TEXT("Cell semantic ID"), Event.EventId, FName(TEXT("science.biology.cell-energy")));

    FWMPlantStepResult Plant;
    Plant.bAccepted = true;
    Plant.GrowthUnits = 0.02f;
    TestTrue(TEXT("Plant growth maps"), FWMScienceVFXAdapter::FromPlant(Plant, FVector::ZeroVector, Event));
    TestEqual(TEXT("Plant semantic ID"), Event.EventId, FName(TEXT("science.biology.plant-growth")));

    FWMEnvironmentStateDelta Recovery;
    Recovery.VegetationHealth = 0.1f;
    Recovery.SoilProtection = 0.1f;
    TestTrue(TEXT("Positive ecology delta maps"), FWMScienceVFXAdapter::FromEnvironmentDelta(Recovery, FVector::ZeroVector, Event));
    TestEqual(TEXT("Positive ecology is recovery"), Event.EventId, FName(TEXT("science.ecology.recovery")));

    FWMEnvironmentStateDelta Stress;
    Stress.WaterFlow = -0.2f;
    TestTrue(TEXT("Negative ecology delta maps"), FWMScienceVFXAdapter::FromEnvironmentDelta(Stress, FVector::ZeroVector, Event));
    TestEqual(TEXT("Negative ecology is stress"), Event.EventId, FName(TEXT("science.ecology.stress")));
    return true;
}

#endif
