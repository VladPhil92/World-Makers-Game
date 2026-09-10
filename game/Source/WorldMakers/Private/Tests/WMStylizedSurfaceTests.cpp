#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Visual/WMStylizedSurfaceLibrary.h"
#include "Visual/WMVisualProfileSettings.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMStylizedSurfaceSemanticsTest,
    "WorldMakers.Visual.Surface.Semantics",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMStylizedSurfaceSemanticsTest::RunTest(const FString& Parameters)
{
    const UWMVisualProfileSettings* Profile = GetDefault<UWMVisualProfileSettings>();
    TestNotNull(TEXT("Visual profile exists"), Profile);
    if (!Profile)
    {
        return false;
    }

    TestTrue(TEXT("Surface response is sane"), Profile->SurfaceResponse.IsSane());

    const FWMStylizedSurfaceLook Water = UWMStylizedSurfaceLibrary::ResolveLook(*Profile, EWMStylizedSurfaceRole::Water);
    const FWMStylizedSurfaceLook Stone = UWMStylizedSurfaceLibrary::ResolveLook(*Profile, EWMStylizedSurfaceRole::Stone);
    const FWMStylizedSurfaceLook Foliage = UWMStylizedSurfaceLibrary::ResolveLook(*Profile, EWMStylizedSurfaceRole::Foliage);
    const FWMStylizedSurfaceLook Bark = UWMStylizedSurfaceLibrary::ResolveLook(*Profile, EWMStylizedSurfaceRole::Bark);
    const FWMStylizedSurfaceLook Preview = UWMStylizedSurfaceLibrary::ResolveLook(*Profile, EWMStylizedSurfaceRole::PreviewValid);
    const FWMStylizedSurfaceLook Build = UWMStylizedSurfaceLibrary::ResolveLook(*Profile, EWMStylizedSurfaceRole::BuildNeutral);

    TestTrue(TEXT("Resolved looks remain sane"), Water.IsSane() && Stone.IsSane() && Foliage.IsSane() && Bark.IsSane() && Preview.IsSane() && Build.IsSane());
    TestTrue(TEXT("Water reads smoother than stone"), Water.Roughness < Stone.Roughness);
    TestTrue(TEXT("Water preserves translucent intent"), Water.bTranslucentIntent && Water.OpacityIntent < 1.0f);
    TestTrue(TEXT("Foliage carries wind response"), Foliage.WindResponse > Bark.WindResponse);
    TestTrue(TEXT("Foliage carries two-sided intent"), Foliage.bTwoSidedIntent);
    TestTrue(TEXT("Preview has stronger emissive intent than placed build"), Preview.EmissiveStrength > Build.EmissiveStrength);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMStylizedSurfaceRoleIdentityTest,
    "WorldMakers.Visual.Surface.StableRoleIdentity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMStylizedSurfaceRoleIdentityTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("Water role string is stable"), UWMStylizedSurfaceLibrary::SurfaceRoleToString(EWMStylizedSurfaceRole::Water), FString(TEXT("water")));
    TestEqual(TEXT("Eco build role string is stable"), UWMStylizedSurfaceLibrary::SurfaceRoleToString(EWMStylizedSurfaceRole::BuildEco), FString(TEXT("build-eco")));
    TestEqual(TEXT("Invalid preview role string is stable"), UWMStylizedSurfaceLibrary::SurfaceRoleToString(EWMStylizedSurfaceRole::PreviewInvalid), FString(TEXT("preview-invalid")));
    TestEqual(TEXT("Magical accent role string is stable"), UWMStylizedSurfaceLibrary::SurfaceRoleToString(EWMStylizedSurfaceRole::MagicalAccent), FString(TEXT("magical-accent")));
    return true;
}

#endif
