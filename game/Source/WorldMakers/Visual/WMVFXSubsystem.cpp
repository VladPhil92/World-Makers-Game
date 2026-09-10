#include "Visual/WMVFXSubsystem.h"

#include "Engine/World.h"
#include "Visual/WMProceduralVFXActor.h"
#include "Visual/WMVisualProfileSettings.h"

void UWMVFXSubsystem::Deinitialize()
{
    for (const TWeakObjectPtr<AWMProceduralVFXActor>& Effect : ActiveProxyEffects)
    {
        if (Effect.IsValid())
        {
            Effect->Destroy();
        }
    }
    ActiveProxyEffects.Reset();
    Runtime = FWMVFXRuntime();
    LastAcceptedEventId = NAME_None;
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
