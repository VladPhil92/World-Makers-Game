#include "Misc/AutomationTest.h"
#include "Mission/WMMissionGeometryLibrary.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMMissionGeometryDistanceTest,
    "WorldMakers.Missions.Geometry.WorldDistance",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMMissionGeometryDistanceTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("Anchor distance is measured in Unreal centimeters"),
        UWMMissionGeometryLibrary::MeasureWorldDistanceCm(FVector(-150.0f, 0.0f, 60.0f), FVector(150.0f, 0.0f, 60.0f)),
        300.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMMissionGeometryContainmentTest,
    "WorldMakers.Missions.Geometry.ZoneContainment",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMMissionGeometryContainmentTest::RunTest(const FString& Parameters)
{
    const FTransform ZoneTransform(FRotator(0.0f, 90.0f, 0.0f), FVector(100.0f, 200.0f, 0.0f));
    const FVector Extent(250.0f, 300.0f, 250.0f);

    TestTrue(TEXT("Zone center is contained"),
        UWMMissionGeometryLibrary::IsWorldPointInsideBox(FVector(100.0f, 200.0f, 0.0f), ZoneTransform, Extent));
    TestFalse(TEXT("Far point is excluded"),
        UWMMissionGeometryLibrary::IsWorldPointInsideBox(FVector(1000.0f, 200.0f, 0.0f), ZoneTransform, Extent));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMMissionGeometryScopedSpanTest,
    "WorldMakers.Missions.Geometry.ScopedStructureSpan",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMMissionGeometryScopedSpanTest::RunTest(const FString& Parameters)
{
    const FTransform ZoneTransform = FTransform::Identity;
    const FVector ZoneExtent(250.0f, 300.0f, 250.0f);

    TArray<FWMMissionPieceGeometrySample> Pieces;

    FWMMissionPieceGeometrySample Left;
    Left.Center = FVector(-100.0f, 0.0f, 50.0f);
    Left.DimensionsCm = FVector(100.0f, 100.0f, 100.0f);
    Pieces.Add(Left);

    FWMMissionPieceGeometrySample Right;
    Right.Center = FVector(100.0f, 0.0f, 50.0f);
    Right.DimensionsCm = FVector(100.0f, 100.0f, 100.0f);
    Pieces.Add(Right);

    FWMMissionPieceGeometrySample Outside;
    Outside.Center = FVector(1000.0f, 0.0f, 50.0f);
    Outside.DimensionsCm = FVector(1000.0f, 1000.0f, 100.0f);
    Pieces.Add(Outside);

    TestEqual(TEXT("Only pieces whose centers are inside the zone contribute"),
        UWMMissionGeometryLibrary::CalculateScopedSpanAlongZoneX(Pieces, ZoneTransform, ZoneExtent),
        300.0f);

    TArray<FWMMissionPieceGeometrySample> RotatedPieces;
    FWMMissionPieceGeometrySample RotatedWall;
    RotatedWall.Center = FVector::ZeroVector;
    RotatedWall.DimensionsCm = FVector(200.0f, 25.0f, 200.0f);
    RotatedWall.YawDegrees = 90.0f;
    RotatedPieces.Add(RotatedWall);

    TestEqual(TEXT("Yaw projection contributes the correct width along mission-local X"),
        UWMMissionGeometryLibrary::CalculateScopedSpanAlongZoneX(RotatedPieces, ZoneTransform, ZoneExtent),
        25.0f);
    return true;
}
