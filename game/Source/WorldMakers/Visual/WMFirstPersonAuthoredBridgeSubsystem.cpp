#include "Visual/WMFirstPersonAuthoredBridgeSubsystem.h"

#include "Animation/AnimationAsset.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Player/WMPlayerCharacter.h"
#include "UObject/SoftObjectPath.h"
#include "Visual/WMFirstPersonInteractionComponent.h"
#include "Visual/WMPresentationSubsystem.h"

namespace
{
    template <typename TObjectType>
    TObjectType* LoadAuthoredObject(const FString& ObjectPath)
    {
        if (ObjectPath.IsEmpty()) return nullptr;
        return Cast<TObjectType>(FSoftObjectPath(ObjectPath).TryLoad());
    }
}

void UWMFirstPersonAuthoredBridgeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Collection.InitializeDependency<UWMPresentationSubsystem>();
    Super::Initialize(Collection);
    bExplicitTakeoverEnabled = FParse::Param(FCommandLine::Get(), TEXT("WMEnableFirstPersonAuthored"));
}

void UWMFirstPersonAuthoredBridgeSubsystem::Deinitialize()
{
    ApplyProxyFallback();
    Animations.Empty();
    Character = nullptr;
    Interaction = nullptr;
    AuthoredArmsComponent = nullptr;
    LeftHandProxy = nullptr;
    RightHandProxy = nullptr;
    ToolProxy = nullptr;
    WristProxy = nullptr;
    ArmsAsset = nullptr;
    ScannerAsset = nullptr;
    BuildToolAsset = nullptr;
    MeasureToolAsset = nullptr;
    WristAsset = nullptr;
    ProxyToolFallback = nullptr;
    ProxyWristFallback = nullptr;
    Super::Deinitialize();
}

TStatId UWMFirstPersonAuthoredBridgeSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UWMFirstPersonAuthoredBridgeSubsystem, STATGROUP_Tickables);
}

bool UWMFirstPersonAuthoredBridgeSubsystem::IsAuthoredTakeoverReady() const
{
    return FWMFirstPersonAuthoredRuntime::CanTakeOver(Availability, bExplicitTakeoverEnabled);
}

UStaticMeshComponent* UWMFirstPersonAuthoredBridgeSubsystem::FindStaticMeshComponent(const FName ComponentName) const
{
    if (!Character) return nullptr;
    TArray<UStaticMeshComponent*> Components;
    Character->GetComponents<UStaticMeshComponent>(Components);
    for (UStaticMeshComponent* Component : Components)
    {
        if (Component && Component->GetFName() == ComponentName)
        {
            return Component;
        }
    }
    return nullptr;
}

void UWMFirstPersonAuthoredBridgeSubsystem::EnsureTargets()
{
    if (IsValid(Character) && IsValid(Interaction) && IsValid(AuthoredArmsComponent) &&
        IsValid(LeftHandProxy) && IsValid(RightHandProxy) && IsValid(ToolProxy) && IsValid(WristProxy))
    {
        return;
    }

    UWorld* World = GetWorld();
    APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
    AWMPlayerCharacter* NewCharacter = PC ? Cast<AWMPlayerCharacter>(PC->GetPawn()) : nullptr;
    if (!NewCharacter || !NewCharacter->IsLocallyControlled()) return;

    if (Character != NewCharacter)
    {
        Character = NewCharacter;
        Interaction = nullptr;
        AuthoredArmsComponent = nullptr;
        LeftHandProxy = nullptr;
        RightHandProxy = nullptr;
        ToolProxy = nullptr;
        WristProxy = nullptr;
        ProxyToolFallback = nullptr;
        ProxyWristFallback = nullptr;
        LastPlayedActionId = NAME_None;
    }

    if (!IsValid(Interaction))
    {
        Interaction = Character->FindComponentByClass<UWMFirstPersonInteractionComponent>();
    }
    if (!IsValid(Interaction)) return;

    if (!IsValid(LeftHandProxy)) LeftHandProxy = FindStaticMeshComponent(TEXT("FirstPersonLeftHandProxy"));
    if (!IsValid(RightHandProxy)) RightHandProxy = FindStaticMeshComponent(TEXT("FirstPersonRightHandProxy"));
    if (!IsValid(ToolProxy)) ToolProxy = FindStaticMeshComponent(TEXT("FirstPersonToolProxy"));
    if (!IsValid(WristProxy)) WristProxy = FindStaticMeshComponent(TEXT("FirstPersonWristDeviceProxy"));

    if (ToolProxy && !ProxyToolFallback) ProxyToolFallback = ToolProxy->GetStaticMesh();
    if (WristProxy && !ProxyWristFallback) ProxyWristFallback = WristProxy->GetStaticMesh();

    if (!IsValid(AuthoredArmsComponent) && Character->FollowCamera)
    {
        AuthoredArmsComponent = NewObject<USkeletalMeshComponent>(Character, TEXT("FirstPersonAuthoredArms"));
        Character->AddInstanceComponent(AuthoredArmsComponent);
        AuthoredArmsComponent->SetupAttachment(Character->FollowCamera);
        AuthoredArmsComponent->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
        AuthoredArmsComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        AuthoredArmsComponent->SetCanEverAffectNavigation(false);
        AuthoredArmsComponent->SetCastShadow(false);
        AuthoredArmsComponent->SetOnlyOwnerSee(true);
        AuthoredArmsComponent->SetVisibility(false, true);
        AuthoredArmsComponent->SetHiddenInGame(true, true);
        AuthoredArmsComponent->RegisterComponent();
    }
}

void UWMFirstPersonAuthoredBridgeSubsystem::TryLoadAuthoredAssets()
{
    if (bLoadAttempted) return;
    bLoadAttempted = true;

    ArmsAsset = LoadAuthoredObject<USkeletalMesh>(FWMFirstPersonAuthoredRuntime::ArmsAssetPath());
    ScannerAsset = LoadAuthoredObject<UStaticMesh>(FWMFirstPersonAuthoredRuntime::ToolAssetPathForMode(TEXT("firstperson.scan")));
    BuildToolAsset = LoadAuthoredObject<UStaticMesh>(FWMFirstPersonAuthoredRuntime::ToolAssetPathForMode(TEXT("firstperson.build")));
    MeasureToolAsset = LoadAuthoredObject<UStaticMesh>(FWMFirstPersonAuthoredRuntime::ToolAssetPathForMode(TEXT("firstperson.measure")));
    WristAsset = LoadAuthoredObject<UStaticMesh>(FWMFirstPersonAuthoredRuntime::WristAssetPath());

    Availability.bArms = ArmsAsset != nullptr;
    Availability.bScanner = ScannerAsset != nullptr;
    Availability.bBuildTool = BuildToolAsset != nullptr;
    Availability.bMeasureTool = MeasureToolAsset != nullptr;
    Availability.bWristDevice = WristAsset != nullptr;

    for (const FName ActionId : FWMFirstPersonAuthoredRuntime::RequiredActionIds())
    {
        if (UAnimationAsset* Animation = LoadAuthoredObject<UAnimationAsset>(FWMFirstPersonAuthoredRuntime::AnimationAssetPath(ActionId)))
        {
            Animations.Add(ActionId, Animation);
        }
    }
    Availability.AnimationCount = Animations.Num();
}

UStaticMesh* UWMFirstPersonAuthoredBridgeSubsystem::ResolveAuthoredTool(const FName ModeId) const
{
    if (ModeId == TEXT("firstperson.scan")) return ScannerAsset;
    if (ModeId == TEXT("firstperson.build")) return BuildToolAsset;
    if (ModeId == TEXT("firstperson.measure")) return MeasureToolAsset;
    return nullptr;
}

void UWMFirstPersonAuthoredBridgeSubsystem::PlayActionIfChanged(const FName ActionId)
{
    if (!AuthoredArmsComponent || ActionId.IsNone() || ActionId == LastPlayedActionId) return;
    if (UAnimationAsset* const* Animation = Animations.Find(ActionId))
    {
        AuthoredArmsComponent->PlayAnimation(*Animation, FWMFirstPersonAuthoredRuntime::IsLoopingAction(ActionId));
        LastPlayedActionId = ActionId;
    }
}

void UWMFirstPersonAuthoredBridgeSubsystem::ApplyAuthoredTakeover()
{
    if (!Interaction || !Interaction->IsFirstPersonInteractionActive() || !IsAuthoredTakeoverReady())
    {
        ApplyProxyFallback();
        return;
    }

    if (!AuthoredArmsComponent || !ArmsAsset || !ToolProxy || !WristProxy || !LeftHandProxy || !RightHandProxy)
    {
        ApplyProxyFallback();
        return;
    }

    AuthoredArmsComponent->SetSkeletalMesh(ArmsAsset);
    AuthoredArmsComponent->SetVisibility(true, true);
    AuthoredArmsComponent->SetHiddenInGame(false, true);
    LeftHandProxy->SetVisibility(false, true);
    RightHandProxy->SetVisibility(false, true);

    const FName ModeId = Interaction->GetActiveModeId();
    if (UStaticMesh* AuthoredTool = ResolveAuthoredTool(ModeId))
    {
        ToolProxy->SetStaticMesh(AuthoredTool);
    }
    WristProxy->SetStaticMesh(WristAsset);
    PlayActionIfChanged(Interaction->GetActiveActionId());
    bTakeoverActive = true;
}

void UWMFirstPersonAuthoredBridgeSubsystem::ApplyProxyFallback()
{
    if (AuthoredArmsComponent)
    {
        AuthoredArmsComponent->Stop();
        AuthoredArmsComponent->SetVisibility(false, true);
        AuthoredArmsComponent->SetHiddenInGame(true, true);
    }
    if (ToolProxy && ProxyToolFallback) ToolProxy->SetStaticMesh(ProxyToolFallback);
    if (WristProxy && ProxyWristFallback) WristProxy->SetStaticMesh(ProxyWristFallback);

    if (Interaction && Interaction->IsFirstPersonInteractionActive())
    {
        if (LeftHandProxy) LeftHandProxy->SetVisibility(true, true);
        if (RightHandProxy) RightHandProxy->SetVisibility(true, true);
    }

    LastPlayedActionId = NAME_None;
    bTakeoverActive = false;
}

void UWMFirstPersonAuthoredBridgeSubsystem::Tick(const float DeltaTime)
{
    if (!FMath::IsFinite(DeltaTime) || DeltaTime <= 0.0f) return;
    EnsureTargets();
    TryLoadAuthoredAssets();

    if (!Interaction || !Interaction->IsFirstPersonInteractionActive())
    {
        ApplyProxyFallback();
        return;
    }

    ApplyAuthoredTakeover();
}
