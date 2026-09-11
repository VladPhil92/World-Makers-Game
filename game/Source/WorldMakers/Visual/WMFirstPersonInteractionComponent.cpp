#include "Visual/WMFirstPersonInteractionComponent.h"

#include "Camera/CameraComponent.h"
#include "Components/ProceduralMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Player/WMPlayerCharacter.h"
#include "UI/WMFirstPersonContextWidget.h"
#include "Visual/WMPresentationSubsystem.h"

UWMFirstPersonInteractionComponent::UWMFirstPersonInteractionComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

void UWMFirstPersonInteractionComponent::BeginPlay()
{
    Super::BeginPlay();
    CharacterOwner = Cast<AWMPlayerCharacter>(GetOwner());
    if (!CharacterOwner || !CharacterOwner->IsLocallyControlled())
    {
        SetComponentTickEnabled(false);
        return;
    }

    EnsureViewModel();
    EnsureContextWidget();
    SetViewModelVisible(false);
}

void UWMFirstPersonInteractionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ExitFirstPersonInteraction();
    if (IsValid(ContextWidget))
    {
        ContextWidget->RemoveFromParent();
    }
    ContextWidget = nullptr;
    Super::EndPlay(EndPlayReason);
}

void UWMFirstPersonInteractionComponent::EnsureViewModel()
{
    if (!CharacterOwner || !CharacterOwner->FollowCamera || IsValid(ViewModelRoot)) return;

    ViewModelRoot = NewObject<USceneComponent>(CharacterOwner, TEXT("FirstPersonViewModelRoot"));
    CharacterOwner->AddInstanceComponent(ViewModelRoot);
    ViewModelRoot->SetupAttachment(CharacterOwner->FollowCamera);
    ViewModelRoot->SetRelativeLocation(FVector::ZeroVector);
    ViewModelRoot->RegisterComponent();

    UStaticMesh* SphereMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    UStaticMesh* CylinderMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));

    auto MakeProxy = [this](const FName Name, UStaticMesh* Mesh, const FVector Scale) -> UStaticMeshComponent*
    {
        if (!CharacterOwner || !ViewModelRoot) return nullptr;
        UStaticMeshComponent* Proxy = NewObject<UStaticMeshComponent>(CharacterOwner, Name);
        CharacterOwner->AddInstanceComponent(Proxy);
        Proxy->SetupAttachment(ViewModelRoot);
        Proxy->SetStaticMesh(Mesh);
        Proxy->SetRelativeScale3D(Scale);
        Proxy->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Proxy->SetCanEverAffectNavigation(false);
        Proxy->SetCastShadow(false);
        Proxy->SetOnlyOwnerSee(true);
        Proxy->RegisterComponent();
        return Proxy;
    };

    LeftHandProxy = MakeProxy(TEXT("FirstPersonLeftHandProxy"), SphereMesh, FVector(0.12f, 0.085f, 0.075f));
    RightHandProxy = MakeProxy(TEXT("FirstPersonRightHandProxy"), SphereMesh, FVector(0.12f, 0.085f, 0.075f));
    ToolProxy = MakeProxy(TEXT("FirstPersonToolProxy"), CylinderMesh, FVector(0.07f, 0.07f, 0.22f));
    WristDeviceProxy = MakeProxy(TEXT("FirstPersonWristDeviceProxy"), CubeMesh, FVector(0.10f, 0.055f, 0.035f));

    if (LeftHandProxy) LeftHandProxy->SetRelativeLocation(FVector(20.0f, -13.0f, -18.0f));
    if (RightHandProxy) RightHandProxy->SetRelativeLocation(FVector(22.0f, 12.0f, -18.0f));
}

void UWMFirstPersonInteractionComponent::EnsureContextWidget()
{
    if (IsValid(ContextWidget) || !CharacterOwner) return;
    APlayerController* PC = Cast<APlayerController>(CharacterOwner->GetController());
    if (!PC || !PC->IsLocalController()) return;

    ContextWidget = CreateWidget<UWMFirstPersonContextWidget>(PC, UWMFirstPersonContextWidget::StaticClass());
    if (ContextWidget)
    {
        ContextWidget->AddToPlayerScreen(95);
        ContextWidget->ClearContext();
    }
}

void UWMFirstPersonInteractionComponent::SetViewModelVisible(const bool bVisible)
{
    for (UStaticMeshComponent* Proxy : TArray<UStaticMeshComponent*>{LeftHandProxy, RightHandProxy, ToolProxy, WristDeviceProxy})
    {
        if (Proxy)
        {
            Proxy->SetVisibility(bVisible, true);
            Proxy->SetHiddenInGame(!bVisible, true);
        }
    }

    if (!CharacterOwner) return;
    if (CharacterOwner->GetMesh()) CharacterOwner->GetMesh()->SetOwnerNoSee(bVisible);
    for (UStaticMeshComponent* Part : TArray<UStaticMeshComponent*>{
        CharacterOwner->PrototypeBody, CharacterOwner->PrototypeHead, CharacterOwner->PrototypeLeftArm,
        CharacterOwner->PrototypeRightArm, CharacterOwner->PrototypeLeftLeg, CharacterOwner->PrototypeRightLeg})
    {
        if (Part) Part->SetOwnerNoSee(bVisible);
    }
    for (UProceduralMeshComponent* Part : TArray<UProceduralMeshComponent*>{
        CharacterOwner->AvatarSpineRig, CharacterOwner->AvatarHeadRig, CharacterOwner->AvatarHairArt,
        CharacterOwner->AvatarUpperArmLeftRig, CharacterOwner->AvatarLowerArmLeftRig, CharacterOwner->AvatarHandLeftRig,
        CharacterOwner->AvatarUpperArmRightRig, CharacterOwner->AvatarLowerArmRightRig, CharacterOwner->AvatarHandRightRig,
        CharacterOwner->AvatarThighLeftRig, CharacterOwner->AvatarCalfLeftRig, CharacterOwner->AvatarFootLeftRig,
        CharacterOwner->AvatarThighRightRig, CharacterOwner->AvatarCalfRightRig, CharacterOwner->AvatarFootRightRig})
    {
        if (Part) Part->SetOwnerNoSee(bVisible);
    }
}

bool UWMFirstPersonInteractionComponent::TryParseMode(const FName ModeId, EWMFirstPersonVisualMode& OutMode) const
{
    if (ModeId == TEXT("firstperson.explore")) { OutMode = EWMFirstPersonVisualMode::Explore; return true; }
    if (ModeId == TEXT("firstperson.build")) { OutMode = EWMFirstPersonVisualMode::Build; return true; }
    if (ModeId == TEXT("firstperson.scan")) { OutMode = EWMFirstPersonVisualMode::Scan; return true; }
    if (ModeId == TEXT("firstperson.measure")) { OutMode = EWMFirstPersonVisualMode::Measure; return true; }
    if (ModeId == TEXT("firstperson.observe")) { OutMode = EWMFirstPersonVisualMode::Observe; return true; }
    return false;
}

bool UWMFirstPersonInteractionComponent::ActivateMode(
    const EWMFirstPersonVisualMode Mode,
    const EWMFirstPersonInteractionAction Action,
    const float DurationSeconds,
    const bool bPersistent)
{
    if (!CharacterOwner || !CharacterOwner->IsLocallyControlled()) return false;
    const bool bReducedMotion = ResolveReducedMotion();
    if (!FWMReferenceVisualPolishRuntime::ResolveFirstPersonProfile(Mode, bReducedMotion, ActiveProfile)) return false;

    EnsureViewModel();
    EnsureContextWidget();

    ActiveMode = Mode;
    bModeActive = true;
    bPersistentMode = bPersistent;
    ModeRemainingSeconds = bPersistent ? 0.0f : FMath::Clamp(FMath::IsFinite(DurationSeconds) ? DurationSeconds : 0.85f, 0.25f, 3.0f);
    Runtime.Trigger(Action, bPersistent ? 1.20f : ModeRemainingSeconds);

    if (UWorld* World = GetWorld())
    {
        if (UWMPresentationSubsystem* Presentation = World->GetSubsystem<UWMPresentationSubsystem>())
        {
            Presentation->SetFirstPersonInteractionMode(ActiveMode, true);
        }
    }

    SetViewModelVisible(true);
    if (ToolProxy) ToolProxy->SetVisibility(ActiveProfile.bToolVisible, true);
    if (WristDeviceProxy) WristDeviceProxy->SetVisibility(ActiveProfile.bWristDeviceVisible, true);
    if (ContextWidget)
    {
        ContextWidget->SetContext(FWMReferenceVisualPolishRuntime::FirstPersonModeToId(ActiveMode), Runtime.GetActionId(), bReducedMotion);
    }
    return true;
}

bool UWMFirstPersonInteractionComponent::PulseSemanticEvent(const FName EventId, const FName ActionId, const float DurationSeconds)
{
    const EWMFirstPersonVisualMode Mode = FWMReferenceVisualPolishRuntime::ResolveModeForSemanticEvent(EventId);
    if (Mode == EWMFirstPersonVisualMode::Explore && !EventId.IsNone())
    {
        return false;
    }

    EWMFirstPersonInteractionAction Action;
    if (!FWMFirstPersonInteractionRuntime::TryParseAction(ActionId, Action))
    {
        Action = FWMFirstPersonInteractionRuntime::DefaultActionForModeId(FWMReferenceVisualPolishRuntime::FirstPersonModeToId(Mode));
    }
    return ActivateMode(Mode, Action, DurationSeconds, false);
}

bool UWMFirstPersonInteractionComponent::SetPersistentModeById(const FName ModeId, const bool bEnabled)
{
    if (!bEnabled)
    {
        ExitFirstPersonInteraction();
        return true;
    }

    EWMFirstPersonVisualMode Mode;
    if (!TryParseMode(ModeId, Mode)) return false;
    const EWMFirstPersonInteractionAction Action = FWMFirstPersonInteractionRuntime::DefaultActionForModeId(ModeId);
    return ActivateMode(Mode, Action, 1.20f, true);
}

void UWMFirstPersonInteractionComponent::ExitFirstPersonInteraction()
{
    if (!bModeActive && !CharacterOwner) return;

    bModeActive = false;
    bPersistentMode = false;
    ModeRemainingSeconds = 0.0f;
    Runtime.Reset();

    if (UWorld* World = GetWorld())
    {
        if (UWMPresentationSubsystem* Presentation = World->GetSubsystem<UWMPresentationSubsystem>())
        {
            Presentation->SetFirstPersonInteractionMode(ActiveMode, false);
        }
    }

    SetViewModelVisible(false);
    if (ContextWidget) ContextWidget->ClearContext();
    ActiveMode = EWMFirstPersonVisualMode::Explore;
}

bool UWMFirstPersonInteractionComponent::ResolveReducedMotion() const
{
    if (const UWorld* World = GetWorld())
    {
        if (const UWMPresentationSubsystem* Presentation = World->GetSubsystem<UWMPresentationSubsystem>())
        {
            return Presentation->IsReducedMotion();
        }
    }
    return false;
}

void UWMFirstPersonInteractionComponent::ApplyViewModelPose(const FWMFirstPersonInteractionPose& Pose)
{
    if (!bModeActive || !Pose.IsSane()) return;

    const FVector RootBob(0.0f, 0.0f, Pose.ViewModelBobCm);
    if (ViewModelRoot) ViewModelRoot->SetRelativeLocation(RootBob);

    if (ToolProxy)
    {
        ToolProxy->SetRelativeLocation(ActiveProfile.ToolOffsetCm + Pose.ToolTranslationCm);
        ToolProxy->SetRelativeRotation(ActiveProfile.ToolRotationDegrees + Pose.ToolRotationDegrees);
        ToolProxy->SetRelativeScale3D(FVector(0.07f, 0.07f, 0.22f) * Pose.ToolScale);
        ToolProxy->SetVisibility(ActiveProfile.bToolVisible, true);
    }
    if (WristDeviceProxy)
    {
        WristDeviceProxy->SetRelativeLocation(ActiveProfile.WristDeviceOffsetCm + Pose.LeftHandTranslationCm * 0.35f);
        WristDeviceProxy->SetRelativeRotation(FRotator(8.0f, 14.0f, -10.0f));
        WristDeviceProxy->SetVisibility(ActiveProfile.bWristDeviceVisible, true);
    }
    if (LeftHandProxy)
    {
        LeftHandProxy->SetRelativeLocation(FVector(20.0f, -13.0f, -18.0f) + Pose.LeftHandTranslationCm);
        LeftHandProxy->SetRelativeRotation(FRotator(-10.0f, 10.0f, -15.0f));
    }
    if (RightHandProxy)
    {
        RightHandProxy->SetRelativeLocation(FVector(22.0f, 12.0f, -18.0f) + Pose.RightHandTranslationCm);
        RightHandProxy->SetRelativeRotation(FRotator(-8.0f, -8.0f, 12.0f));
    }
    if (ContextWidget) ContextWidget->SetContextAlpha(Pose.ContextAlpha);
}

void UWMFirstPersonInteractionComponent::TickComponent(
    const float DeltaTime,
    const ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (!bModeActive || !FMath::IsFinite(DeltaTime) || DeltaTime <= 0.0f) return;

    const bool bReducedMotion = ResolveReducedMotion();
    const FWMFirstPersonInteractionPose Pose = Runtime.Update(DeltaTime, bReducedMotion);
    ApplyViewModelPose(Pose);

    if (bPersistentMode)
    {
        if (!Runtime.IsActive())
        {
            const EWMFirstPersonInteractionAction HoldAction = FWMFirstPersonInteractionRuntime::DefaultActionForModeId(
                FWMReferenceVisualPolishRuntime::FirstPersonModeToId(ActiveMode));
            Runtime.Trigger(HoldAction, 1.20f);
        }
        return;
    }

    ModeRemainingSeconds = FMath::Max(0.0f, ModeRemainingSeconds - DeltaTime);
    if (ModeRemainingSeconds <= 0.0f)
    {
        ExitFirstPersonInteraction();
    }
}
