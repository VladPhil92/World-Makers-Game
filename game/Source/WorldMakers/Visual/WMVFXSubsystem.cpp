#include "Visual/WMVFXSubsystem.h"

#include "Building/WMBuildWorldStateSubsystem.h"
#include "Engine/World.h"
#include "Environment/WMEnvironmentStateSubsystem.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Visual/WMProceduralVFXActor.h"
#include "Visual/WMVisualProfileSettings.h"

namespace
{
    bool SamePlacedPiece(const FWMPlacedBuildPieceSnapshot& A, const FWMPlacedBuildPieceSnapshot& B)
    {
        return A.PieceId == B.PieceId && A.LocationCm.Equals(B.LocationCm, 0.1f) && FMath::IsNearlyEqual(A.YawDegrees, B.YawDegrees, 0.1f);
    }

    const FWMPlacedBuildPieceSnapshot* FindUnmatched(const TArray<FWMPlacedBuildPieceSnapshot>& Candidates, const TArray<FWMPlacedBuildPieceSnapshot>& Other)
    {
        for (const FWMPlacedBuildPieceSnapshot& Candidate : Candidates)
        {
            const bool bMatched = Other.ContainsByPredicate([&Candidate](const FWMPlacedBuildPieceSnapshot& OtherPiece)
            {
                return SamePlacedPiece(Candidate, OtherPiece);
            });
            if (!bMatched)
            {
                return &Candidate;
            }
        }
        return nullptr;
    }
}

void UWMVFXSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Collection.InitializeDependency<UWMBuildWorldStateSubsystem>();
    Collection.InitializeDependency<UWMEnvironmentStateSubsystem>();
    Super::Initialize(Collection);

    if (UWorld* World = GetWorld())
    {
        if (UWMBuildWorldStateSubsystem* BuildState = World->GetSubsystem<UWMBuildWorldStateSubsystem>())
        {
            LastBuildSnapshot = BuildState->GetPlacedPieces();
            BuildState->OnBuildWorldChanged.AddDynamic(this, &UWMVFXSubsystem::HandleBuildWorldChanged);
        }
        if (UWMEnvironmentStateSubsystem* EnvironmentState = World->GetSubsystem<UWMEnvironmentStateSubsystem>())
        {
            LastEnvironmentSnapshot = EnvironmentState->GetStateSnapshot();
            bHasEnvironmentSnapshot = LastEnvironmentSnapshot.IsBounded();
            EnvironmentState->OnEnvironmentStateChanged.AddDynamic(this, &UWMVFXSubsystem::HandleEnvironmentStateChanged);
        }
    }
}

void UWMVFXSubsystem::Deinitialize()
{
    if (UWorld* World = GetWorld())
    {
        if (UWMBuildWorldStateSubsystem* BuildState = World->GetSubsystem<UWMBuildWorldStateSubsystem>())
        {
            BuildState->OnBuildWorldChanged.RemoveDynamic(this, &UWMVFXSubsystem::HandleBuildWorldChanged);
        }
        if (UWMEnvironmentStateSubsystem* EnvironmentState = World->GetSubsystem<UWMEnvironmentStateSubsystem>())
        {
            EnvironmentState->OnEnvironmentStateChanged.RemoveDynamic(this, &UWMVFXSubsystem::HandleEnvironmentStateChanged);
        }
    }

    for (const TWeakObjectPtr<AWMProceduralVFXActor>& Effect : ActiveProxyEffects)
    {
        if (Effect.IsValid())
        {
            Effect->Destroy();
        }
    }
    ActiveProxyEffects.Reset();
    LastBuildSnapshot.Reset();
    Runtime = FWMVFXRuntime();
    LastAcceptedEventId = NAME_None;
    bHasEnvironmentSnapshot = false;
    Super::Deinitialize();
}

FWMVFXBudget UWMVFXSubsystem::ResolveBudget() const
{
    FWMVFXBudget Budget;
    const UWMVisualProfileSettings* VisualSettings = GetDefault<UWMVisualProfileSettings>();
    const EWMVisualQualityTier Tier = VisualSettings ? VisualSettings->DefaultQualityTier : EWMVisualQualityTier::Mid;

    switch (Tier)
    {
        case EWMVisualQualityTier::Low:
            Budget.MaxActiveProxyEffects = 8;
            Budget.MaxEventsPerSecond = 8;
            Budget.MaxDurationSeconds = 1.15f;
            Budget.MaxIntensity = 0.85f;
            break;
        case EWMVisualQualityTier::High:
            Budget.MaxActiveProxyEffects = 28;
            Budget.MaxEventsPerSecond = 28;
            Budget.MaxDurationSeconds = 2.0f;
            Budget.MaxIntensity = 1.0f;
            break;
        case EWMVisualQualityTier::Mid:
        default:
            Budget.MaxActiveProxyEffects = 16;
            Budget.MaxEventsPerSecond = 16;
            Budget.MaxDurationSeconds = 1.5f;
            Budget.MaxIntensity = 1.0f;
            break;
    }
    return Budget;
}

void UWMVFXSubsystem::PruneExpiredEffects()
{
    ActiveProxyEffects.RemoveAll([](const TWeakObjectPtr<AWMProceduralVFXActor>& Effect)
    {
        return !Effect.IsValid();
    });
}

int32 UWMVFXSubsystem::GetActiveProxyEffectCount() const
{
    int32 Count = 0;
    for (const TWeakObjectPtr<AWMProceduralVFXActor>& Effect : ActiveProxyEffects)
    {
        if (Effect.IsValid())
        {
            ++Count;
        }
    }
    return Count;
}

bool UWMVFXSubsystem::EmitSemanticEvent(
    const FName EventId,
    const FVector LocationCm,
    const float Intensity,
    const FVector Direction,
    const float DurationSeconds)
{
    FWMVFXEvent Event;
    Event.EventId = EventId;
    Event.LocationCm = LocationCm;
    Event.Direction = Direction.IsNearlyZero() ? FVector::ForwardVector : Direction.GetSafeNormal();
    Event.Intensity = FMath::Clamp(Intensity, 0.0f, 1.0f);
    Event.DurationSeconds = FMath::Clamp(DurationSeconds, 0.08f, 2.5f);
    Event.MotionScale = 1.0f;
    return EmitEvent(Event);
}

bool UWMVFXSubsystem::EmitEvent(const FWMVFXEvent& Event)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }

    PruneExpiredEffects();
    FWMVFXEvent Accepted = Event;
    const FWMVFXBudget Budget = ResolveBudget();
    if (!Runtime.TryAccept(Accepted, World->GetTimeSeconds(), GetActiveProxyEffectCount(), Budget, bReducedMotion))
    {
        return false;
    }

    FWMVFXStyle Style;
    if (!FWMVFXRuntime::ResolveStyle(Accepted.EventId, Style))
    {
        return false;
    }

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AWMProceduralVFXActor* Effect = World->SpawnActor<AWMProceduralVFXActor>(AWMProceduralVFXActor::StaticClass(), Accepted.LocationCm, FRotator::ZeroRotator, SpawnParameters);
    if (!Effect || !Effect->InitializeEffect(Accepted, Style))
    {
        if (Effect)
        {
            Effect->Destroy();
        }
        return false;
    }

    ActiveProxyEffects.Add(Effect);
    LastAcceptedEventId = Accepted.EventId;
    return true;
}

void UWMVFXSubsystem::HandleBuildWorldChanged(const int32 Revision)
{
    if (Revision <= 0 || !GetWorld()) return;
    UWMBuildWorldStateSubsystem* BuildState = GetWorld()->GetSubsystem<UWMBuildWorldStateSubsystem>();
    if (!BuildState) return;

    const TArray<FWMPlacedBuildPieceSnapshot> Current = BuildState->GetPlacedPieces();
    const FWMPlacedBuildPieceSnapshot* Added = FindUnmatched(Current, LastBuildSnapshot);
    const FWMPlacedBuildPieceSnapshot* Removed = FindUnmatched(LastBuildSnapshot, Current);

    if (Current.Num() > LastBuildSnapshot.Num() && Added)
    {
        EmitSemanticEvent(TEXT("gameplay.build.place"), Added->LocationCm, 0.85f, FVector::UpVector, 0.60f);
    }
    else if (Current.Num() < LastBuildSnapshot.Num() && Removed)
    {
        EmitSemanticEvent(TEXT("gameplay.build.remove"), Removed->LocationCm, 0.80f, FVector::UpVector, 0.55f);
    }
    else if (Added && Removed)
    {
        FVector Direction = Added->LocationCm - Removed->LocationCm;
        if (Direction.IsNearlyZero()) Direction = FVector::ForwardVector;
        EmitSemanticEvent(TEXT("gameplay.build.move"), Added->LocationCm, 0.70f, Direction, 0.55f);
    }

    LastBuildSnapshot = Current;
}

void UWMVFXSubsystem::HandleEnvironmentStateChanged(const FWMEnvironmentStateSnapshot Snapshot)
{
    if (!Snapshot.IsBounded()) return;
    if (!bHasEnvironmentSnapshot)
    {
        LastEnvironmentSnapshot = Snapshot;
        bHasEnvironmentSnapshot = true;
        return;
    }

    const float SignedChange =
        (Snapshot.VegetationHealth - LastEnvironmentSnapshot.VegetationHealth) +
        (Snapshot.WaterFlow - LastEnvironmentSnapshot.WaterFlow) +
        (Snapshot.SoilProtection - LastEnvironmentSnapshot.SoilProtection) +
        (Snapshot.ShadeCoverage - LastEnvironmentSnapshot.ShadeCoverage);

    LastEnvironmentSnapshot = Snapshot;
    if (FMath::IsNearlyZero(SignedChange) || !GetWorld()) return;

    FVector Location = FVector::ZeroVector;
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
    {
        if (APawn* Pawn = PC->GetPawn())
        {
            Location = Pawn->GetActorLocation() + FVector(0.0f, 0.0f, 30.0f);
        }
    }

    const FName EventId = SignedChange > 0.0f ? FName(TEXT("science.ecology.recovery")) : FName(TEXT("science.ecology.stress"));
    const float Intensity = FMath::Clamp(FMath::Abs(SignedChange) * 0.75f, 0.20f, 1.0f);
    EmitSemanticEvent(EventId, Location, Intensity, FVector::UpVector, 0.95f);
}
