#include "Performance/WMPerformanceProfileSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Environment/WMCaribbeanRainforestPrototype.h"
#include "HAL/IConsoleManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Visual/WMVisualProfileSettings.h"

namespace
{
    const FName TabletMediumProfileId(TEXT("performance.tablet.medium"));
    const FName DesktopReferenceProfileId(TEXT("performance.desktop.reference"));
}

void UWMPerformanceProfileSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    if (ReloadProfiles())
    {
        ApplyProfile(ResolvePlatformDefaultProfileId());
    }
}

bool UWMPerformanceProfileSubsystem::ReloadProfiles()
{
    Catalog = FWMPerformanceProfileCatalog();
    ActiveProfileId = NAME_None;

    const FString Path = FPaths::Combine(
        FPaths::ProjectContentDir(),
        TEXT("WorldMakers/Performance/tablet-performance-profiles.json"));

    FString Json;
    if (!FFileHelper::LoadFileToString(Json, *Path))
    {
        return false;
    }

    FWMPerformanceProfileCatalog Parsed;
    FString Error;
    if (!FWMPerformanceProfileCatalog::TryParseJson(Json, Parsed, Error))
    {
        return false;
    }

    Catalog = MoveTemp(Parsed);
    return true;
}

bool UWMPerformanceProfileSubsystem::ApplyProfile(const FName ProfileId)
{
    const FWMPerformanceProfileDefinition* Profile = Catalog.FindProfile(ProfileId);
    if (!Profile || !Profile->IsSane()) return false;
    if (!ApplyAllowlistedScalability(Profile->Scalability)) return false;
    if (!ApplyVisualQualityTier(Profile->Tier)) return false;
    ActiveProfileId = ProfileId;
    return true;
}

TArray<FName> UWMPerformanceProfileSubsystem::GetAvailableProfileIds() const
{
    TArray<FName> Result;
    Result.Reserve(Catalog.Profiles.Num());
    for (const FWMPerformanceProfileDefinition& Profile : Catalog.Profiles)
    {
        Result.Add(Profile.ProfileId);
    }
    Result.Sort([](const FName& A, const FName& B)
    {
        return A.ToString() < B.ToString();
    });
    return Result;
}

FWMPerformanceProfileDefinition UWMPerformanceProfileSubsystem::GetActiveProfile() const
{
    const FWMPerformanceProfileDefinition* Profile = Catalog.FindProfile(ActiveProfileId);
    return Profile ? *Profile : FWMPerformanceProfileDefinition();
}

const FWMPerformanceProfileDefinition* UWMPerformanceProfileSubsystem::FindProfile(const FName ProfileId) const
{
    return Catalog.FindProfile(ProfileId);
}

FName UWMPerformanceProfileSubsystem::ResolvePlatformDefaultProfileId() const
{
#if PLATFORM_ANDROID || PLATFORM_IOS
    return TabletMediumProfileId;
#else
    return DesktopReferenceProfileId;
#endif
}

bool UWMPerformanceProfileSubsystem::ApplyAllowlistedScalability(const FWMScalabilityProfile& Scalability) const
{
    if (!Scalability.IsSane()) return false;

    const TMap<FString, FString> Assignments = Scalability.BuildAllowlistedCVarAssignments();
    for (const TPair<FString, FString>& Assignment : Assignments)
    {
        IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(*Assignment.Key);
        if (!Variable)
        {
            return false;
        }
        Variable->Set(*Assignment.Value, ECVF_SetByGameSetting);
    }
    return true;
}

bool UWMPerformanceProfileSubsystem::ApplyVisualQualityTier(const FName Tier) const
{
    EWMVisualQualityTier VisualTier = EWMVisualQualityTier::Mid;
    if (Tier == FName(TEXT("low")))
    {
        VisualTier = EWMVisualQualityTier::Low;
    }
    else if (Tier == FName(TEXT("medium")))
    {
        VisualTier = EWMVisualQualityTier::Mid;
    }
    else if (Tier == FName(TEXT("high")) || Tier == FName(TEXT("reference")))
    {
        VisualTier = EWMVisualQualityTier::High;
    }
    else
    {
        return false;
    }

    UWMVisualProfileSettings* VisualProfile = GetMutableDefault<UWMVisualProfileSettings>();
    if (!VisualProfile)
    {
        return false;
    }
    VisualProfile->DefaultQualityTier = VisualTier;

    UGameInstance* GameInstance = GetGameInstance();
    UWorld* World = GameInstance ? GameInstance->GetWorld() : nullptr;
    if (World)
    {
        for (TActorIterator<AWMCaribbeanRainforestPrototype> It(World); It; ++It)
        {
            It->RefreshFromVisualProfile();
        }
    }
    return true;
}
