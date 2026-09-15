#pragma once

#include "CoreMinimal.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "WMCloudSyncSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
    FWMCloudSyncResult,
    bool, bSuccess,
    int32, Revision,
    FString, StateJson,
    FString, ErrorCode);

/**
 * Runtime bridge between the Unreal client and the authenticated CTG One
 * player-state API. The subsystem never stores the CTG One bearer token on
 * disk and never talks directly to the World Makers Supabase project.
 *
 * Authentication is intentionally injected by the launcher/auth layer. That
 * keeps identity ownership in CTG One while this subsystem focuses only on
 * cloud-state transport, optimistic revisions and idempotent writes.
 */
UCLASS()
class WORLDMAKERS_API UWMCloudSyncSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Deinitialize() override;

    /** Configure the CTG One API origin and an already-issued bearer token. */
    UFUNCTION(BlueprintCallable, Category = "World Makers|Cloud Sync")
    bool Configure(const FString& InApiBaseUrl, const FString& InAccessToken);

    /** Remove all in-memory credentials and cached remote state. */
    UFUNCTION(BlueprintCallable, Category = "World Makers|Cloud Sync")
    void ClearCredentials();

    UFUNCTION(BlueprintPure, Category = "World Makers|Cloud Sync")
    bool IsConfigured() const;

    UFUNCTION(BlueprintPure, Category = "World Makers|Cloud Sync")
    bool IsRequestInFlight() const { return bRequestInFlight; }

    UFUNCTION(BlueprintPure, Category = "World Makers|Cloud Sync")
    int32 GetCurrentRevision() const { return CurrentRevision; }

    UFUNCTION(BlueprintPure, Category = "World Makers|Cloud Sync")
    bool HasCloudProfile() const { return bHasCloudProfile; }

    UFUNCTION(BlueprintPure, Category = "World Makers|Cloud Sync")
    FString GetLastStateJson() const { return LastStateJson; }

    /** Create an idempotency key once, then reuse it for every retry of one write. */
    UFUNCTION(BlueprintCallable, Category = "World Makers|Cloud Sync")
    FString CreateSyncEventId() const;

    /** Pull the canonical remote state. Returns false if the request could not start. */
    UFUNCTION(BlueprintCallable, Category = "World Makers|Cloud Sync")
    bool PullPlayerState();

    /**
     * Push a full player-state document. StatePayloadJson must contain the arrays
     * `saves`, `missions`, `discoveries` and `achievements`. The subsystem adds
     * schemaVersion and expectedRevision before sending.
     *
     * EventId is mandatory. Generate it once with CreateSyncEventId and keep it
     * stable across retries so the backend can return the original committed
     * response without applying the write twice.
     */
    UFUNCTION(BlueprintCallable, Category = "World Makers|Cloud Sync")
    bool PushPlayerState(
        const FString& StatePayloadJson,
        int32 ExpectedRevision,
        const FString& EventId);

    UPROPERTY(BlueprintAssignable, Category = "World Makers|Cloud Sync")
    FWMCloudSyncResult OnPullCompleted;

    UPROPERTY(BlueprintAssignable, Category = "World Makers|Cloud Sync")
    FWMCloudSyncResult OnPushCompleted;

private:
    FString ApiBaseUrl = TEXT("https://worldmakers.ctgone.com");
    FString AccessToken;
    FString LastStateJson;
    int32 CurrentRevision = 0;
    bool bHasCloudProfile = false;
    bool bRequestInFlight = false;

    bool ValidateConfiguration() const;
    bool BuildPushBody(
        const FString& StatePayloadJson,
        int32 ExpectedRevision,
        FString& OutBody,
        FString& OutErrorCode) const;
    bool ParseSuccessfulStateResponse(
        const FString& ResponseBody,
        int32& OutRevision,
        bool& OutProfileExists,
        FString& OutErrorCode) const;
    static FString ExtractApiError(const FString& ResponseBody, const FString& Fallback);

    void HandlePullResponse(
        FHttpRequestPtr Request,
        FHttpResponsePtr Response,
        bool bConnectedSuccessfully);
    void HandlePushResponse(
        FHttpRequestPtr Request,
        FHttpResponsePtr Response,
        bool bConnectedSuccessfully);
};
