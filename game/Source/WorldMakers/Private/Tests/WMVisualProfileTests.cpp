#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Visual/WMVisualProfileSettings.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMVisualProfileBudgetTest,
    "WorldMakers.Visual.Profile.Budgets",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMVisualProfileBudgetTest::RunTest(const FString& Parameters)
{
    const UWMVisualProfileSettings* Profile = GetDefault<UWMVisualProfileSettings>();
    TestNotNull(TEXT("Visual profile settings exist"), Profile);
    if (!Profile)
    {
        return false;
    }

    const FWMVisualBudget& Low = Profile->GetBudget(EWMVisualQualityTier::Low);
    const FWMVisualBudget& Mid = Profile->GetBudget(EWMVisualQualityTier::Mid);
    const FWMVisualBudget& High = Profile->GetBudget(EWMVisualQualityTier::High);

    TestTrue(TEXT("Low budget is sane"), Low.IsSane());
    TestTrue(TEXT("Mid budget is sane"), Mid.IsSane());
    TestTrue(TEXT("High budget is sane"), High.IsSane());
    TestTrue(TEXT("Tree density scales monotonically"), Low.TreeClusters <= Mid.TreeClusters && Mid.TreeClusters <= High.TreeClusters);
    TestTrue(TEXT("Rock density scales monotonically"), Low.RockClusters <= Mid.RockClusters && Mid.RockClusters <= High.RockClusters);
    TestTrue(TEXT("Atmosphere look is sane"), Profile->Atmosphere.IsSane());
    TestTrue(TEXT("Camera FOV stays tablet-readable"), Profile->Atmosphere.CameraFOVDegrees >= 60.0f && Profile->Atmosphere.CameraFOVDegrees <= 90.0f);
    TestTrue(TEXT("Fog remains restrained for gameplay readability"), Profile->Atmosphere.FogDensity <= 0.05f);
    TestEqual(TEXT("Default profile name is explicit"), Profile->ProfileName, FString(TEXT("CaribbeanRainforestPrototype")));
    return true;
}

#endif
