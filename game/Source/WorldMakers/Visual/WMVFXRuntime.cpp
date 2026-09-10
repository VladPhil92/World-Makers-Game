#include "Visual/WMVFXRuntime.h"

namespace
{
    bool IsFiniteVector(const FVector& Value)
    {
        return FMath::IsFinite(Value.X) && FMath::IsFinite(Value.Y) && FMath::IsFinite(Value.Z);
    }

    FLinearColor HexLike(const float R, const float G, const float B)
    {
        return FLinearColor(R, G, B, 1.0f);
    }
}

bool FWMVFXStyle::IsSane() const
{
    return Color.IsFinite() &&
        FMath::IsFinite(BaseRadiusCm) && BaseRadiusCm >= 5.0f && BaseRadiusCm <= 500.0f &&
        FMath::IsFinite(TravelCm) && TravelCm >= 0.0f && TravelCm <= 800.0f &&
        FMath::IsFinite(PulseCycles) && PulseCycles >= 0.0f && PulseCycles <= 6.0f;
}

bool FWMVFXEvent::IsSane() const
{
    return !EventId.IsNone() && IsFiniteVector(LocationCm) && IsFiniteVector(Direction) &&
        FMath::Abs(LocationCm.X) <= 10000000.0f && FMath::Abs(LocationCm.Y) <= 10000000.0f && FMath::Abs(LocationCm.Z) <= 10000000.0f &&
        FMath::IsFinite(Intensity) && Intensity >= 0.0f && Intensity <= 1.0f &&
        FMath::IsFinite(DurationSeconds) && DurationSeconds >= 0.08f && DurationSeconds <= 2.5f &&
        FMath::IsFinite(MotionScale) && MotionScale >= 0.0f && MotionScale <= 1.0f;
}

bool FWMVFXBudget::IsSane() const
{
    return MaxActiveProxyEffects >= 1 && MaxActiveProxyEffects <= 64 &&
        MaxEventsPerSecond >= 1 && MaxEventsPerSecond <= 60 &&
        FMath::IsFinite(MaxDurationSeconds) && MaxDurationSeconds >= 0.1f && MaxDurationSeconds <= 2.5f &&
        FMath::IsFinite(MaxIntensity) && MaxIntensity >= 0.1f && MaxIntensity <= 1.0f;
}

EWMVFXDomain FWMVFXRuntime::ResolveDomain(const FName EventId)
{
    const FString Id = EventId.ToString();
    if (Id.StartsWith(TEXT("science.chemistry."))) return EWMVFXDomain::Chemistry;
    if (Id.StartsWith(TEXT("science.physics."))) return EWMVFXDomain::Physics;
    if (Id.StartsWith(TEXT("science.biology."))) return EWMVFXDomain::Biology;
    if (Id.StartsWith(TEXT("science.ecology."))) return EWMVFXDomain::Ecology;
    if (Id.StartsWith(TEXT("fantasy."))) return EWMVFXDomain::Fantasy;
    return EWMVFXDomain::Gameplay;
}

bool FWMVFXRuntime::ResolveStyle(const FName EventId, FWMVFXStyle& OutStyle)
{
    OutStyle = FWMVFXStyle();
    OutStyle.Domain = ResolveDomain(EventId);
    const FString Id = EventId.ToString();

    if (Id == TEXT("gameplay.build.place"))
    {
        OutStyle.Shape = EWMVFXShape::Ring;
        OutStyle.Color = HexLike(0.388f, 0.831f, 0.773f);
        OutStyle.BaseRadiusCm = 42.0f;
        OutStyle.TravelCm = 90.0f;
    }
    else if (Id == TEXT("gameplay.build.remove"))
    {
        OutStyle.Shape = EWMVFXShape::Burst;
        OutStyle.Color = HexLike(0.941f, 0.643f, 0.235f);
        OutStyle.BaseRadiusCm = 36.0f;
        OutStyle.TravelCm = 55.0f;
    }
    else if (Id == TEXT("gameplay.build.move"))
    {
        OutStyle.Shape = EWMVFXShape::Directional;
        OutStyle.Color = HexLike(0.745f, 0.604f, 0.420f);
        OutStyle.BaseRadiusCm = 34.0f;
        OutStyle.TravelCm = 70.0f;
    }
    else if (Id == TEXT("mission.measure.reveal"))
    {
        OutStyle.Shape = EWMVFXShape::Ring;
        OutStyle.Color = HexLike(0.545f, 0.780f, 0.847f);
        OutStyle.BaseRadiusCm = 34.0f;
        OutStyle.TravelCm = 110.0f;
    }
    else if (Id == TEXT("world.observe.reveal"))
    {
        OutStyle.Shape = EWMVFXShape::Halo;
        OutStyle.Color = HexLike(1.0f, 0.890f, 0.690f);
        OutStyle.BaseRadiusCm = 48.0f;
        OutStyle.TravelCm = 45.0f;
        OutStyle.PulseCycles = 1.5f;
    }
    else if (Id == TEXT("science.chemistry.dissolution"))
    {
        OutStyle.Shape = EWMVFXShape::Halo;
        OutStyle.Color = HexLike(0.184f, 0.561f, 0.616f);
        OutStyle.BaseRadiusCm = 32.0f;
        OutStyle.TravelCm = 38.0f;
        OutStyle.PulseCycles = 2.0f;
    }
    else if (Id == TEXT("science.chemistry.saturation"))
    {
        OutStyle.Shape = EWMVFXShape::Ring;
        OutStyle.Color = HexLike(0.388f, 0.831f, 0.773f);
        OutStyle.BaseRadiusCm = 40.0f;
        OutStyle.TravelCm = 28.0f;
        OutStyle.PulseCycles = 2.0f;
    }
    else if (Id == TEXT("science.chemistry.filtration"))
    {
        OutStyle.Shape = EWMVFXShape::Directional;
        OutStyle.Color = HexLike(0.545f, 0.780f, 0.847f);
        OutStyle.BaseRadiusCm = 30.0f;
        OutStyle.TravelCm = 55.0f;
    }
    else if (Id == TEXT("science.chemistry.reaction"))
    {
        OutStyle.Shape = EWMVFXShape::Burst;
        OutStyle.Color = HexLike(0.941f, 0.643f, 0.235f);
        OutStyle.BaseRadiusCm = 42.0f;
        OutStyle.TravelCm = 95.0f;
    }
    else if (Id == TEXT("science.physics.force"))
    {
        OutStyle.Shape = EWMVFXShape::Directional;
        OutStyle.Color = HexLike(0.576f, 0.443f, 0.878f);
        OutStyle.BaseRadiusCm = 28.0f;
        OutStyle.TravelCm = 130.0f;
    }
    else if (Id == TEXT("science.physics.circuit-flow"))
    {
        OutStyle.Shape = EWMVFXShape::Ring;
        OutStyle.Color = HexLike(1.0f, 0.890f, 0.690f);
        OutStyle.BaseRadiusCm = 28.0f;
        OutStyle.TravelCm = 42.0f;
        OutStyle.PulseCycles = 2.5f;
    }
    else if (Id == TEXT("science.biology.cell-energy"))
    {
        OutStyle.Shape = EWMVFXShape::Halo;
        OutStyle.Color = HexLike(0.576f, 0.443f, 0.878f);
        OutStyle.BaseRadiusCm = 30.0f;
        OutStyle.TravelCm = 35.0f;
        OutStyle.PulseCycles = 2.0f;
    }
    else if (Id == TEXT("science.biology.plant-growth"))
    {
        OutStyle.Shape = EWMVFXShape::Ring;
        OutStyle.Color = HexLike(0.243f, 0.541f, 0.341f);
        OutStyle.BaseRadiusCm = 36.0f;
        OutStyle.TravelCm = 70.0f;
    }
    else if (Id == TEXT("science.ecology.recovery"))
    {
        OutStyle.Shape = EWMVFXShape::Halo;
        OutStyle.Color = HexLike(0.243f, 0.541f, 0.341f);
        OutStyle.BaseRadiusCm = 55.0f;
        OutStyle.TravelCm = 70.0f;
    }
    else if (Id == TEXT("science.ecology.stress"))
    {
        OutStyle.Shape = EWMVFXShape::Burst;
        OutStyle.Color = HexLike(0.941f, 0.643f, 0.235f);
        OutStyle.BaseRadiusCm = 44.0f;
        OutStyle.TravelCm = 45.0f;
    }
    else if (Id == TEXT("fantasy.portal.open"))
    {
        OutStyle.Shape = EWMVFXShape::Halo;
        OutStyle.Color = HexLike(0.576f, 0.443f, 0.878f);
        OutStyle.BaseRadiusCm = 85.0f;
        OutStyle.TravelCm = 75.0f;
        OutStyle.PulseCycles = 2.0f;
    }
    else if (Id == TEXT("fantasy.rune.activate"))
    {
        OutStyle.Shape = EWMVFXShape::Burst;
        OutStyle.Color = HexLike(0.576f, 0.443f, 0.878f);
        OutStyle.BaseRadiusCm = 38.0f;
        OutStyle.TravelCm = 70.0f;
    }
    else
    {
        return false;
    }

    return OutStyle.IsSane();
}

bool FWMVFXRuntime::TryAccept(
    FWMVFXEvent& InOutEvent,
    const double NowSeconds,
    const int32 ActiveProxyCount,
    const FWMVFXBudget& Budget,
    const bool bReducedMotion)
{
    if (!InOutEvent.IsSane() || !Budget.IsSane() || !FMath::IsFinite(NowSeconds) || NowSeconds < 0.0 || ActiveProxyCount < 0)
    {
        return false;
    }

    FWMVFXStyle Style;
    if (!ResolveStyle(InOutEvent.EventId, Style) || ActiveProxyCount >= Budget.MaxActiveProxyEffects)
    {
        return false;
    }

    if (WindowStartSeconds < 0.0 || (NowSeconds - WindowStartSeconds) >= 1.0)
    {
        WindowStartSeconds = NowSeconds;
        AcceptedInWindow = 0;
    }
    else if (NowSeconds < WindowStartSeconds)
    {
        WindowStartSeconds = NowSeconds;
        AcceptedInWindow = 0;
    }

    if (AcceptedInWindow >= Budget.MaxEventsPerSecond)
    {
        return false;
    }

    InOutEvent.Intensity = FMath::Clamp(InOutEvent.Intensity, 0.0f, Budget.MaxIntensity);
    InOutEvent.DurationSeconds = FMath::Clamp(InOutEvent.DurationSeconds, 0.08f, Budget.MaxDurationSeconds);
    InOutEvent.MotionScale = bReducedMotion ? FMath::Min(InOutEvent.MotionScale, 0.35f) : InOutEvent.MotionScale;
    ++AcceptedInWindow;
    return InOutEvent.IsSane();
}
