#if WITH_DEV_AUTOMATION_TESTS

#include "Environment/WMBiomeTypes.h"
#include "Misc/AutomationTest.h"

namespace
{
    const TCHAR* TestBiomeJson = LR"JSON({
        "schemaVersion": 1,
        "id": "biome.test",
        "prototypeOnly": true,
        "originCm": [0, 0, 0],
        "zones": [
            {"id":"zone.test.center","kind":"build-clearing","centerCm":[0,0,0],"extentCm":[100,100,100],"buildAllowed":true,"missionIds":[]},
            {"id":"zone.test.east","kind":"exploration","centerCm":[400,0,0],"extentCm":[100,100,100],"buildAllowed":false,"missionIds":[]}
        ],
        "pointsOfInterest": [
            {"id":"poi.test.tree","zoneId":"zone.test.east","category":"tree","locationCm":[400,0,0],"discoveryRadiusCm":150,"discoveryId":"discovery.test.tree"}
        ]
    })JSON";
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMBiomeDefinitionParseTest,
    "WorldMakers.Exploration.Definition.ParsesRuntimeContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMBiomeDefinitionParseTest::RunTest(const FString& Parameters)
{
    FWMBiomeRuntimeDefinition Definition;
    FString Error;
    TestTrue(TEXT("Biome runtime JSON parses"), FWMBiomeRuntimeDefinition::TryParseJson(TestBiomeJson, Definition, Error));
    TestEqual(TEXT("Stable biome ID preserved"), Definition.BiomeId, FName(TEXT("biome.test")));
    TestEqual(TEXT("Two semantic zones loaded"), Definition.Zones.Num(), 2);
    TestEqual(TEXT("One POI loaded"), Definition.PointsOfInterest.Num(), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMBiomeZoneContainmentTest,
    "WorldMakers.Exploration.Geometry.ZoneContainment",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMBiomeZoneContainmentTest::RunTest(const FString& Parameters)
{
    FWMBiomeRuntimeDefinition Definition;
    FString Error;
    if (!FWMBiomeRuntimeDefinition::TryParseJson(TestBiomeJson, Definition, Error))
    {
        AddError(Error);
        return false;
    }

    const FWMBiomeZoneDefinition* CenterZone = Definition.FindZoneAtWorldLocation(FVector(50.0f, 0.0f, 0.0f));
    const FWMBiomeZoneDefinition* EastZone = Definition.FindZoneAtWorldLocation(FVector(400.0f, 0.0f, 0.0f));
    const FWMBiomeZoneDefinition* Outside = Definition.FindZoneAtWorldLocation(FVector(900.0f, 900.0f, 0.0f));

    TestTrue(TEXT("Center zone resolves"), CenterZone && CenterZone->ZoneId == FName(TEXT("zone.test.center")));
    TestTrue(TEXT("East zone resolves"), EastZone && EastZone->ZoneId == FName(TEXT("zone.test.east")));
    TestNull(TEXT("Outside position resolves no zone"), Outside);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMExplorationDiscoveryIdempotencyTest,
    "WorldMakers.Exploration.Discovery.IdempotentStableIds",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMExplorationDiscoveryIdempotencyTest::RunTest(const FString& Parameters)
{
    FWMExplorationProgressModel Progress;
    const FName DiscoveryId(TEXT("discovery.test.tree"));
    TestTrue(TEXT("First discovery registers"), Progress.RegisterDiscovery(DiscoveryId));
    TestFalse(TEXT("Duplicate discovery does not register twice"), Progress.RegisterDiscovery(DiscoveryId));
    TestTrue(TEXT("Discovery can be queried by stable ID"), Progress.HasDiscovered(DiscoveryId));
    TestEqual(TEXT("Only one discovery stored"), Progress.GetDiscoveredIds().Num(), 1);
    return true;
}

#endif
