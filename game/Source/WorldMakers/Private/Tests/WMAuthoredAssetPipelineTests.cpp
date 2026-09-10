#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Visual/WMAuthoredAssetTypes.h"

namespace WMAuthoredAssetTests
{
    FWMAuthoredVisualAssetDefinition MakeAsset(const FString& Suffix, const FName Kind = TEXT("StaticMesh"))
    {
        FWMAuthoredVisualAssetDefinition Asset;
        Asset.AssetId = FName(*FString::Printf(TEXT("test.asset.%s"), *Suffix));
        Asset.AssetKind = Kind;
        Asset.ObjectPath = FString::Printf(TEXT("/Game/WorldMakers/Test/SM_WM_Test_%s.SM_WM_Test_%s"), *Suffix, *Suffix);
        Asset.SourcePath = FString::Printf(TEXT("SourceArt/WorldMakers/Test/SM_WM_Test_%s.fbx"), *Suffix);
        Asset.CollisionPolicy = TEXT("proxy");
        Asset.FallbackMode = TEXT("procedural");
        Asset.MinLods = 3;
        Asset.MaxMaterialSlots = 2;
        Asset.bAuthoredPresent = false;
        return Asset;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMAuthoredAssetDefinitionTest,
    "WorldMakers.Visual.AuthoredAssets.DefinitionGuardsNamespaceAndFallback",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMAuthoredAssetDefinitionTest::RunTest(const FString& Parameters)
{
    FWMAuthoredVisualAssetDefinition Asset = WMAuthoredAssetTests::MakeAsset(TEXT("A"));
    TestTrue(TEXT("World Makers authored definition is sane"), Asset.IsSane());
    TestFalse(TEXT("Unproduced asset cannot be loaded"), Asset.CanAttemptLoad());

    Asset.bAuthoredPresent = true;
    TestTrue(TEXT("Produced sane asset becomes loadable"), Asset.CanAttemptLoad());

    Asset.ObjectPath = TEXT("/Engine/BasicShapes/Cube.Cube");
    TestFalse(TEXT("Engine placeholder cannot masquerade as authored art"), Asset.IsSane());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMAuthoredAssetCatalogTest,
    "WorldMakers.Visual.AuthoredAssets.CatalogRequiresUniqueStableIds",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMAuthoredAssetCatalogTest::RunTest(const FString& Parameters)
{
    FWMAuthoredVisualAssetCatalog Catalog;
    Catalog.SchemaVersion = 1;
    Catalog.Units = TEXT("centimeters");
    Catalog.UpAxis = TEXT("Z");
    Catalog.ForwardAxis = TEXT("X");
    for (int32 Index = 0; Index < 8; ++Index)
    {
        Catalog.Assets.Add(WMAuthoredAssetTests::MakeAsset(FString::FromInt(Index)));
    }

    TestTrue(TEXT("Eight unique assets satisfy catalog floor"), Catalog.IsSane());
    TestNotNull(TEXT("Stable ID lookup succeeds"), Catalog.FindAsset(TEXT("test.asset.3")));

    Catalog.Assets[7].AssetId = Catalog.Assets[0].AssetId;
    TestFalse(TEXT("Duplicate stable IDs invalidate catalog"), Catalog.IsSane());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMAuthoredAssetBudgetTest,
    "WorldMakers.Visual.AuthoredAssets.BudgetsStayTabletBounded",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMAuthoredAssetBudgetTest::RunTest(const FString& Parameters)
{
    FWMAuthoredVisualAssetDefinition Asset = WMAuthoredAssetTests::MakeAsset(TEXT("Budget"));
    Asset.MinLods = 8;
    Asset.MaxMaterialSlots = 4;
    TestTrue(TEXT("Upper supported production limits remain valid"), Asset.IsSane());

    Asset.MaxMaterialSlots = 5;
    TestFalse(TEXT("More than four material slots are rejected"), Asset.IsSane());
    Asset.MaxMaterialSlots = 2;
    Asset.MinLods = 0;
    TestFalse(TEXT("Assets must define at least one LOD"), Asset.IsSane());
    return true;
}

#endif
