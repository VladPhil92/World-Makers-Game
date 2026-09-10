#include "Performance/WMPerformanceProfileSubsystem.h"

#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

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
