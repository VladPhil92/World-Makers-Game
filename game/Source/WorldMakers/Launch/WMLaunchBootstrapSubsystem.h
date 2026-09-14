#pragma once

#include "Adventure/WMEpicJourneySaveGame.h"
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "WMLaunchBootstrapSubsystem.generated.h"

USTRUCT(BlueprintType)
struct WORLDMAKERS_API FWMNativeEpicResume
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Launch")
    FName EpicId;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Launch")
    FName ChapterId;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Launch")
    int32 ChapterIndex = 0;

    UPROPERTY(BlueprintReadOnly, Category = "World Makers|Launch")
    int32 ChapterCount = 0;

    bool IsSet() const { return !EpicId.IsNone() && !ChapterId.IsNone() && ChapterIndex >= 0 && ChapterCount > 0; }
};

/**
 * M5.6F native bootstrap. A custom URI carries only an opaque one-time ticket.
 * The signed player context is redeemed from the configured HTTPS dashboard API;
 * no server HMAC secret is compiled into or persisted by the game client.
 */
UCLASS()
class WORLDMAKERS_API UWMLaunchBootstrapSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintPure, Category = "World Makers|Launch")
    bool IsNativeLaunchRequested() const { return bNativeLaunchRequested; }

    UFUNCTION(BlueprintPure, Category = "World Makers|Launch")
    bool IsNativeLaunchReady() const { return bNativeLaunchReady; }

    UFUNCTION(BlueprintPure, Category = "World Makers|Launch")
    bool HasNativeLaunchError() const { return bNativeLaunchError; }

    UFUNCTION(BlueprintPure, Category = "World Makers|Launch")
    FString GetNativeLaunchError() const { return NativeLaunchError; }

    UFUNCTION(BlueprintPure, Category = "World Makers|Launch")
    FName GetSelectedModeId() const { return SelectedModeId; }

    UFUNCTION(BlueprintPure, Category = "World Makers|Launch")
    FName GetSelectedWorldId() const { return SelectedWorldId; }

    UFUNCTION(BlueprintPure, Category = "World Makers|Launch")
    FName GetSelectedMissionId() const { return SelectedMissionId; }

    /** Native v2 launches suppress config-driven auto-start until ticket redemption succeeds or fails closed. */
    UFUNCTION(BlueprintPure, Category = "World Makers|Launch")
    bool ShouldDeferAutomaticExperienceStart() const { return bNativeLaunchRequested && !bNativeLaunchReady; }

    /** Apply the redeemed launch selection and optional epic resume once the gameplay world has begun play. */
    UFUNCTION(BlueprintCallable, Category = "World Makers|Launch")
    bool TryApplyEpicResume();

    /** Best-effort server sync. Local SaveGame remains authoritative while offline. */
    bool SyncEpicCheckpoint(const FWMEpicCheckpoint& Checkpoint);

    /** Pure parser used by automation tests and startup. */
    static bool TryExtractTicketFromCommandLine(const FString& CommandLine, FString& OutTicket);

private:
    bool IsTrustedApiBaseUrl(const FString& Candidate) const;
    void BeginTicketRedemption();
    bool ConsumeRedemptionJson(const FString& Json);
    bool StartSelectedContent();
    void SetLaunchError(const FString& Error);

    bool bNativeLaunchRequested = false;
    bool bNativeLaunchReady = false;
    bool bNativeLaunchError = false;
    bool bEpicResumeApplied = false;
    FString NativeLaunchError;
    FString ApiBaseUrl;
    FString LaunchTicket;
    FString ProgressSyncToken;
    FName SelectedModeId;
    FName SelectedWorldId;
    FName SelectedMissionId;
    FWMNativeEpicResume EpicResume;
};