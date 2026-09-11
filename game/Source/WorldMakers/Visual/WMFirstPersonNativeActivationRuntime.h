#pragma once

#include "CoreMinimal.h"

/**
 * Build-time provenance generated after checkout and before packaging.
 * It avoids the impossible requirement that a committed manifest contain the SHA of the
 * same commit that contains it. The reviewed source commit stays immutable while the
 * actual activation/build commit is bound at packaging time.
 */
struct WORLDMAKERS_API FWMFirstPersonBuildProvenance
{
    FString Status = TEXT("unbound");
    FString BuildCommitSha;
    FString ReviewedSourceCommitSha;
    FString ActivationCandidateSha256;
    FString ActivationManifestSha256;

    bool IsStructurallyValid() const;
};

/**
 * Fail-closed production activation state for the first-person authored pack.
 * TargetCommitSha is the source commit that received native review, not the activation
 * commit itself. Review mode remains separate from production authorization.
 */
struct WORLDMAKERS_API FWMFirstPersonNativeActivationState
{
    bool bActivated = false;
    FString Status = TEXT("blocked");
    FString TargetCommitSha;
    FString NativeImportReportSha256;
    FString ReviewEvidenceSha256;
    FString ActivationCandidateSha256;
    bool bAllFiveAssetsApproved = false;
    bool bAllNineAnimationsApproved = false;
    bool bHumanReviewApproved = false;
    bool bDeviceReviewApproved = false;

    bool IsStructurallyValid() const;
    bool AllowsProductionTakeover(const FWMFirstPersonBuildProvenance& Provenance) const;
};

struct WORLDMAKERS_API FWMFirstPersonNativeActivationRuntime
{
    static bool TryParseJson(const FString& JsonText, FWMFirstPersonNativeActivationState& OutState);
    static bool TryLoadPackagedState(FWMFirstPersonNativeActivationState& OutState);
    static bool TryParseBuildProvenanceJson(const FString& JsonText, FWMFirstPersonBuildProvenance& OutState);
    static bool TryLoadPackagedBuildProvenance(FWMFirstPersonBuildProvenance& OutState);
    static FString PackagedRelativePath();
    static FString PackagedBuildProvenanceRelativePath();
};
