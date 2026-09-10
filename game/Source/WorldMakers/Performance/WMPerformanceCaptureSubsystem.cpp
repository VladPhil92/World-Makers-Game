#include "Performance/WMPerformanceCaptureSubsystem.h"

#include "Building/WMBuildWorldStateSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Environment/WMBiomeRuntimeSubsystem.h"
#include "Environment/WMEnvironmentStateSubsystem.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Performance/WMPerformanceProfileSubsystem.h"

TStatId UWMPerformanceCaptureSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UWMPerformanceCaptureSubsystem, STATGROUP_Tickables);
}

bool UWMPerformanceCaptureSubsystem::BeginCapture()
{
    UWorld* World = GetWorld();
    UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
    UWMPerformanceProfileSubsystem* Profiles = GameInstance ? GameInstance->GetSubsystem<UWMPerformanceProfileSubsystem>() : nullptr;
    if (!Profiles) return false;

    const FWMPerformanceProfileDefinition Profile = Profiles->GetActiveProfile();
    if (!Profile.IsSane()) return false;

    CaptureProfileId = Profile.ProfileId;
    CaptureBudget = Profile.Budget;
    Accumulator.Reset();
    LastSummary = FWMPerformanceCaptureSummary();
    LastReportPath.Reset();
    StructuralSampleAccumulatorSeconds = 0.0f;
    bCapturing = true;
    ObserveStructuralCounts();
    return true;
}

void UWMPerformanceCaptureSubsystem::Tick(const float DeltaTime)
{
    if (!bCapturing || !FMath::IsFinite(DeltaTime) || DeltaTime <= 0.0f) return;

    Accumulator.AddFrameTimeMs(DeltaTime * 1000.0f);
    StructuralSampleAccumulatorSeconds += DeltaTime;
    if (StructuralSampleAccumulatorSeconds >= StructuralSampleIntervalSeconds)
    {
        ObserveStructuralCounts();
        StructuralSampleAccumulatorSeconds = 0.0f;
    }
}

FWMPerformanceCaptureSummary UWMPerformanceCaptureSubsystem::EndCapture(const bool bWriteReport)
{
    if (!bCapturing) return LastSummary;

    ObserveStructuralCounts();
    bCapturing = false;
    LastSummary = Accumulator.BuildSummary(CaptureProfileId, CaptureBudget);
    if (bWriteReport)
    {
        WriteSummaryReport(LastSummary);
    }
    return LastSummary;
}

FWMPerformanceCaptureSummary UWMPerformanceCaptureSubsystem::GetLiveSummary() const
{
    if (!bCapturing) return LastSummary;
    return Accumulator.BuildSummary(CaptureProfileId, CaptureBudget);
}

void UWMPerformanceCaptureSubsystem::ObserveStructuralCounts()
{
    UWorld* World = GetWorld();
    if (!World) return;

    int32 ActorCount = 0;
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        ++ActorCount;
    }

    int32 BuildPieceCount = 0;
    if (const UWMBuildWorldStateSubsystem* BuildWorld = World->GetSubsystem<UWMBuildWorldStateSubsystem>())
    {
        BuildPieceCount = BuildWorld->GetPlacedPieces().Num();
    }

    int32 InteractableCount = 0;
    if (const UWMBiomeRuntimeSubsystem* Biome = World->GetSubsystem<UWMBiomeRuntimeSubsystem>())
    {
        InteractableCount += Biome->GetInteractionTargetCount();
    }
    if (const UWMEnvironmentStateSubsystem* Environment = World->GetSubsystem<UWMEnvironmentStateSubsystem>())
    {
        InteractableCount += Environment->GetActionTargetCount();
    }

    Accumulator.ObserveStructuralCounts(ActorCount, BuildPieceCount, InteractableCount);
}

bool UWMPerformanceCaptureSubsystem::WriteSummaryReport(const FWMPerformanceCaptureSummary& Summary)
{
    if (!Summary.HasSamples() || Summary.ProfileId.IsNone()) return false;

    const FString Directory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("WorldMakers/Performance"));
    if (!IFileManager::Get().MakeDirectory(*Directory, true) && !IFileManager::Get().DirectoryExists(*Directory))
    {
        return false;
    }

    FString SafeProfileId = Summary.ProfileId.ToString();
    SafeProfileId.ReplaceInline(TEXT("."), TEXT("-"));
    SafeProfileId.ReplaceInline(TEXT("/"), TEXT("-"));
    SafeProfileId.ReplaceInline(TEXT("\\"), TEXT("-"));
    LastReportPath = FPaths::Combine(Directory, FString::Printf(TEXT("capture-%s.json"), *SafeProfileId));
    return FFileHelper::SaveStringToFile(Summary.ToJson(), *LastReportPath);
}
