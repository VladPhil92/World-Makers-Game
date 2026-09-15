#include "Input/WMInputReadinessLibrary.h"

namespace
{
FWMTouchControlSpec MakeTouchControl(
    const TCHAR* Id,
    EWMInputSemantic Semantic,
    FVector2D Center,
    FVector2D Size,
    bool bAnalog)
{
    FWMTouchControlSpec Spec;
    Spec.ControlId = FName(Id);
    Spec.Semantic = Semantic;
    Spec.NormalizedCenter = Center;
    Spec.NormalizedSize = Size;
    Spec.bAnalog = bAnalog;
    return Spec;
}
}

TArray<FName> UWMInputReadinessLibrary::GetCanonicalActionIds()
{
    return {
        TEXT("input.move"),
        TEXT("input.look"),
        TEXT("input.jump"),
        TEXT("input.interact"),
        TEXT("input.observe"),
        TEXT("input.build.place"),
        TEXT("input.build.rotate"),
        TEXT("input.build.remove"),
        TEXT("input.build.move"),
        TEXT("input.build.cycle"),
        TEXT("input.cancel"),
        TEXT("input.undo"),
        TEXT("input.redo"),
        TEXT("input.measure"),
        TEXT("input.mission.cycle"),
        TEXT("input.pause")
    };
}

TArray<FWMTouchControlSpec> UWMInputReadinessLibrary::GetDefaultTabletTouchLayout()
{
    return {
        MakeTouchControl(TEXT("touch.move"), EWMInputSemantic::Move, FVector2D(0.16f, 0.76f), FVector2D(0.28f, 0.38f), true),
        MakeTouchControl(TEXT("touch.look"), EWMInputSemantic::Look, FVector2D(0.72f, 0.52f), FVector2D(0.50f, 0.62f), true),
        MakeTouchControl(TEXT("touch.interact"), EWMInputSemantic::Interact, FVector2D(0.86f, 0.71f), FVector2D(0.11f, 0.16f), false),
        MakeTouchControl(TEXT("touch.jump"), EWMInputSemantic::Jump, FVector2D(0.88f, 0.86f), FVector2D(0.10f, 0.14f), false),
        MakeTouchControl(TEXT("touch.observe"), EWMInputSemantic::Observe, FVector2D(0.74f, 0.80f), FVector2D(0.10f, 0.14f), false),
        MakeTouchControl(TEXT("touch.build-primary"), EWMInputSemantic::PlaceBuild, FVector2D(0.87f, 0.56f), FVector2D(0.11f, 0.15f), false),
        MakeTouchControl(TEXT("touch.pause"), EWMInputSemantic::Pause, FVector2D(0.95f, 0.08f), FVector2D(0.07f, 0.10f), false)
    };
}

FName UWMInputReadinessLibrary::ToCanonicalActionId(EWMInputSemantic Semantic)
{
    const TArray<FName> Ids = GetCanonicalActionIds();
    const int32 Index = static_cast<int32>(Semantic);
    return Ids.IsValidIndex(Index) ? Ids[Index] : NAME_None;
}
