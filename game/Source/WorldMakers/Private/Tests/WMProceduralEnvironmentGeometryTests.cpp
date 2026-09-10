#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Visual/WMProceduralEnvironmentGeometry.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMEnvironmentGroundDeterminismTest,
    "WorldMakers.Visual.Environment.GroundIsDeterministic",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMEnvironmentGroundDeterminismTest::RunTest(const FString& Parameters)
{
    FWMEnvironmentMeshData First;
    FWMEnvironmentMeshData Second;
    FRandomStream FirstRandom(17062026);
    FRandomStream SecondRandom(17062026);

    FWMProceduralEnvironmentGeometry::AppendIrregularGroundDisc(First, FVector::ZeroVector, 2400.0f, 18, FirstRandom);
    FWMProceduralEnvironmentGeometry::AppendIrregularGroundDisc(Second, FVector::ZeroVector, 2400.0f, 18, SecondRandom);

    TestTrue(TEXT("First ground mesh is sane"), First.IsSane());
    TestTrue(TEXT("Second ground mesh is sane"), Second.IsSane());
    TestEqual(TEXT("Ground triangle count is deterministic"), First.NumTriangles(), Second.NumTriangles());
    TestEqual(TEXT("Ground has one triangle per rim segment"), First.NumTriangles(), 18);
    TestTrue(TEXT("Ground first vertex is deterministic"), First.Vertices[1].Equals(Second.Vertices[1], 0.001f));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMEnvironmentTreeSilhouetteTest,
    "WorldMakers.Visual.Environment.TreeHasTrunkRootsAndCanopy",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMEnvironmentTreeSilhouetteTest::RunTest(const FString& Parameters)
{
    FWMEnvironmentMeshData Bark;
    FWMEnvironmentMeshData Foliage;

    FWMProceduralEnvironmentGeometry::AppendTaperedTrunk(Bark, FVector::ZeroVector, FVector(18.0f, -9.0f, 420.0f), 42.0f, 20.0f, 6, 12.0f);
    FWMProceduralEnvironmentGeometry::AppendButtressRoot(Bark, FVector::ZeroVector, FVector::ForwardVector, 130.0f, 46.0f, 80.0f);
    FWMProceduralEnvironmentGeometry::AppendFacetedEllipsoid(Foliage, FVector(18.0f, -9.0f, 475.0f), FVector(150.0f, 125.0f, 82.0f), 8, 12.0f);

    TestTrue(TEXT("Bark geometry is sane"), Bark.IsSane());
    TestTrue(TEXT("Foliage geometry is sane"), Foliage.IsSane());
    TestTrue(TEXT("Tree trunk and roots produce substantial faceting"), Bark.NumTriangles() >= 20);
    TestTrue(TEXT("Canopy produces a faceted closed volume"), Foliage.NumTriangles() >= 24);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMEnvironmentUnderstoryTest,
    "WorldMakers.Visual.Environment.UnderstoryUsesReadableLeafStar",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMEnvironmentUnderstoryTest::RunTest(const FString& Parameters)
{
    FWMEnvironmentMeshData Leaves;
    FWMProceduralEnvironmentGeometry::AppendLeafCluster(Leaves, FVector(20.0f, 40.0f, 0.0f), 85.0f, 5, 18.0f);

    TestTrue(TEXT("Leaf cluster is sane"), Leaves.IsSane());
    TestEqual(TEXT("Each leaf contributes two triangles"), Leaves.NumTriangles(), 10);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMEnvironmentRiverTest,
    "WorldMakers.Visual.Environment.RiverIsContinuousStrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMEnvironmentRiverTest::RunTest(const FString& Parameters)
{
    FWMEnvironmentMeshData River;
    FWMProceduralEnvironmentGeometry::AppendSinuousRiverStrip(River, 2600.0f, -2700.0f, -36.0f, 185.0f, 16, 118.0f, 1.35f);

    TestTrue(TEXT("River geometry is sane"), River.IsSane());
    TestEqual(TEXT("Each river segment contributes two triangles"), River.NumTriangles(), 32);
    TestTrue(TEXT("River has visible lateral extent"), FMath::Abs(River.Vertices[0].Y - River.Vertices[2].Y) > 200.0f);
    return true;
}

#endif
