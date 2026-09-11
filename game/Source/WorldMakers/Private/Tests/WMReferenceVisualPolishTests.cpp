#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Visual/WMReferenceVisualPolishRuntime.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMReferenceVisualPolishFirstPersonModesTest,
    "WorldMakers.Visual.ReferencePolish.FirstPersonModesRemainReadable",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMReferenceVisualPolishFirstPersonModesTest::RunTest(const FString& Parameters)
{
    for (const EWMFirstPersonVisualMode Mode : {
        EWMFirstPersonVisualMode::Explore,
        EWMFirstPersonVisualMode::Build,
        EWMFirstPersonVisualMode::Scan,
        EWMFirstPersonVisualMode::Measure,
        EWMFirstPersonVisualMode::Observe})
    {
        FWMFirstPersonVisualProfile Profile;
        TestTrue(TEXT("Every first-person mode resolves"), FWMReferenceVisualPolishRuntime::ResolveFirstPersonProfile(Mode, false, Profile));
        TestTrue(TEXT("Every first-person mode stays bounded"), Profile.IsSane());
        TestTrue(TEXT("Large HUD panels stay capped"), Profile.MaxLargePanels <= 2);
    }

    FWMFirstPersonVisualProfile Explore;
    FWMFirstPersonVisualProfile Build;
    FWMReferenceVisualPolishRuntime::ResolveFirstPersonProfile(EWMFirstPersonVisualMode::Explore, false, Explore);
    FWMReferenceVisualPolishRuntime::ResolveFirstPersonProfile(EWMFirstPersonVisualMode::Build, false, Build);
    TestFalse(TEXT("Explore has no permanent context panel"), Explore.bContextPanelVisible);
    TestFalse(TEXT("Explore has no build palette"), Explore.bBuildPaletteVisible);
    TestTrue(TEXT("Build exposes authored tool framing"), Build.bToolVisible);
    TestTrue(TEXT("Build alone exposes build palette"), Build.bBuildPaletteVisible);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMReferenceVisualPolishContextHUDTest,
    "WorldMakers.Visual.ReferencePolish.ContextHUDIsModeBound",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMReferenceVisualPolishContextHUDTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("Build palette belongs to build mode"),
        FWMReferenceVisualPolishRuntime::IsContextPanelAllowed(EWMFirstPersonVisualMode::Build, TEXT("hud.build-palette")));
    TestFalse(TEXT("Build palette is hidden during exploration"),
        FWMReferenceVisualPolishRuntime::IsContextPanelAllowed(EWMFirstPersonVisualMode::Explore, TEXT("hud.build-palette")));
    TestTrue(TEXT("Science panel is allowed while scanning"),
        FWMReferenceVisualPolishRuntime::IsContextPanelAllowed(EWMFirstPersonVisualMode::Scan, TEXT("hud.science-panel")));
    TestFalse(TEXT("Science panel is not baseline explore HUD"),
        FWMReferenceVisualPolishRuntime::IsContextPanelAllowed(EWMFirstPersonVisualMode::Explore, TEXT("hud.science-panel")));
    TestTrue(TEXT("Measurement readout is measure-only"),
        FWMReferenceVisualPolishRuntime::IsContextPanelAllowed(EWMFirstPersonVisualMode::Measure, TEXT("hud.measurement-readout")));
    TestFalse(TEXT("Measurement readout is not scan HUD"),
        FWMReferenceVisualPolishRuntime::IsContextPanelAllowed(EWMFirstPersonVisualMode::Scan, TEXT("hud.measurement-readout")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMReferenceVisualPolishReducedMotionTest,
    "WorldMakers.Visual.ReferencePolish.ReducedMotionRemovesFirstPersonLag",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMReferenceVisualPolishReducedMotionTest::RunTest(const FString& Parameters)
{
    FWMFirstPersonVisualProfile Normal;
    FWMFirstPersonVisualProfile Reduced;
    FWMReferenceVisualPolishRuntime::ResolveFirstPersonProfile(EWMFirstPersonVisualMode::Scan, false, Normal);
    FWMReferenceVisualPolishRuntime::ResolveFirstPersonProfile(EWMFirstPersonVisualMode::Scan, true, Reduced);
    TestTrue(TEXT("Normal scan may use bounded motion"), Normal.CameraBobCm > 0.0f && Normal.ToolLagDegrees > 0.0f);
    TestEqual(TEXT("Reduced motion removes camera bob"), Reduced.CameraBobCm, 0.0f);
    TestEqual(TEXT("Reduced motion removes tool lag"), Reduced.ToolLagDegrees, 0.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMReferenceVisualPolishMotionPersonalityTest,
    "WorldMakers.Visual.ReferencePolish.MotionPersonalityChangesPoseNotGameplay",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMReferenceVisualPolishMotionPersonalityTest::RunTest(const FString& Parameters)
{
    FWMExplorerMotionStyle Curious;
    FWMExplorerMotionStyle Inventor;
    FWMExplorerMotionStyle Guardian;
    FWMExplorerMotionStyle Knowledge;
    TestTrue(TEXT("Curious resolves"), FWMReferenceVisualPolishRuntime::ResolveMotionStyle(EWMExplorerMotionPersonality::CuriousExplorer, false, Curious));
    TestTrue(TEXT("Inventor resolves"), FWMReferenceVisualPolishRuntime::ResolveMotionStyle(EWMExplorerMotionPersonality::ScientistInventor, false, Inventor));
    TestTrue(TEXT("Guardian resolves"), FWMReferenceVisualPolishRuntime::ResolveMotionStyle(EWMExplorerMotionPersonality::NatureGuardian, false, Guardian));
    TestTrue(TEXT("Knowledge resolves"), FWMReferenceVisualPolishRuntime::ResolveMotionStyle(EWMExplorerMotionPersonality::KnowledgeExplorer, false, Knowledge));
    TestTrue(TEXT("Profiles are visually distinct"),
        Curious.ArmSwingScale != Inventor.ArmSwingScale && Guardian.SettleSeconds != Knowledge.SettleSeconds);

    FWMExplorerMotionStyle Reduced;
    FWMReferenceVisualPolishRuntime::ResolveMotionStyle(EWMExplorerMotionPersonality::CuriousExplorer, true, Reduced);
    TestEqual(TEXT("Reduced motion disables secondary motion"), Reduced.SecondaryMotionScale, 0.0f);
    TestTrue(TEXT("Reduced head response stays bounded"), Reduced.HeadLookScale <= 0.90f);
    return true;
}

#endif
