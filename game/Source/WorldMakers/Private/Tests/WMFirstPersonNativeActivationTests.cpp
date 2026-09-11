#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Visual/WMFirstPersonNativeActivationRuntime.h"

namespace
{
    FString ValidActivatedJson(const FString& ReviewedSourceSha)
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
        })JSON"), *ReviewedSourceSha);
    }

    FString ValidProvenanceJson(const FString& BuildSha, const FString& ReviewedSourceSha)
    {
        return FString::Printf(TEXT(R"JSON({
            "schemaVersion":1,
            "provenanceId":"visual.first-person-build-provenance.v1",
            "status":"bound",
            "buildCommitSha":"%s",
            "reviewedSourceCommitSha":"%s",
            "activationCandidateSha256":"cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc",
            "activationManifestSha256":"dddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd"
        })JSON"), *BuildSha, *ReviewedSourceSha);
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
    FWMFirstPersonBuildProvenance Provenance;
    TestTrue(TEXT("Blocked manifest parses"), FWMFirstPersonNativeActivationRuntime::TryParseJson(Json, State));
    TestTrue(TEXT("Bound provenance parses"), FWMFirstPersonNativeActivationRuntime::TryParseBuildProvenanceJson(
        ValidProvenanceJson(TEXT("1111111111111111111111111111111111111111"), TEXT("0123456789abcdef0123456789abcdef01234567")), Provenance));
    TestFalse(TEXT("Blocked manifest is not structurally valid"), State.IsStructurallyValid());
    TestFalse(TEXT("Blocked manifest denies production takeover"), State.AllowsProductionTakeover(Provenance));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMFirstPersonNativeActivationExactCommitTest,
    "WorldMakers.Visual.FirstPersonNativeActivation.ExactCommitAllowsProductionTakeover",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMFirstPersonNativeActivationExactCommitTest::RunTest(const FString& Parameters)
{
    const FString Reviewed = TEXT("0123456789abcdef0123456789abcdef01234567");
    const FString Build = TEXT("1111111111111111111111111111111111111111");
    FWMFirstPersonNativeActivationState State;
    FWMFirstPersonBuildProvenance Provenance;
    TestTrue(TEXT("Activated manifest parses"), FWMFirstPersonNativeActivationRuntime::TryParseJson(ValidActivatedJson(Reviewed), State));
    TestTrue(TEXT("Build provenance parses"), FWMFirstPersonNativeActivationRuntime::TryParseBuildProvenanceJson(ValidProvenanceJson(Build, Reviewed), Provenance));
    TestTrue(TEXT("Activated manifest is structurally valid"), State.IsStructurallyValid());
    TestTrue(TEXT("Build provenance is structurally valid"), Provenance.IsStructurallyValid());
    TestTrue(TEXT("Reviewed source plus bound build provenance is approved"), State.AllowsProductionTakeover(Provenance));
    TestTrue(TEXT("Activation build may differ from reviewed source without self-reference"), Build != Reviewed);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMFirstPersonNativeActivationWrongCommitTest,
    "WorldMakers.Visual.FirstPersonNativeActivation.WrongCommitFailsClosed",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMFirstPersonNativeActivationWrongCommitTest::RunTest(const FString& Parameters)
{
    const FString Reviewed = TEXT("0123456789abcdef0123456789abcdef01234567");
    const FString OtherReviewed = TEXT("fedcba9876543210fedcba9876543210fedcba98");
    FWMFirstPersonNativeActivationState State;
    FWMFirstPersonBuildProvenance Provenance;
    TestTrue(TEXT("Activated manifest parses"), FWMFirstPersonNativeActivationRuntime::TryParseJson(ValidActivatedJson(Reviewed), State));
    TestTrue(TEXT("Mismatched provenance parses structurally"), FWMFirstPersonNativeActivationRuntime::TryParseBuildProvenanceJson(
        ValidProvenanceJson(TEXT("1111111111111111111111111111111111111111"), OtherReviewed), Provenance));
    TestFalse(TEXT("Different reviewed source is rejected"), State.AllowsProductionTakeover(Provenance));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMFirstPersonNativeActivationUnboundProvenanceTest,
    "WorldMakers.Visual.FirstPersonNativeActivation.UnboundBuildProvenanceFailsClosed",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMFirstPersonNativeActivationUnboundProvenanceTest::RunTest(const FString& Parameters)
{
    const FString Reviewed = TEXT("0123456789abcdef0123456789abcdef01234567");
    FWMFirstPersonNativeActivationState State;
    FWMFirstPersonBuildProvenance Provenance;
    TestTrue(TEXT("Activated manifest parses"), FWMFirstPersonNativeActivationRuntime::TryParseJson(ValidActivatedJson(Reviewed), State));
    const FString Unbound = TEXT(R"JSON({
        "schemaVersion":1,
        "provenanceId":"visual.first-person-build-provenance.v1",
        "status":"unbound",
        "buildCommitSha":"",
        "reviewedSourceCommitSha":"",
        "activationCandidateSha256":"",
        "activationManifestSha256":""
    })JSON");
    TestTrue(TEXT("Unbound provenance parses"), FWMFirstPersonNativeActivationRuntime::TryParseBuildProvenanceJson(Unbound, Provenance));
    TestFalse(TEXT("Unbound provenance is not structurally valid"), Provenance.IsStructurallyValid());
    TestFalse(TEXT("Unbound provenance denies takeover"), State.AllowsProductionTakeover(Provenance));
    return true;
}

#endif
