#pragma once

#include "CoreMinimal.h"

enum class EWMFirstPersonVisualMode : uint8
{
    Explore,
    Build,
    Scan,
    Measure,
    Observe
};

enum class EWMExplorerMotionPersonality : uint8
{
    CuriousExplorer,
    ScientistInventor,
    NatureGuardian,
    KnowledgeExplorer
};

struct WORLDMAKERS_API FWMFirstPersonVisualProfile
{
    EWMFirstPersonVisualMode Mode = EWMFirstPersonVisualMode::Explore;
    float FieldOfViewDegrees = 74.0f;
    FVector ToolOffsetCm = FVector(24.0f, 18.0f, -18.0f);
    FRotator ToolRotationDegrees = FRotator(-6.0f, -4.0f, 2.0f);
    FVector WristDeviceOffsetCm = FVector(18.0f, -16.0f, -14.0f);
    float CameraBobCm = 0.55f;
    float ToolLagDegrees = 1.5f;
    bool bToolVisible = false;
    bool bWristDeviceVisible = true;
    bool bCompassVisible = true;
    bool bHotbarVisible = true;
    bool bMissionSummaryVisible = true;
    bool bMinimapVisible = true;
    bool bContextPanelVisible = false;
    bool bBuildPaletteVisible = false;
    int32 MaxLargePanels = 2;

    bool IsSane() const;
};

struct WORLDMAKERS_API FWMExplorerMotionStyle
{
    EWMExplorerMotionPersonality Personality = EWMExplorerMotionPersonality::CuriousExplorer;
    float StrideScale = 1.0f;
    float ArmSwingScale = 1.0f;
    float HeadLookScale = 1.0f;
    float SettleSeconds = 0.18f;
    float SecondaryMotionScale = 0.8f;

    bool IsSane() const;
};

/**
 * Presentation-only interpretation of the approved World Makers visual references.
 * It never changes movement speed, collision, mission state, rewards or scientific outcomes.
 */
struct WORLDMAKERS_API FWMReferenceVisualPolishRuntime
{
    static bool ResolveFirstPersonProfile(EWMFirstPersonVisualMode Mode, bool bReducedMotion, FWMFirstPersonVisualProfile& OutProfile);
    static bool ResolveMotionStyle(EWMExplorerMotionPersonality Personality, bool bReducedMotion, FWMExplorerMotionStyle& OutStyle);
    static FName FirstPersonModeToId(EWMFirstPersonVisualMode Mode);
    static FName MotionPersonalityToId(EWMExplorerMotionPersonality Personality);
    static bool IsContextPanelAllowed(EWMFirstPersonVisualMode Mode, FName PanelId);

    static EWMFirstPersonVisualMode ResolveModeForSemanticEvent(const FName EventId)
    {
        const FString Id = EventId.ToString();
        if (Id.StartsWith(TEXT("gameplay.build."))) return EWMFirstPersonVisualMode::Build;
        if (Id == TEXT("mission.measure.reveal")) return EWMFirstPersonVisualMode::Measure;
        if (Id == TEXT("world.observe.reveal")) return EWMFirstPersonVisualMode::Observe;
        if (Id.StartsWith(TEXT("science."))) return EWMFirstPersonVisualMode::Scan;
        return EWMFirstPersonVisualMode::Explore;
    }
};
