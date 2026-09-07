#if WITH_DEV_AUTOMATION_TESTS

#include "Building/WMBuildCatalogSettings.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMBuildPrototypeCatalogTest,
    "WorldMakers.Building.Catalog.PrototypePieces",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMBuildPrototypeCatalogTest::RunTest(const FString& Parameters)
{
    const UWMBuildCatalogSettings* Catalog = GetDefault<UWMBuildCatalogSettings>();
    TestNotNull(TEXT("Build catalog settings load"), Catalog);
    if (!Catalog)
    {
        return false;
    }

    const FName RequiredIds[] = {
        TEXT("prototype.cube"),
        TEXT("prototype.floor"),
        TEXT("prototype.wall"),
        TEXT("prototype.pillar")
    };

    for (const FName PieceId : RequiredIds)
    {
        FWMBuildPieceSpec Spec;
        TestTrue(*FString::Printf(TEXT("Catalog resolves %s"), *PieceId.ToString()), Catalog->FindPieceSpec(PieceId, Spec));
        TestTrue(*FString::Printf(TEXT("Catalog spec %s is sane"), *PieceId.ToString()), Spec.IsSane());
    }

    FWMBuildPieceSpec Unknown;
    TestFalse(TEXT("Unknown catalog ids are rejected"), Catalog->FindPieceSpec(TEXT("prototype.unknown"), Unknown));
    return true;
}

#endif
