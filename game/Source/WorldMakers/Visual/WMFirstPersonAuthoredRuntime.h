#pragma once

#include "CoreMinimal.h"

struct WORLDMAKERS_API FWMFirstPersonAuthoredAvailability
{
    bool bArms = false;
    bool bScanner = false;
    bool bBuildTool = false;
    bool bMeasureTool = false;
    bool bWristDevice = false;
    int32 AnimationCount = 0;

    bool IsComplete() const;
};

/** Stable asset/path contract between the source-proxy kit and native authored first-person assets. */
struct WORLDMAKERS_API FWMFirstPersonAuthoredRuntime
{
    static constexpr int32 RequiredAnimationCount = 9;

    static FString ArmsAssetPath();
    static FString WristAssetPath();
    static FString ToolAssetPathForMode(FName ModeId);
    static FString AnimationAssetPath(FName ActionId);
    static TArray<FName> RequiredActionIds();
    static bool IsLoopingAction(FName ActionId);
    static bool CanTakeOver(const FWMFirstPersonAuthoredAvailability& Availability, bool bExplicitlyEnabled);
};
