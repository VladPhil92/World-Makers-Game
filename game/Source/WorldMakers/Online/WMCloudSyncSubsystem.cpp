#include "Online/WMCloudSyncSubsystem.h"

#include "Dom/JsonObject.h"
#include "Http.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
    constexpr TCHAR PlayerStatePath[] = TEXT("/api/worldmakers/player-state");

    FString NormalizeBaseUrl(const FString& Input)
    {
        FString Value = Input.TrimStartAndEnd();
        while (Value.EndsWith(TEXT("/")))
        {
            Value.LeftChopInline(1, EAllowShrinking::No);
        }
        return Value;
    }

    bool IsAllowedApiBaseUrl(const FString& Url)
    {
        if (Url.StartsWith(TEXT("https://"), ESearchCase::IgnoreCase))
        {
            return true;
        }

#if WITH_EDITOR
        return Url.StartsWith(TEXT("http://localhost"), ESearchCase::IgnoreCase)
            || Url.StartsWith(TEXT("http://127.0.0.1"), ESearchCase::IgnoreCase);
#else
        return false;
#endif
    }
}

void UWMCloudSyncSubsystem::Deinitialize()
{
    ClearCredentials();
    Super::Deinitialize();
}

bool UWMCloudSyncSubsystem::Configure(const FString& InApiBaseUrl, const FString& InAccessToken)
{
    const FString NormalizedUrl = NormalizeBaseUrl(InApiBaseUrl);
    const FString NormalizedToken = InAccessToken.TrimStartAndEnd();

    if (!IsAllowedApiBaseUrl(NormalizedUrl) || NormalizedToken.IsEmpty())
    {
        return false;
    }

    ApiBaseUrl = NormalizedUrl;
    AccessToken = NormalizedToken;
    return true;
}

void UWMCloudSyncSubsystem::ClearCredentials()
{
    AccessToken.Reset();
    LastStateJson.Reset();
    CurrentRevision = 0;
    bHasCloudProfile = false;
    bRequestInFlight = false;
}

bool UWMCloudSyncSubsystem::IsConfigured() const
{
    return ValidateConfiguration();
}

bool UWMCloudSyncSubsystem::ValidateConfiguration() const
{
    return !AccessToken.IsEmpty() && IsAllowedApiBaseUrl(ApiBaseUrl);
}

FString UWMCloudSyncSubsystem::CreateSyncEventId() const
{
    return FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphensLower);
}

bool UWMCloudSyncSubsystem::PullPlayerState()
{
    if (!ValidateConfiguration())
    {
        OnPullCompleted.Broadcast(false, CurrentRevision, LastStateJson, TEXT("not_configured"));
        return false;
    }

    if (bRequestInFlight)
    {
        OnPullCompleted.Broadcast(false, CurrentRevision, LastStateJson, TEXT("request_in_flight"));
        return false;
    }

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(ApiBaseUrl + PlayerStatePath);
    Request->SetVerb(TEXT("GET"));
    Request->SetHeader(TEXT("Accept"), TEXT("application/json"));
    Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *AccessToken));
    Request->OnProcessRequestComplete().BindUObject(this, &UWMCloudSyncSubsystem::HandlePullResponse);

    bRequestInFlight = true;
    if (!Request->ProcessRequest())
    {
        bRequestInFlight = false;
        OnPullCompleted.Broadcast(false, CurrentRevision, LastStateJson, TEXT("request_start_failed"));
        return false;
    }

    return true;
}

bool UWMCloudSyncSubsystem::PushPlayerState(
    const FString& StatePayloadJson,
    int32 ExpectedRevision,
    const FString& EventId)
{
    if (!ValidateConfiguration())
    {
        OnPushCompleted.Broadcast(false, CurrentRevision, LastStateJson, TEXT("not_configured"));
        return false;
    }

    if (bRequestInFlight)
    {
        OnPushCompleted.Broadcast(false, CurrentRevision, LastStateJson, TEXT("request_in_flight"));
        return false;
    }

    if (ExpectedRevision < 0)
    {
        OnPushCompleted.Broadcast(false, CurrentRevision, LastStateJson, TEXT("invalid_revision"));
        return false;
    }

    const FString NormalizedEventId = EventId.TrimStartAndEnd();
    if (NormalizedEventId.Len() < 8 || NormalizedEventId.Len() > 128)
    {
        OnPushCompleted.Broadcast(false, CurrentRevision, LastStateJson, TEXT("invalid_event_id"));
        return false;
    }

    FString RequestBody;
    FString BuildError;
    if (!BuildPushBody(StatePayloadJson, ExpectedRevision, RequestBody, BuildError))
    {
        OnPushCompleted.Broadcast(false, CurrentRevision, LastStateJson, BuildError);
        return false;
    }

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(ApiBaseUrl + PlayerStatePath);
    Request->SetVerb(TEXT("PUT"));
    Request->SetHeader(TEXT("Accept"), TEXT("application/json"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *AccessToken));
    Request->SetHeader(TEXT("Idempotency-Key"), NormalizedEventId);
    Request->SetContentAsString(RequestBody);
    Request->OnProcessRequestComplete().BindUObject(this, &UWMCloudSyncSubsystem::HandlePushResponse);

    bRequestInFlight = true;
    if (!Request->ProcessRequest())
    {
        bRequestInFlight = false;
        OnPushCompleted.Broadcast(false, CurrentRevision, LastStateJson, TEXT("request_start_failed"));
        return false;
    }

    return true;
}

bool UWMCloudSyncSubsystem::BuildPushBody(
    const FString& StatePayloadJson,
    int32 ExpectedRevision,
    FString& OutBody,
    FString& OutErrorCode) const
{
    TSharedPtr<FJsonObject> InputObject;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(StatePayloadJson);
    if (!FJsonSerializer::Deserialize(Reader, InputObject) || !InputObject.IsValid())
    {
        OutErrorCode = TEXT("invalid_state_json");
        return false;
    }

    const TArray<TSharedPtr<FJsonValue>>* Saves = nullptr;
    const TArray<TSharedPtr<FJsonValue>>* Missions = nullptr;
    const TArray<TSharedPtr<FJsonValue>>* Discoveries = nullptr;
    const TArray<TSharedPtr<FJsonValue>>* Achievements = nullptr;

    if (!InputObject->TryGetArrayField(TEXT("saves"), Saves)
        || !InputObject->TryGetArrayField(TEXT("missions"), Missions)
        || !InputObject->TryGetArrayField(TEXT("discoveries"), Discoveries)
        || !InputObject->TryGetArrayField(TEXT("achievements"), Achievements)
        || Saves == nullptr
        || Missions == nullptr
        || Discoveries == nullptr
        || Achievements == nullptr)
    {
        OutErrorCode = TEXT("missing_state_arrays");
        return false;
    }

    TSharedRef<FJsonObject> OutputObject = MakeShared<FJsonObject>();
    OutputObject->SetNumberField(TEXT("schemaVersion"), 1);
    OutputObject->SetNumberField(TEXT("expectedRevision"), ExpectedRevision);
    OutputObject->SetArrayField(TEXT("saves"), *Saves);
    OutputObject->SetArrayField(TEXT("missions"), *Missions);
    OutputObject->SetArrayField(TEXT("discoveries"), *Discoveries);
    OutputObject->SetArrayField(TEXT("achievements"), *Achievements);

    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutBody);
    if (!FJsonSerializer::Serialize(OutputObject, Writer))
    {
        OutErrorCode = TEXT("state_serialization_failed");
        return false;
    }

    return true;
}

FString UWMCloudSyncSubsystem::ExtractApiError(const FString& ResponseBody, const FString& Fallback)
{
    TSharedPtr<FJsonObject> Object;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseBody);
    if (FJsonSerializer::Deserialize(Reader, Object) && Object.IsValid())
    {
        FString Error;
        if (Object->TryGetStringField(TEXT("error"), Error) && !Error.IsEmpty())
        {
            return Error;
        }
    }

    return Fallback;
}

bool UWMCloudSyncSubsystem::ParseSuccessfulStateResponse(
    const FString& ResponseBody,
    int32& OutRevision,
    bool& OutProfileExists,
    FString& OutErrorCode) const
{
    TSharedPtr<FJsonObject> Object;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseBody);
    if (!FJsonSerializer::Deserialize(Reader, Object) || !Object.IsValid())
    {
        OutErrorCode = TEXT("invalid_response_json");
        return false;
    }

    double SchemaVersion = 0.0;
    if (!Object->TryGetNumberField(TEXT("schemaVersion"), SchemaVersion) || SchemaVersion < 2.0)
    {
        OutErrorCode = TEXT("unsupported_response_schema");
        return false;
    }

    const TSharedPtr<FJsonObject>* Profile = nullptr;
    if (!Object->TryGetObjectField(TEXT("profile"), Profile) || Profile == nullptr || !Profile->IsValid())
    {
        OutErrorCode = TEXT("missing_profile_contract");
        return false;
    }

    double RevisionNumber = 0.0;
    bool bExists = false;
    if (!(*Profile)->TryGetNumberField(TEXT("revision"), RevisionNumber)
        || !(*Profile)->TryGetBoolField(TEXT("exists"), bExists))
    {
        OutErrorCode = TEXT("invalid_profile_contract");
        return false;
    }

    if (RevisionNumber < 0.0 || RevisionNumber > static_cast<double>(MAX_int32))
    {
        OutErrorCode = TEXT("invalid_profile_revision");
        return false;
    }

    OutRevision = static_cast<int32>(RevisionNumber);
    OutProfileExists = bExists;
    return true;
}

void UWMCloudSyncSubsystem::HandlePullResponse(
    FHttpRequestPtr Request,
    FHttpResponsePtr Response,
    bool bConnectedSuccessfully)
{
    bRequestInFlight = false;

    if (!bConnectedSuccessfully || !Response.IsValid())
    {
        OnPullCompleted.Broadcast(false, CurrentRevision, LastStateJson, TEXT("network_error"));
        return;
    }

    const FString Body = Response->GetContentAsString();
    const int32 StatusCode = Response->GetResponseCode();
    if (!EHttpResponseCodes::IsOk(StatusCode))
    {
        OnPullCompleted.Broadcast(
            false,
            CurrentRevision,
            LastStateJson,
            ExtractApiError(Body, FString::Printf(TEXT("http_%d"), StatusCode)));
        return;
    }

    int32 ParsedRevision = 0;
    bool bProfileExists = false;
    FString ParseError;
    if (!ParseSuccessfulStateResponse(Body, ParsedRevision, bProfileExists, ParseError))
    {
        OnPullCompleted.Broadcast(false, CurrentRevision, LastStateJson, ParseError);
        return;
    }

    CurrentRevision = ParsedRevision;
    bHasCloudProfile = bProfileExists;
    LastStateJson = Body;
    OnPullCompleted.Broadcast(true, CurrentRevision, LastStateJson, FString());
}

void UWMCloudSyncSubsystem::HandlePushResponse(
    FHttpRequestPtr Request,
    FHttpResponsePtr Response,
    bool bConnectedSuccessfully)
{
    bRequestInFlight = false;

    if (!bConnectedSuccessfully || !Response.IsValid())
    {
        OnPushCompleted.Broadcast(false, CurrentRevision, LastStateJson, TEXT("network_error"));
        return;
    }

    const FString Body = Response->GetContentAsString();
    const int32 StatusCode = Response->GetResponseCode();
    if (!EHttpResponseCodes::IsOk(StatusCode))
    {
        OnPushCompleted.Broadcast(
            false,
            CurrentRevision,
            LastStateJson,
            ExtractApiError(Body, FString::Printf(TEXT("http_%d"), StatusCode)));
        return;
    }

    int32 ParsedRevision = 0;
    bool bProfileExists = false;
    FString ParseError;
    if (!ParseSuccessfulStateResponse(Body, ParsedRevision, bProfileExists, ParseError))
    {
        OnPushCompleted.Broadcast(false, CurrentRevision, LastStateJson, ParseError);
        return;
    }

    CurrentRevision = ParsedRevision;
    bHasCloudProfile = bProfileExists;
    LastStateJson = Body;
    OnPushCompleted.Broadcast(true, CurrentRevision, LastStateJson, FString());
}
