#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Visual/WMAvatarArtTypes.h"
#include "Visual/WMProceduralAvatarGeometry.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMAvatarProportionsTest,
    "WorldMakers.Visual.Avatar.ProportionsAreChildReadable",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMAvatarProportionsTest::RunTest(const FString& Parameters)
{
    const UWMAvatarVisualSettings* Settings = GetDefault<UWMAvatarVisualSettings>();
    TestNotNull(TEXT("Avatar visual settings exist"), Settings);
    if (!Settings)
    {
        return false;
    }

    TestTrue(TEXT("Avatar proportions are sane"), Settings->Proportions.IsSane());
    TestTrue(TEXT("Avatar is approximately five heads tall"), FMath::IsNearlyEqual(Settings->Proportions.GetHeadsTall(), 5.0f, 0.05f));
    TestTrue(TEXT("Shoulders remain wider than hips"), Settings->Proportions.ShoulderWidthCm > Settings->Proportions.HipWidthCm);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMAvatarRigContractTest,
    "WorldMakers.Visual.Avatar.RigContractIsStable",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMAvatarRigContractTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("Rig contract is sane"), FWMAvatarRigContract::IsSane());
    TestEqual(TEXT("Rig exposes nineteen stable joints"), FWMAvatarRigContract::GetJoints().Num(), 19);
    TestEqual(TEXT("Head follows neck"), FWMAvatarRigContract::GetParentJoint(FName(TEXT("head"))), FName(TEXT("neck")));
    TestEqual(TEXT("Left hand follows lower arm"), FWMAvatarRigContract::GetParentJoint(FName(TEXT("hand_l"))), FName(TEXT("lowerarm_l")));
    TestEqual(TEXT("Right foot follows calf"), FWMAvatarRigContract::GetParentJoint(FName(TEXT("foot_r"))), FName(TEXT("calf_r")));
    TestTrue(TEXT("Customization includes hair"), FWMAvatarRigContract::GetCustomizationSlots().Contains(FName(TEXT("hair"))));
    TestTrue(TEXT("Customization includes hand prop"), FWMAvatarRigContract::GetCustomizationSlots().Contains(FName(TEXT("hand-prop"))));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMAvatarProceduralGeometryTest,
    "WorldMakers.Visual.Avatar.ProceduralGeometryFitsBudgets",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMAvatarProceduralGeometryTest::RunTest(const FString& Parameters)
{
    const UWMAvatarVisualSettings* Settings = GetDefault<UWMAvatarVisualSettings>();
    TestNotNull(TEXT("Avatar visual settings exist"), Settings);
    if (!Settings)
    {
        return false;
    }

    const TArray<EWMVisualQualityTier> Tiers = {
        EWMVisualQualityTier::Low,
        EWMVisualQualityTier::Mid,
        EWMVisualQualityTier::High,
    };
    for (const EWMVisualQualityTier Tier : Tiers)
    {
        const FWMAvatarArtBudget& Budget = Settings->GetBudget(Tier);
        TestTrue(TEXT("Avatar budget is sane"), Budget.IsSane());
        const int32 Estimate = FWMProceduralAvatarGeometry::EstimateTriangleCount(Settings->Proportions, Budget);
        TestTrue(TEXT("Procedural source avatar stays under authored triangle ceiling"), Estimate > 0 && Estimate < Budget.MaxTriangles);
    }

    FWMEnvironmentMeshData Torso;
    FWMProceduralAvatarGeometry::BuildTorso(Torso, Settings->Proportions);
    TestTrue(TEXT("Torso geometry is sane"), Torso.IsSane());

    FWMEnvironmentMeshData Head;
    FWMProceduralAvatarGeometry::BuildHead(Head, Settings->Proportions, Settings->MidBudget.HeadSegments);
    TestTrue(TEXT("Head geometry is sane"), Head.IsSane());

    FWMEnvironmentMeshData Hair;
    FWMProceduralAvatarGeometry::BuildHairCap(Hair, Settings->Proportions, Settings->MidBudget.HeadSegments);
    TestTrue(TEXT("Hair geometry is sane"), Hair.IsSane());
    TestTrue(TEXT("Hair has richer silhouette than one head shell"), Hair.NumTriangles() > Head.NumTriangles());
    return true;
}

#endif
