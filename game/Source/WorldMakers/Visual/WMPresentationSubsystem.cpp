#include "Visual/WMPresentationSubsystem.h"

#include "Blueprint/UserWidget.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Mission/WMMissionRuntimeSubsystem.h"
#include "UI/WMPresentationOverlayWidget.h"
#include "Visual/WMVFXSubsystem.h"

void UWMPresentationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Collection.InitializeDependency<UWMVFXSubsystem>();
    Super::Initialize(Collection);

    FWMPresentationRuntime::ResolveCameraProfile(EWMPresentationCameraMode::Explore, false, ActiveProfile);
    if (UWorld* World = GetWorld())
    {
        if (UWMVFXSubsystem* VFX = World->GetSubsystem<UWMVFXSubsystem>())
        {
            VFX->OnVFXAccepted.AddDynamic(this, &UWMPresentationSubsystem::HandleVFXAccepted);
        }
    }
}

void UWMPresentationSubsystem::Deinitialize()
{
    if (UWorld* World = GetWorld())
    {
        if (UWMVFXSubsystem* VFX = World->GetSubsystem<UWMVFXSubsystem>())
        {
            VFX->OnVFXAccepted.RemoveDynamic(this, &UWMPresentationSubsystem::HandleVFXAccepted);
        }
    }
    if (IsValid(Overlay))
    {
        Overlay->RemoveFromParent();
    }
    Overlay = nullptr;
    CameraBoom = nullptr;
    Camera = nullptr;
    Super::Deinitialize();
}

TStatId UWMPresentationSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UWMPresentationSubsystem, STATGROUP_Tickables);
}

void UWMPresentationSubsystem::EnsurePresentationTargets()
{
    if (IsValid(CameraBoom) && IsValid(Camera) && IsValid(Overlay)) return;
    UWorld* World = GetWorld();
    APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
    APawn* Pawn = PC ? PC->GetPawn() : nullptr;
    if (!Pawn) return;

    if (!IsValid(CameraBoom)) CameraBoom = Pawn->FindComponentByClass<USpringArmComponent>();
    if (!IsValid(Camera)) Camera = Pawn->FindComponentByClass<UCameraComponent>();

    if (!IsValid(Overlay) && PC && PC->IsLocalController())
    {
        Overlay = CreateWidget<UWMPresentationOverlayWidget>(PC, UWMPresentationOverlayWidget::StaticClass());
        if (Overlay)
        {
            Overlay->AddToPlayerScreen(80);
        }
    }

    if (IsValid(CameraBoom))
    {
        CameraBoom->bDoCollisionTest = true;
        CameraBoom->ProbeSize = FMath::Max(CameraBoom->ProbeSize, 12.0f);
        CameraBoom->bEnableCameraLag = false;
        CameraBoom->bEnableCameraRotationLag = false;
    }
}

void UWMPresentationSubsystem::Tick(const float DeltaTime)
{
    if (!FMath::IsFinite(DeltaTime) || DeltaTime <= 0.0f) return;
    EnsurePresentationTargets();
    UpdateMissionReveal();

    if (PulseRemainingSeconds > 0.0f)
    {
        PulseRemainingSeconds = FMath::Max(0.0f, PulseRemainingSeconds - DeltaTime);
        if (PulseRemainingSeconds <= 0.0f)
        {
            FWMPresentationRuntime::ResolveCameraProfile(EWMPresentationCameraMode::Explore, bReducedMotion, ActiveProfile);
        }
    }
    ApplyCameraProfile(DeltaTime);
}

void UWMPresentationSubsystem::ApplyCameraProfile(const float DeltaTime)
{
    if (!IsValid(CameraBoom) || !IsValid(Camera) || !ActiveProfile.IsSane()) return;

    const float Blend = FMath::Max(ActiveProfile.BlendSeconds, 0.01f);
    const float InterpSpeed = 4.0f / Blend;
    CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, ActiveProfile.ArmLengthCm, DeltaTime, InterpSpeed);
    CameraBoom->TargetOffset = FMath::VInterpTo(CameraBoom->TargetOffset, ActiveProfile.TargetOffsetCm, DeltaTime, InterpSpeed);
    Camera->SetFieldOfView(FMath::FInterpTo(Camera->FieldOfView, ActiveProfile.FieldOfViewDegrees, DeltaTime, InterpSpeed));
}

bool UWMPresentationSubsystem::PulseCameraMode(
    const EWMPresentationCameraMode Mode,
    const float DurationSeconds,
    const FName CueId)
{
    FWMPresentationCameraProfile Profile;
    if (!FWMPresentationRuntime::ResolveCameraProfile(Mode, bReducedMotion, Profile)) return false;

    ActiveProfile = Profile;
    PulseRemainingSeconds = FWMPresentationRuntime::ClampPulseDuration(
        DurationSeconds > 0.0f ? DurationSeconds : Profile.HoldSeconds,
        bReducedMotion);

    if (IsValid(Overlay) && !CueId.IsNone())
    {
        Overlay->ShowCue(CueId, PulseRemainingSeconds, bReducedMotion);
    }
    return true;
}

void UWMPresentationSubsystem::SetReducedMotion(const bool bEnabled)
{
    bReducedMotion = bEnabled;
    if (UWorld* World = GetWorld())
    {
        if (UWMVFXSubsystem* VFX = World->GetSubsystem<UWMVFXSubsystem>())
        {
            VFX->SetReducedMotion(bEnabled);
        }
    }
    FWMPresentationRuntime::ResolveCameraProfile(EWMPresentationCameraMode::Explore, bReducedMotion, ActiveProfile);
    PulseRemainingSeconds = 0.0f;
    if (IsValid(Overlay)) Overlay->DismissCue();
}

FName UWMPresentationSubsystem::ResolveCueForEvent(const FName EventId) const
{
    const FString Id = EventId.ToString();
    if (Id.StartsWith(TEXT("gameplay.build."))) return TEXT("presentation.build.confirm");
    if (Id == TEXT("world.observe.reveal")) return TEXT("presentation.observe.focus");
    if (Id.StartsWith(TEXT("science.ecology."))) return TEXT("presentation.ecology.changed");
    if (Id.StartsWith(TEXT("science.")) || Id == TEXT("mission.measure.reveal")) return TEXT("presentation.science.focus");
    if (Id.StartsWith(TEXT("fantasy."))) return TEXT("presentation.adventure.reveal");
    return NAME_None;
}

void UWMPresentationSubsystem::HandleVFXAccepted(const FName EventId, const FVector LocationCm, const float Intensity)
{
    if (EventId.IsNone() || LocationCm.ContainsNaN() || !FMath::IsFinite(Intensity)) return;
    const EWMPresentationCameraMode Mode = FWMPresentationRuntime::ResolveModeForSemanticEvent(EventId);
    if (Mode == EWMPresentationCameraMode::Explore) return;

    FWMPresentationCameraProfile Profile;
    if (!FWMPresentationRuntime::ResolveCameraProfile(Mode, bReducedMotion, Profile)) return;
    const float Duration = FMath::Clamp(Profile.HoldSeconds + FMath::Clamp(Intensity, 0.0f, 1.0f) * 0.25f, 0.25f, 2.2f);
    PulseCameraMode(Mode, Duration, ResolveCueForEvent(EventId));
}

void UWMPresentationSubsystem::UpdateMissionReveal()
{
    UWorld* World = GetWorld();
    UWMMissionRuntimeSubsystem* Missions = World ? World->GetSubsystem<UWMMissionRuntimeSubsystem>() : nullptr;
    if (!Missions) return;

    const FName ActiveMission = Missions->GetActiveMissionId();
    if (ActiveMission.IsNone()) return;
    if (LastMissionId.IsNone())
    {
        LastMissionId = ActiveMission;
        return;
    }
    if (ActiveMission != LastMissionId)
    {
        LastMissionId = ActiveMission;
        PulseCameraMode(EWMPresentationCameraMode::AdventureReveal, 1.45f, TEXT("presentation.mission.changed"));
    }
}
