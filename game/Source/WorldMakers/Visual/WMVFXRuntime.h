#pragma once

#include "CoreMinimal.h"

enum class EWMVFXDomain : uint8
{
    Gameplay,
    Chemistry,
    Physics,
    Biology,
    Ecology,
    Fantasy
};

enum class EWMVFXShape : uint8
{
    Ring,
    Burst,
    Directional,
    Halo
};

struct WORLDMAKERS_API FWMVFXStyle
{
    EWMVFXDomain Domain = EWMVFXDomain::Gameplay;
    EWMVFXShape Shape = EWMVFXShape::Ring;
    FLinearColor Color = FLinearColor::White;
    float BaseRadiusCm = 45.0f;
    float TravelCm = 80.0f;
    float PulseCycles = 1.0f;

    bool IsSane() const;
};

/**
 * Presentation-only event. It contains no reward, progression or child-authored payload.
 * EventId is a stable semantic ID emitted only after an authoritative gameplay/simulation result.
 */
struct WORLDMAKERS_API FWMVFXEvent
{
    FName EventId = NAME_None;
    FVector LocationCm = FVector::ZeroVector;
    FVector Direction = FVector::ForwardVector;
    float Intensity = 1.0f;
    float DurationSeconds = 0.65f;
    float MotionScale = 1.0f;

    bool IsSane() const;
};

struct WORLDMAKERS_API FWMVFXBudget
{
    int32 MaxActiveProxyEffects = 16;
    int32 MaxEventsPerSecond = 16;
    float MaxDurationSeconds = 1.5f;
    float MaxIntensity = 1.0f;

    bool IsSane() const;
};

/** Pure deterministic acceptance/style layer used by the world subsystem and automation tests. */
struct WORLDMAKERS_API FWMVFXRuntime
{
    bool TryAccept(FWMVFXEvent& InOutEvent, double NowSeconds, int32 ActiveProxyCount, const FWMVFXBudget& Budget, bool bReducedMotion);

    static bool ResolveStyle(FName EventId, FWMVFXStyle& OutStyle);
    static EWMVFXDomain ResolveDomain(FName EventId);

private:
    double WindowStartSeconds = -1.0;
    int32 AcceptedInWindow = 0;
};
