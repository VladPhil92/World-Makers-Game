#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Visual/WMFirstPersonNativeActivationRuntime.h"

namespace
{
    FString ValidActivatedJson(const FString& CommitSha)
    {
        return FString::Printf(TEXT(R"JSON({
            "schemaVersion":1,
            "activationId":"visual.first-person-native-activation.v1",
            "status":"activated",
            "activated":true,
            "targetCommitSha":"%s",
            "nativeImportReportSha256":"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
            "reviewEvidenceSha256":"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb",
            "activationCandidateSha256":"cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc",
            "allFiveAssetsApproved":true,
            "allNineAnimationsApproved":true,
            "humanReviewApproved":true,
            "deviceReviewApproved":true
        })JSON"), *CommitSha);
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMFirstPersonNativeActivationBlockedByDefaultTest,
    "WorldMakers.Visual.FirstPersonNativeActivation.BlockedManifestFailsClosed",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMFirstPersonNativeActivationBlockedByDefaultTest::RunTest(const FString& Parameters)
{
    const FString Json = TEXT(R"JSON({
        "schemaVersion":1,
        "activationId":"visual.first-person-native-activation.v1",
        "status":"blocked",
        "activated":false,
        "targetCommitSha":"",
        "nativeImportReportSha256":"",
        "reviewEvidenceSha256":"",
        "activationCandidateSha256":"",
        "allFiveAssetsApproved":false,
        "allNineAnimationsApproved":false,
        "humanReviewApproved":false,
        "deviceReviewApproved":false
    })JSON");
    FWMFirstPersonNativeActivationState State;
    TestTrue(TEXT("Blocked manifest parses"), FWMFirstPersonNativeActivationRuntime::TryParseJson(Json, State));
    TestFalse(TEXT("Blocked manifest is not structurally valid"), State.IsStructurallyValid());
    TestFalse(TEXT("Blocked manifest denies production takeover"), State.AllowsProductionTakeover(TEXT("0123456789012345678901234567890123456789")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMFirstPersonNativeActivationExactCommitTest,
    "WorldMakers.Visual.FirstPersonNativeActivation.ExactCommitAllowsProductionTakeover",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMFirstPersonNativeActivationExactCommitTest::RunTest(const FString& Parameters)
{
    const FString Commit = TEXT("0123456789abcdef0123456789abcdef01234567");
    FWMFirstPersonNativeActivationState State;
    TestTrue(TEXT("Activated manifest parses"), FWMFirstPersonNativeActivationRuntime::TryParseJson(ValidActivatedJson(Commit), State));
    TestTrue(TEXT("Activated manifest is structurally valid"), State.IsStructurallyValid());
    TestTrue(TEXT("Exact commit is approved"), State.AllowsProductionTakeover(Commit));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMFirstPersonNativeActivationWrongCommitTest,
    "WorldMakers.Visual.FirstPersonNativeActivation.WrongCommitFailsClosed",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMFirstPersonNativeActivationWrongCommitTest::RunTest(const FString& Parameters)
{
    const FString Commit = TEXT("0123456789abcdef0123456789abcdef01234567");
    const FString Other = TEXT("fedcba9876543210fedcba9876543210fedcba98");
    FWMFirstPersonNativeActivationState State;
    TestTrue(TEXT("Activated manifest parses"), FWMFirstPersonNativeActivationRuntime::TryParseJson(ValidActivatedJson(Commit), State));
    TestFalse(TEXT("Different build commit is rejected"), State.AllowsProductionTakeover(Other));
    return true;
}

#endif
