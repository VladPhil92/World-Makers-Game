#include "Visual/WMFirstPersonNativeActivationRuntime.h"

#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
    bool IsLowerHexOfLength(const FString& Value, const int32 Length)
    {
        if (Value.Len() != Length) return false;
        for (const TCHAR Character : Value)
        {
            const bool bDigit = Character >= TEXT('0') && Character <= TEXT('9');
            const bool bHex = Character >= TEXT('a') && Character <= TEXT('f');
            if (!bDigit && !bHex) return false;
        }
        return true;
    }
}

bool FWMFirstPersonNativeActivationState::IsStructurallyValid() const
{
    if (!bActivated || Status != TEXT("activated")) return false;
    if (!IsLowerHexOfLength(TargetCommitSha, 40)) return false;
    if (!IsLowerHexOfLength(NativeImportReportSha256, 64)) return false;
    if (!IsLowerHexOfLength(ReviewEvidenceSha256, 64)) return false;
    if (!IsLowerHexOfLength(ActivationCandidateSha256, 64)) return false;
    return bAllFiveAssetsApproved && bAllNineAnimationsApproved && bHumanReviewApproved && bDeviceReviewApproved;
}

bool FWMFirstPersonNativeActivationState::AllowsProductionTakeover(const FString& BuildCommitSha) const
{
    return IsStructurallyValid() && IsLowerHexOfLength(BuildCommitSha, 40) && TargetCommitSha == BuildCommitSha;
}

bool FWMFirstPersonNativeActivationRuntime::TryParseJson(const FString& JsonText, FWMFirstPersonNativeActivationState& OutState)
{
    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid()) return false;

    double SchemaVersion = 0.0;
    if (!Root->TryGetNumberField(TEXT("schemaVersion"), SchemaVersion) || static_cast<int32>(SchemaVersion) != 1) return false;

    FString ActivationId;
    if (!Root->TryGetStringField(TEXT("activationId"), ActivationId) || ActivationId != TEXT("visual.first-person-native-activation.v1")) return false;

    FWMFirstPersonNativeActivationState Parsed;
    Root->TryGetStringField(TEXT("status"), Parsed.Status);
    Root->TryGetBoolField(TEXT("activated"), Parsed.bActivated);
    Root->TryGetStringField(TEXT("targetCommitSha"), Parsed.TargetCommitSha);
    Root->TryGetStringField(TEXT("nativeImportReportSha256"), Parsed.NativeImportReportSha256);
    Root->TryGetStringField(TEXT("reviewEvidenceSha256"), Parsed.ReviewEvidenceSha256);
    Root->TryGetStringField(TEXT("activationCandidateSha256"), Parsed.ActivationCandidateSha256);
    Root->TryGetBoolField(TEXT("allFiveAssetsApproved"), Parsed.bAllFiveAssetsApproved);
    Root->TryGetBoolField(TEXT("allNineAnimationsApproved"), Parsed.bAllNineAnimationsApproved);
    Root->TryGetBoolField(TEXT("humanReviewApproved"), Parsed.bHumanReviewApproved);
    Root->TryGetBoolField(TEXT("deviceReviewApproved"), Parsed.bDeviceReviewApproved);

    OutState = MoveTemp(Parsed);
    return true;
}

bool FWMFirstPersonNativeActivationRuntime::TryLoadPackagedState(FWMFirstPersonNativeActivationState& OutState)
{
    FString JsonText;
    const FString FullPath = FPaths::Combine(FPaths::ProjectContentDir(), PackagedRelativePath());
    return FFileHelper::LoadFileToString(JsonText, *FullPath) && TryParseJson(JsonText, OutState);
}

FString FWMFirstPersonNativeActivationRuntime::PackagedRelativePath()
{
    return TEXT("WorldMakers/Visual/first-person-native-activation-v1.json");
}
