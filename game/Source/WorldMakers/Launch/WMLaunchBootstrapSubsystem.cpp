#include "Launch/WMLaunchBootstrapSubsystem.h"

#include "Adventure/WMEclipseEngineExperienceSubsystem.h"
#include "Adventure/WMEpicRuntimeSubsystem.h"
#include "Adventure/WMGardenEndWinterExperienceSubsystem.h"
#include "Dom/JsonObject.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/DateTime.h"
#include "Misc/Parse.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
    const FString NativeLaunchPrefix(TEXT("worldmakers://launch?"));
    const FString NativeLaunchProtocol(TEXT("worldmakers-launch-v2"));
    const FString ProgressSyncScope(TEXT("epic-checkpoint:write"));

    bool IsOpaqueToken(const FString& Value)
    {
        if (Value.Len() < 32 || Value.Len() > 256) return false;
        for (const TCHAR Character : Value)
        {
            if (!FChar::IsAlnum(Character) && Character != TEXT('_') && Character != TEXT('-')) return false;
        }
        return true;
    }

    bool IsFutureIso8601(const FString& Value)
    {
        FDateTime Parsed;
        return FDateTime::ParseIso8601(*Value, Parsed) && Parsed > FDateTime::UtcNow();
    }

    FString JsonString(const TSharedRef<FJsonObject>& Object)
    {
        FString Result;
        const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Result);
        FJsonSerializer::Serialize(Object, Writer);
        return Result;
    }
}

void UWMLaunchBootstrapSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    FString Ticket;
    if (!TryExtractTicketFromCommandLine(FCommandLine::Get(), Ticket)) return;

    bNativeLaunchRequested = true;
    LaunchTicket = Ticket;
    ApiBaseUrl = FPlatformMisc::GetEnvironmentVariable(TEXT("WORLD_MAKERS_API_BASE_URL"));
    if (ApiBaseUrl.IsEmpty() && GConfig)
    {
        GConfig->GetString(
            TEXT("/Script/WorldMakers.WMLaunchBootstrapSubsystem"),
            TEXT("ApiBaseUrl"),
            ApiBaseUrl,
            GGameIni);
    }
    ApiBaseUrl.TrimStartAndEndInline();
    while (ApiBaseUrl.EndsWith(TEXT("/"))) ApiBaseUrl.LeftChopInline(1);

    if (!IsTrustedApiBaseUrl(ApiBaseUrl))
    {
        SetLaunchError(TEXT("Native launch API base URL is missing or untrusted."));
        return;
    }

    BeginTicketRedemption();
}

void UWMLaunchBootstrapSubsystem::Deinitialize()
{
    LaunchTicket.Reset();
    ProgressSyncToken.Reset();
    EpicResume = FWMNativeEpicResume();
    Super::Deinitialize();
}

bool UWMLaunchBootstrapSubsystem::TryExtractTicketFromCommandLine(const FString& CommandLine, FString& OutTicket)
{
    OutTicket.Reset();
    const int32 UriStart = CommandLine.Find(NativeLaunchPrefix, ESearchCase::IgnoreCase, ESearchDir::FromStart);
    if (UriStart == INDEX_NONE) return false;

    int32 UriEnd = CommandLine.Len();
    for (int32 Index = UriStart; Index < CommandLine.Len(); ++Index)
    {
        const TCHAR Character = CommandLine[Index];
        if (Index > UriStart && (FChar::IsWhitespace(Character) || Character == TEXT('"') || Character == TEXT('\'')))
        {
            UriEnd = Index;
            break;
        }
    }

    const FString Uri = CommandLine.Mid(UriStart, UriEnd - UriStart);
    const int32 QueryIndex = Uri.Find(TEXT("?"));
    if (QueryIndex == INDEX_NONE) return false;

    FString Protocol;
    FString Ticket;
    bool bHasPayload = false;
    TArray<FString> Fields;
    Uri.Mid(QueryIndex + 1).ParseIntoArray(Fields, TEXT("&"), true);
    for (const FString& Field : Fields)
    {
        FString Key;
        FString Value;
        if (!Field.Split(TEXT("="), &Key, &Value)) continue;
        if (Key.Equals(TEXT("protocol"), ESearchCase::IgnoreCase)) Protocol = Value;
        else if (Key.Equals(TEXT("ticket"), ESearchCase::IgnoreCase)) Ticket = Value;
        else if (Key.Equals(TEXT("payload"), ESearchCase::IgnoreCase)) bHasPayload = true;
    }

    if (bHasPayload || Protocol != NativeLaunchProtocol || !IsOpaqueToken(Ticket)) return false;
    OutTicket = Ticket;
    return true;
}

bool UWMLaunchBootstrapSubsystem::IsTrustedApiBaseUrl(const FString& Candidate) const
{
    if (Candidate.StartsWith(TEXT("https://"), ESearchCase::IgnoreCase)) return true;
#if !UE_BUILD_SHIPPING
    const auto IsLoopbackBase = [&Candidate](const FString& Prefix)
    {
        if (!Candidate.StartsWith(Prefix, ESearchCase::IgnoreCase)) return false;
        const FString Remainder = Candidate.Mid(Prefix.Len());
        return Remainder.IsEmpty() || Remainder.StartsWith(TEXT(":")) || Remainder.StartsWith(TEXT("/"));
    };
    if (IsLoopbackBase(TEXT("http://localhost")) || IsLoopbackBase(TEXT("http://127.0.0.1"))) return true;
#endif
    return false;
}

void UWMLaunchBootstrapSubsystem::BeginTicketRedemption()
{
    if (!bNativeLaunchRequested || LaunchTicket.IsEmpty() || !IsTrustedApiBaseUrl(ApiBaseUrl))
    {
        SetLaunchError(TEXT("Native launch redemption cannot start."));
        return;
    }

    const TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
    Body->SetStringField(TEXT("ticket"), LaunchTicket);

    const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(ApiBaseUrl + TEXT("/api/native/launch/redeem"));
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetContentAsString(JsonString(Body));

    const TWeakObjectPtr<UWMLaunchBootstrapSubsystem> WeakThis(this);
    Request->OnProcessRequestComplete().BindLambda(
        [WeakThis](FHttpRequestPtr HttpRequest, FHttpResponsePtr Response, bool bSucceeded)
        {
            if (!WeakThis.IsValid()) return;
            UWMLaunchBootstrapSubsystem* Self = WeakThis.Get();
            if (!bSucceeded || !Response.IsValid() || Response->GetResponseCode() != 200)
            {
                Self->SetLaunchError(TEXT("Native launch ticket redemption failed."));
                return;
            }
            if (!Self->ConsumeRedemptionJson(Response->GetContentAsString()))
            {
                Self->SetLaunchError(TEXT("Native launch redemption payload is invalid."));
                return;
            }
            if (UGameInstance* GameInstance = Self->GetGameInstance())
            {
                if (UWorld* World = GameInstance->GetWorld(); World && World->HasBegunPlay()) Self->TryApplyEpicResume();
            }
        });

    if (!Request->ProcessRequest()) SetLaunchError(TEXT("Native launch redemption request could not be dispatched."));
}

bool UWMLaunchBootstrapSubsystem::ConsumeRedemptionJson(const FString& Json)
{
    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid()) return false;

    FString Protocol;
    if (!Root->TryGetStringField(TEXT("protocol"), Protocol) || Protocol != NativeLaunchProtocol) return false;
    if (!Root->HasTypedField<EJson::Object>(TEXT("context")) || !Root->HasTypedField<EJson::Object>(TEXT("progressSync"))) return false;

    const TSharedPtr<FJsonObject> Context = Root->GetObjectField(TEXT("context"));
    const TSharedPtr<FJsonObject> ProgressSync = Root->GetObjectField(TEXT("progressSync"));
    FString ExpiresAt;
    FString Token;
    FString Scope;
    FString SyncExpiresAt;
    if (!Context.IsValid() || !Context->TryGetStringField(TEXT("expiresAt"), ExpiresAt) || !IsFutureIso8601(ExpiresAt)) return false;
    if (!ProgressSync.IsValid() ||
        !ProgressSync->TryGetStringField(TEXT("token"), Token) || !IsOpaqueToken(Token) ||
        !ProgressSync->TryGetStringField(TEXT("scope"), Scope) || Scope != ProgressSyncScope ||
        !ProgressSync->TryGetStringField(TEXT("expiresAt"), SyncExpiresAt) || !IsFutureIso8601(SyncExpiresAt)) return false;

    FWMNativeEpicResume ParsedResume;
    if (Context->HasTypedField<EJson::Object>(TEXT("epicResume")))
    {
        const TSharedPtr<FJsonObject> Resume = Context->GetObjectField(TEXT("epicResume"));
        FString EpicId;
        FString ChapterId;
        int32 ChapterIndex = -1;
        int32 ChapterCount = 0;
        if (!Resume.IsValid() ||
            !Resume->TryGetStringField(TEXT("epicId"), EpicId) || EpicId.IsEmpty() ||
            !Resume->TryGetStringField(TEXT("chapterId"), ChapterId) || ChapterId.IsEmpty() ||
            !Resume->TryGetNumberField(TEXT("chapterIndex"), ChapterIndex) ||
            !Resume->TryGetNumberField(TEXT("chapterCount"), ChapterCount) ||
            ChapterIndex < 0 || ChapterCount <= 0 || ChapterIndex >= ChapterCount)
        {
            return false;
        }
        ParsedResume.EpicId = FName(*EpicId);
        ParsedResume.ChapterId = FName(*ChapterId);
        ParsedResume.ChapterIndex = ChapterIndex;
        ParsedResume.ChapterCount = ChapterCount;
    }

    ProgressSyncToken = MoveTemp(Token);
    EpicResume = ParsedResume;
    LaunchTicket.Reset();
    NativeLaunchError.Reset();
    bNativeLaunchError = false;
    bNativeLaunchReady = true;
    return true;
}

void UWMLaunchBootstrapSubsystem::SetLaunchError(const FString& Error)
{
    LaunchTicket.Reset();
    ProgressSyncToken.Reset();
    bNativeLaunchReady = false;
    bNativeLaunchError = true;
    NativeLaunchError = Error;
}

bool UWMLaunchBootstrapSubsystem::TryApplyEpicResume()
{
    if (!bNativeLaunchRequested || !bNativeLaunchReady || bNativeLaunchError) return false;
    if (bEpicResumeApplied) return true;
    if (!EpicResume.IsSet())
    {
        bEpicResumeApplied = true;
        return true;
    }

    UGameInstance* GameInstance = GetGameInstance();
    UWorld* World = GameInstance ? GameInstance->GetWorld() : nullptr;
    if (!World || !World->HasBegunPlay()) return false;

    UWMEpicRuntimeSubsystem* Epic = World->GetSubsystem<UWMEpicRuntimeSubsystem>();
    if (!Epic || !Epic->ApplyExternalResumeCheckpoint(
        EpicResume.EpicId,
        EpicResume.ChapterId,
        EpicResume.ChapterIndex,
        EpicResume.ChapterCount))
    {
        SetLaunchError(TEXT("Native epic resume checkpoint was rejected."));
        return false;
    }

    bool bStarted = false;
    if (EpicResume.EpicId == FName(TEXT("epic.eclipse-engine")))
    {
        if (UWMEclipseEngineExperienceSubsystem* Experience = World->GetSubsystem<UWMEclipseEngineExperienceSubsystem>())
        {
            bStarted = Experience->StartEclipseEngine();
        }
    }
    else if (EpicResume.EpicId == FName(TEXT("epic.garden-end-winter")))
    {
        if (UWMGardenEndWinterExperienceSubsystem* Experience = World->GetSubsystem<UWMGardenEndWinterExperienceSubsystem>())
        {
            bStarted = Experience->StartGardenEndWinter();
        }
    }

    if (!bStarted)
    {
        SetLaunchError(TEXT("Native epic experience could not start."));
        return false;
    }
    bEpicResumeApplied = true;
    return true;
}

bool UWMLaunchBootstrapSubsystem::SyncEpicCheckpoint(const FWMEpicCheckpoint& Checkpoint)
{
    if (!bNativeLaunchRequested || !bNativeLaunchReady || bNativeLaunchError || ProgressSyncToken.IsEmpty()) return false;
    if (Checkpoint.EpicId.IsNone() || Checkpoint.ChapterCount <= 0) return false;

    const TSharedRef<FJsonObject> CheckpointJson = MakeShared<FJsonObject>();
    CheckpointJson->SetStringField(TEXT("epicId"), Checkpoint.EpicId.ToString());
    if (Checkpoint.bCompleted) CheckpointJson->SetField(TEXT("chapterId"), MakeShared<FJsonValueNull>());
    else CheckpointJson->SetStringField(TEXT("chapterId"), Checkpoint.ChapterId.ToString());
    CheckpointJson->SetNumberField(TEXT("chapterIndex"), Checkpoint.ChapterIndex);
    CheckpointJson->SetStringField(TEXT("state"), Checkpoint.bCompleted ? TEXT("complete") : TEXT("in-progress"));

    const TSharedRef<FJsonObject> Body = MakeShared<FJsonObject>();
    Body->SetStringField(TEXT("progressSyncToken"), ProgressSyncToken);
    Body->SetObjectField(TEXT("checkpoint"), CheckpointJson);

    const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(ApiBaseUrl + TEXT("/api/native/epic-checkpoint"));
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetContentAsString(JsonString(Body));
    // Network sync is intentionally best-effort. The local SaveGame already succeeded before this call.
    return Request->ProcessRequest();
}
