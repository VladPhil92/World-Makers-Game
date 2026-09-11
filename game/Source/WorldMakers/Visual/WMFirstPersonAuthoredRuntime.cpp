#include "Visual/WMFirstPersonAuthoredRuntime.h"

bool FWMFirstPersonAuthoredAvailability::IsComplete() const
{
    return bArms && bScanner && bBuildTool && bMeasureTool && bWristDevice &&
        AnimationCount == FWMFirstPersonAuthoredRuntime::RequiredAnimationCount;
}

FString FWMFirstPersonAuthoredRuntime::ArmsAssetPath()
{
    return TEXT("/Game/WorldMakers/Characters/FirstPerson/SK_WM_FirstPersonArms.SK_WM_FirstPersonArms");
}

FString FWMFirstPersonAuthoredRuntime::WristAssetPath()
{
    return TEXT("/Game/WorldMakers/Tools/Wrist/SM_WM_WristDevice.SM_WM_WristDevice");
}

FString FWMFirstPersonAuthoredRuntime::ToolAssetPathForMode(const FName ModeId)
{
    if (ModeId == TEXT("firstperson.scan"))
    {
        return TEXT("/Game/WorldMakers/Tools/Scanner/SM_WM_Scanner.SM_WM_Scanner");
    }
    if (ModeId == TEXT("firstperson.build"))
    {
        return TEXT("/Game/WorldMakers/Tools/Build/SM_WM_BuildTool.SM_WM_BuildTool");
    }
    if (ModeId == TEXT("firstperson.measure"))
    {
        return TEXT("/Game/WorldMakers/Tools/Measure/SM_WM_MeasureTool.SM_WM_MeasureTool");
    }
    return FString();
}

FString FWMFirstPersonAuthoredRuntime::AnimationAssetPath(const FName ActionId)
{
    if (ActionId == TEXT("tool-raise")) return TEXT("/Game/WorldMakers/Animations/FirstPerson/A_FP_ToolRaise.A_FP_ToolRaise");
    if (ActionId == TEXT("tool-lower")) return TEXT("/Game/WorldMakers/Animations/FirstPerson/A_FP_ToolLower.A_FP_ToolLower");
    if (ActionId == TEXT("scan-anticipate")) return TEXT("/Game/WorldMakers/Animations/FirstPerson/A_FP_ScanAnticipate.A_FP_ScanAnticipate");
    if (ActionId == TEXT("scan-hold")) return TEXT("/Game/WorldMakers/Animations/FirstPerson/A_FP_ScanHold.A_FP_ScanHold");
    if (ActionId == TEXT("scan-settle")) return TEXT("/Game/WorldMakers/Animations/FirstPerson/A_FP_ScanSettle.A_FP_ScanSettle");
    if (ActionId == TEXT("build-point")) return TEXT("/Game/WorldMakers/Animations/FirstPerson/A_FP_BuildPoint.A_FP_BuildPoint");
    if (ActionId == TEXT("build-confirm")) return TEXT("/Game/WorldMakers/Animations/FirstPerson/A_FP_BuildConfirm.A_FP_BuildConfirm");
    if (ActionId == TEXT("measure-focus")) return TEXT("/Game/WorldMakers/Animations/FirstPerson/A_FP_MeasureFocus.A_FP_MeasureFocus");
    if (ActionId == TEXT("observe-focus")) return TEXT("/Game/WorldMakers/Animations/FirstPerson/A_FP_ObserveFocus.A_FP_ObserveFocus");
    return FString();
}

TArray<FName> FWMFirstPersonAuthoredRuntime::RequiredActionIds()
{
    return {
        TEXT("tool-raise"),
        TEXT("tool-lower"),
        TEXT("scan-anticipate"),
        TEXT("scan-hold"),
        TEXT("scan-settle"),
        TEXT("build-point"),
        TEXT("build-confirm"),
        TEXT("measure-focus"),
        TEXT("observe-focus"),
    };
}

bool FWMFirstPersonAuthoredRuntime::IsLoopingAction(const FName ActionId)
{
    return ActionId == TEXT("scan-hold");
}

bool FWMFirstPersonAuthoredRuntime::CanTakeOver(
    const FWMFirstPersonAuthoredAvailability& Availability,
    const bool bExplicitlyEnabled)
{
    return bExplicitlyEnabled && Availability.IsComplete();
}
