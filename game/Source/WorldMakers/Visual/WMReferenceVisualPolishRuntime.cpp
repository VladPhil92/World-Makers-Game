#include "Visual/WMReferenceVisualPolishRuntime.h"

namespace
{
    bool IsFiniteVector(const FVector& Value)
    {
        return FMath::IsFinite(Value.X) && FMath::IsFinite(Value.Y) && FMath::IsFinite(Value.Z);
    }

    bool IsFiniteRotator(const FRotator& Value)
    {
        return FMath::IsFinite(Value.Pitch) && FMath::IsFinite(Value.Yaw) && FMath::IsFinite(Value.Roll);
    }
}

bool FWMFirstPersonVisualProfile::IsSane() const
{
    return FMath::IsFinite(FieldOfViewDegrees) && FieldOfViewDegrees >= 58.0f && FieldOfViewDegrees <= 84.0f &&
        IsFiniteVector(ToolOffsetCm) && ToolOffsetCm.Size() <= 80.0f &&
        IsFiniteRotator(ToolRotationDegrees) &&
        IsFiniteVector(WristDeviceOffsetCm) && WristDeviceOffsetCm.Size() <= 80.0f &&
        FMath::IsFinite(CameraBobCm) && CameraBobCm >= 0.0f && CameraBobCm <= 1.5f &&
        FMath::IsFinite(ToolLagDegrees) && ToolLagDegrees >= 0.0f && ToolLagDegrees <= 4.0f &&
        MaxLargePanels >= 0 && MaxLargePanels <= 2;
}

bool FWMExplorerMotionStyle::IsSane() const
{
    return FMath::IsFinite(StrideScale) && StrideScale >= 0.80f && StrideScale <= 1.15f &&
        FMath::IsFinite(ArmSwingScale) && ArmSwingScale >= 0.75f && ArmSwingScale <= 1.15f &&
        FMath::IsFinite(HeadLookScale) && HeadLookScale >= 0.80f && HeadLookScale <= 1.20f &&
        FMath::IsFinite(SettleSeconds) && SettleSeconds >= 0.10f && SettleSeconds <= 0.30f &&
        FMath::IsFinite(SecondaryMotionScale) && SecondaryMotionScale >= 0.0f && SecondaryMotionScale <= 1.0f;
}

bool FWMReferenceVisualPolishRuntime::ResolveFirstPersonProfile(
    const EWMFirstPersonVisualMode Mode,
    const bool bReducedMotion,
    FWMFirstPersonVisualProfile& OutProfile)
{
    OutProfile = FWMFirstPersonVisualProfile();
    OutProfile.Mode = Mode;

    switch (Mode)
    {
    case EWMFirstPersonVisualMode::Build:
        OutProfile.FieldOfViewDegrees = 76.0f;
        OutProfile.ToolOffsetCm = FVector(28.0f, 20.0f, -16.0f);
        OutProfile.ToolRotationDegrees = FRotator(-5.0f, -7.0f, 2.0f);
        OutProfile.CameraBobCm = 0.45f;
        OutProfile.ToolLagDegrees = 1.35f;
        OutProfile.bToolVisible = true;
        OutProfile.bWristDeviceVisible = false;
        OutProfile.bContextPanelVisible = true;
        OutProfile.bBuildPaletteVisible = true;
        break;
    case EWMFirstPersonVisualMode::Scan:
        OutProfile.FieldOfViewDegrees = 68.0f;
        OutProfile.ToolOffsetCm = FVector(24.0f, 17.0f, -15.0f);
        OutProfile.ToolRotationDegrees = FRotator(-3.0f, -4.0f, 1.0f);
        OutProfile.CameraBobCm = 0.25f;
        OutProfile.ToolLagDegrees = 0.85f;
        OutProfile.bToolVisible = true;
        OutProfile.bWristDeviceVisible = true;
        OutProfile.bContextPanelVisible = true;
        break;
    case EWMFirstPersonVisualMode::Measure:
        OutProfile.FieldOfViewDegrees = 70.0f;
        OutProfile.ToolOffsetCm = FVector(25.0f, 18.0f, -17.0f);
        OutProfile.ToolRotationDegrees = FRotator(-4.0f, -5.0f, 1.0f);
        OutProfile.CameraBobCm = 0.28f;
        OutProfile.ToolLagDegrees = 0.90f;
        OutProfile.bToolVisible = true;
        OutProfile.bWristDeviceVisible = true;
        OutProfile.bContextPanelVisible = true;
        break;
    case EWMFirstPersonVisualMode::Observe:
        OutProfile.FieldOfViewDegrees = 66.0f;
        OutProfile.CameraBobCm = 0.18f;
        OutProfile.ToolLagDegrees = 0.60f;
        OutProfile.bToolVisible = false;
        OutProfile.bWristDeviceVisible = true;
        OutProfile.bHotbarVisible = false;
        OutProfile.bContextPanelVisible = true;
        break;
    case EWMFirstPersonVisualMode::Explore:
    default:
        OutProfile.FieldOfViewDegrees = 74.0f;
        OutProfile.CameraBobCm = 0.55f;
        OutProfile.ToolLagDegrees = 1.50f;
        OutProfile.bToolVisible = false;
        OutProfile.bWristDeviceVisible = true;
        OutProfile.bContextPanelVisible = false;
        OutProfile.bBuildPaletteVisible = false;
        break;
    }

    if (bReducedMotion)
    {
        OutProfile.CameraBobCm = 0.0f;
        OutProfile.ToolLagDegrees = 0.0f;
    }

    return OutProfile.IsSane();
}

bool FWMReferenceVisualPolishRuntime::ResolveMotionStyle(
    const EWMExplorerMotionPersonality Personality,
    const bool bReducedMotion,
    FWMExplorerMotionStyle& OutStyle)
{
    OutStyle = FWMExplorerMotionStyle();
    OutStyle.Personality = Personality;

    switch (Personality)
    {
    case EWMExplorerMotionPersonality::ScientistInventor:
        OutStyle.StrideScale = 0.92f;
        OutStyle.ArmSwingScale = 0.88f;
        OutStyle.HeadLookScale = 0.92f;
        OutStyle.SettleSeconds = 0.15f;
        OutStyle.SecondaryMotionScale = 0.60f;
        break;
    case EWMExplorerMotionPersonality::NatureGuardian:
        OutStyle.StrideScale = 0.96f;
        OutStyle.ArmSwingScale = 0.82f;
        OutStyle.HeadLookScale = 1.00f;
        OutStyle.SettleSeconds = 0.22f;
        OutStyle.SecondaryMotionScale = 0.75f;
        break;
    case EWMExplorerMotionPersonality::KnowledgeExplorer:
        OutStyle.StrideScale = 1.00f;
        OutStyle.ArmSwingScale = 0.95f;
        OutStyle.HeadLookScale = 1.15f;
        OutStyle.SettleSeconds = 0.17f;
        OutStyle.SecondaryMotionScale = 0.70f;
        break;
    case EWMExplorerMotionPersonality::CuriousExplorer:
    default:
        OutStyle.StrideScale = 1.00f;
        OutStyle.ArmSwingScale = 1.05f;
        OutStyle.HeadLookScale = 1.10f;
        OutStyle.SettleSeconds = 0.18f;
        OutStyle.SecondaryMotionScale = 0.80f;
        break;
    }

    if (bReducedMotion)
    {
        OutStyle.ArmSwingScale = FMath::Min(OutStyle.ArmSwingScale, 0.82f);
        OutStyle.HeadLookScale = FMath::Min(OutStyle.HeadLookScale, 0.90f);
        OutStyle.SecondaryMotionScale = 0.0f;
        OutStyle.SettleSeconds = FMath::Min(OutStyle.SettleSeconds, 0.14f);
    }

    return OutStyle.IsSane();
}

FName FWMReferenceVisualPolishRuntime::FirstPersonModeToId(const EWMFirstPersonVisualMode Mode)
{
    switch (Mode)
    {
    case EWMFirstPersonVisualMode::Build: return TEXT("firstperson.build");
    case EWMFirstPersonVisualMode::Scan: return TEXT("firstperson.scan");
    case EWMFirstPersonVisualMode::Measure: return TEXT("firstperson.measure");
    case EWMFirstPersonVisualMode::Observe: return TEXT("firstperson.observe");
    case EWMFirstPersonVisualMode::Explore:
    default: return TEXT("firstperson.explore");
    }
}

FName FWMReferenceVisualPolishRuntime::MotionPersonalityToId(const EWMExplorerMotionPersonality Personality)
{
    switch (Personality)
    {
    case EWMExplorerMotionPersonality::ScientistInventor: return TEXT("scientist-inventor");
    case EWMExplorerMotionPersonality::NatureGuardian: return TEXT("nature-guardian");
    case EWMExplorerMotionPersonality::KnowledgeExplorer: return TEXT("knowledge-explorer");
    case EWMExplorerMotionPersonality::CuriousExplorer:
    default: return TEXT("curious-explorer");
    }
}

bool FWMReferenceVisualPolishRuntime::IsContextPanelAllowed(const EWMFirstPersonVisualMode Mode, const FName PanelId)
{
    if (PanelId.IsNone()) return false;
    if (PanelId == TEXT("hud.build-palette") || PanelId == TEXT("hud.snap-feedback"))
    {
        return Mode == EWMFirstPersonVisualMode::Build;
    }
    if (PanelId == TEXT("hud.science-panel"))
    {
        return Mode == EWMFirstPersonVisualMode::Scan || Mode == EWMFirstPersonVisualMode::Measure || Mode == EWMFirstPersonVisualMode::Observe;
    }
    if (PanelId == TEXT("hud.measurement-readout"))
    {
        return Mode == EWMFirstPersonVisualMode::Measure;
    }
    if (PanelId == TEXT("hud.object-inspection"))
    {
        return Mode == EWMFirstPersonVisualMode::Observe || Mode == EWMFirstPersonVisualMode::Scan;
    }
    return false;
}
