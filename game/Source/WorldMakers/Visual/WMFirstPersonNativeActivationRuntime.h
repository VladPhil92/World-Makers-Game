#pragma once

#include "CoreMinimal.h"

/**
 * Fail-closed production activation state for the first-person authored pack.
 * Review mode is handled separately by the authored bridge command-line flag.
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
    bool AllowsProductionTakeover(const FString& BuildCommitSha) const;
};

struct WORLDMAKERS_API FWMFirstPersonNativeActivationRuntime
{
    static bool TryParseJson(const FString& JsonText, FWMFirstPersonNativeActivationState& OutState);
    static bool TryLoadPackagedState(FWMFirstPersonNativeActivationState& OutState);
    static FString PackagedRelativePath();
};
