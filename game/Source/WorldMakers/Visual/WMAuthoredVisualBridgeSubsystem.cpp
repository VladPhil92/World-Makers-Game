#include "Visual/WMAuthoredVisualBridgeSubsystem.h"

#include "Animation/AnimInstance.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/ProceduralMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Environment/WMCaribbeanRainforestPrototype.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Player/WMPlayerCharacter.h"
#include "Visual/WMAuthoredAssetSubsystem.h"
#include "Visual/WMAvatarArtTypes.h"
#include "Visual/WMVFXSubsystem.h"

namespace WMAuthoredBridge
{
    bool CopyInstances(
        UHierarchicalInstancedStaticMeshComponent* Source,
        UHierarchicalInstancedStaticMeshComponent* Destination,
        const int32 MaxCount = MAX_int32)
    {
        if (!Source || !Destination)
        {
            return false;
        }

        Destination->ClearInstances();
        const int32 Count = FMath::Min(Source->GetInstanceCount(), MaxCount);
        for (int32 Index = 0; Index < Count; ++Index)
        {
            FTransform InstanceTransform;
            if (!Source->GetInstanceTransform(Index, InstanceTransform, false))
            {
                return false;
            }
            Destination->AddInstance(InstanceTransform, false);
        }
        return true;
    }

    void SetProceduralFamilyVisible(UProceduralMeshComponent* Component, const bool bVisible)
    {
        if (!Component)
        {
            return;
        }
        Component->SetVisibility(bVisible, true);
        Component->SetHiddenInGame(!bVisible, true);
    }

    const TCHAR* JsonBool(const bool bValue)
    {
        return bValue ? TEXT("true") : TEXT("false");
    }
}

void UWMAuthoredVisualBridgeSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);
    RetryRemainingSeconds = 3.0f;
    RetryAccumulatorSeconds = 0.0f;
    bTakeoverReportWritten = false;
    FParse::Value(FCommandLine::Get(), TEXT("WMN2TakeoverReport="), TakeoverReportPath);
    FParse::Value(FCommandLine::Get(), TEXT("WMBuildCommit="), TakeoverBuildCommit);
    RefreshAuthoredVisuals();
    TryWriteN2TakeoverReport(false);
}

void UWMAuthoredVisualBridgeSubsystem::Tick(const float DeltaTime)
{
    if (!FMath::IsFinite(DeltaTime) || DeltaTime <= 0.0f)
    {
        return;
    }

    if (RetryRemainingSeconds > 0.0f)
    {
        RetryRemainingSeconds = FMath::Max(0.0f, RetryRemainingSeconds - DeltaTime);
        RetryAccumulatorSeconds += DeltaTime;
        if (RetryAccumulatorSeconds >= 0.25f)
        {
            RetryAccumulatorSeconds = 0.0f;
            RefreshAuthoredVisuals();
        }
    }

    TryWriteN2TakeoverReport(false);
    if (RetryRemainingSeconds <= 0.0f)
    {
        TryWriteN2TakeoverReport(true);
    }
}

TStatId UWMAuthoredVisualBridgeSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UWMAuthoredVisualBridgeSubsystem, STATGROUP_Tickables);
}

UWMAuthoredAssetSubsystem* UWMAuthoredVisualBridgeSubsystem::GetAssetSubsystem() const
{
    UWorld* World = GetWorld();
    UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
    return GameInstance ? GameInstance->GetSubsystem<UWMAuthoredAssetSubsystem>() : nullptr;
}

UHierarchicalInstancedStaticMeshComponent* UWMAuthoredVisualBridgeSubsystem::CreateRenderFamily(
    AWMCaribbeanRainforestPrototype* Biome,
    const FName ComponentName,
    UStaticMesh* Mesh) const
{
    if (!Biome || !Mesh || !Biome->SceneRoot)
    {
        return nullptr;
    }

    TInlineComponentArray<UHierarchicalInstancedStaticMeshComponent*> ExistingComponents;
    Biome->GetComponents(ExistingComponents);
    for (UHierarchicalInstancedStaticMeshComponent* Existing : ExistingComponents)
    {
        if (Existing && Existing->GetFName() == ComponentName)
        {
            Existing->ClearInstances();
            Existing->SetStaticMesh(Mesh);
            Existing->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Existing->SetCanEverAffectNavigation(false);
            Existing->SetVisibility(false, true);
            Existing->SetHiddenInGame(true, true);
            return Existing;
        }
    }

    UHierarchicalInstancedStaticMeshComponent* Component = NewObject<UHierarchicalInstancedStaticMeshComponent>(Biome, ComponentName);
    if (!Component)
    {
        return nullptr;
    }

    Biome->AddInstanceComponent(Component);
    Component->SetupAttachment(Biome->SceneRoot);
    Component->SetStaticMesh(Mesh);
    Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Component->SetCanEverAffectNavigation(false);
    Component->SetVisibility(false, true);
    Component->SetHiddenInGame(true, true);
    Component->RegisterComponent();
    return Component;
}

void UWMAuthoredVisualBridgeSubsystem::SetAuthoredEnvironmentVisible(
    AWMCaribbeanRainforestPrototype* Biome,
    const bool bVisible) const
{
    if (!Biome)
    {
        return;
    }

    TInlineComponentArray<UHierarchicalInstancedStaticMeshComponent*> Components;
    Biome->GetComponents(Components);
    for (UHierarchicalInstancedStaticMeshComponent* Component : Components)
    {
        if (Component && Component->GetName().StartsWith(TEXT("Authored")))
        {
            Component->SetVisibility(bVisible, true);
            Component->SetHiddenInGame(!bVisible, true);
        }
    }
}

void UWMAuthoredVisualBridgeSubsystem::SetProceduralEnvironmentVisible(
    AWMCaribbeanRainforestPrototype* Biome,
    const bool bVisible) const
{
    if (!Biome)
    {
        return;
    }

    for (UProceduralMeshComponent* Procedural : TArray<UProceduralMeshComponent*>{
        Biome->GroundArt,
        Biome->TerrainArt,
        Biome->BarkAndRootsArt,
        Biome->FoliageArt,
        Biome->StoneArt,
        Biome->WaterArt})
    {
        WMAuthoredBridge::SetProceduralFamilyVisible(Procedural, bVisible);
    }
}

bool UWMAuthoredVisualBridgeSubsystem::TryPrepareAuthoredAvatar(
    AWMPlayerCharacter* Character,
    UWMAuthoredAssetSubsystem* Assets)
{
    if (!Character || !Assets || !Character->GetMesh())
    {
        return false;
    }

    USkeletalMesh* AuthoredMesh = Assets->LoadSkeletalMesh(TEXT("character.player.child-explorer"));
    if (!AuthoredMesh)
    {
        return false;
    }
    bAuthoredAvatarReady = true;

    const TSubclassOf<UAnimInstance> AnimClass = Assets->LoadAnimationBlueprintClass(TEXT("animation.player.blueprint"));
    if (!AnimClass)
    {
        return false;
    }

    Character->GetMesh()->SetSkeletalMesh(AuthoredMesh);
    Character->GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Character->GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
    Character->GetMesh()->SetAnimInstanceClass(AnimClass);
    bAuthoredAnimationBlueprintActive = true;

    // N2 runtime activation may override the V4 source-development preference, but only after
    // P1 explicitly declares both the production mesh and AnimBP present. This mutates only the
    // process-local default object; reverting the P1 registry restores the procedural fallback.
    if (UWMAvatarVisualSettings* Settings = GetMutableDefault<UWMAvatarVisualSettings>())
    {
        Settings->bUseProceduralAvatarArt = false;
    }
    Character->RefreshAvatarVisualPath();

    return !Character->IsProceduralAvatarActive() && Character->GetMesh()->IsVisible();
}

bool UWMAuthoredVisualBridgeSubsystem::TryApplyAuthoredEnvironment(
    AWMCaribbeanRainforestPrototype* Biome,
    UWMAuthoredAssetSubsystem* Assets)
{
    if (!Biome || !Assets)
    {
        return false;
    }

    UStaticMesh* Ground = Assets->LoadStaticMesh(TEXT("environment.rainforest.ground.a"));
    UStaticMesh* Terrain = Assets->LoadStaticMesh(TEXT("environment.rainforest.terrain.a"));
    UStaticMesh* TreeA = Assets->LoadStaticMesh(TEXT("environment.rainforest.tree.a"));
    UStaticMesh* TreeB = Assets->LoadStaticMesh(TEXT("environment.rainforest.tree.b"));
    UStaticMesh* TreeC = Assets->LoadStaticMesh(TEXT("environment.rainforest.tree.c"));
    UStaticMesh* Understory = Assets->LoadStaticMesh(TEXT("environment.rainforest.understory.a"));
    UStaticMesh* Rock = Assets->LoadStaticMesh(TEXT("environment.rainforest.rock.a"));
    UStaticMesh* WaterEdge = Assets->LoadStaticMesh(TEXT("environment.rainforest.water-edge.a"));
    UStaticMesh* HeroCeiba = Assets->LoadStaticMesh(TEXT("environment.rainforest.hero-ceiba"));

    if (!Ground || !Terrain || !TreeA || !TreeB || !TreeC || !Understory || !Rock || !WaterEdge || !HeroCeiba)
    {
        return false;
    }

    UHierarchicalInstancedStaticMeshComponent* GroundAuthored = CreateRenderFamily(Biome, TEXT("AuthoredGroundArt"), Ground);
    UHierarchicalInstancedStaticMeshComponent* TerrainAuthored = CreateRenderFamily(Biome, TEXT("AuthoredTerrainArt"), Terrain);
    UHierarchicalInstancedStaticMeshComponent* TreeArtA = CreateRenderFamily(Biome, TEXT("AuthoredTreeArtA"), TreeA);
    UHierarchicalInstancedStaticMeshComponent* TreeArtB = CreateRenderFamily(Biome, TEXT("AuthoredTreeArtB"), TreeB);
    UHierarchicalInstancedStaticMeshComponent* TreeArtC = CreateRenderFamily(Biome, TEXT("AuthoredTreeArtC"), TreeC);
    UHierarchicalInstancedStaticMeshComponent* UnderstoryArt = CreateRenderFamily(Biome, TEXT("AuthoredUnderstoryArt"), Understory);
    UHierarchicalInstancedStaticMeshComponent* RockAuthored = CreateRenderFamily(Biome, TEXT("AuthoredRockArt"), Rock);
    UHierarchicalInstancedStaticMeshComponent* WaterAuthored = CreateRenderFamily(Biome, TEXT("AuthoredWaterEdgeArt"), WaterEdge);
    UHierarchicalInstancedStaticMeshComponent* HeroArt = CreateRenderFamily(Biome, TEXT("AuthoredHeroCeibaArt"), HeroCeiba);

    const bool bComponentsReady = GroundAuthored && TerrainAuthored && TreeArtA && TreeArtB && TreeArtC &&
        UnderstoryArt && RockAuthored && WaterAuthored && HeroArt;
    if (!bComponentsReady)
    {
        SetAuthoredEnvironmentVisible(Biome, false);
        return false;
    }

    if (!WMAuthoredBridge::CopyInstances(Biome->GroundTiles, GroundAuthored) ||
        !WMAuthoredBridge::CopyInstances(Biome->TerrainMounds, TerrainAuthored) ||
        !WMAuthoredBridge::CopyInstances(Biome->Rocks, RockAuthored) ||
        !WMAuthoredBridge::CopyInstances(Biome->WaterEdgeMarkers, WaterAuthored))
    {
        SetAuthoredEnvironmentVisible(Biome, false);
        return false;
    }

    TreeArtA->ClearInstances();
    TreeArtB->ClearInstances();
    TreeArtC->ClearInstances();
    UnderstoryArt->ClearInstances();
    HeroArt->ClearInstances();

    const int32 TreeCount = Biome->TreeTrunks ? Biome->TreeTrunks->GetInstanceCount() : 0;
    if (TreeCount < 1)
    {
        SetAuthoredEnvironmentVisible(Biome, false);
        return false;
    }

    const int32 HeroIndex = TreeCount - 1;
    for (int32 Index = 0; Index < HeroIndex; ++Index)
    {
        FTransform TreeTransform;
        if (!Biome->TreeTrunks->GetInstanceTransform(Index, TreeTransform, false))
        {
            SetAuthoredEnvironmentVisible(Biome, false);
            return false;
        }

        UHierarchicalInstancedStaticMeshComponent* Target = Index % 3 == 0 ? TreeArtA : (Index % 3 == 1 ? TreeArtB : TreeArtC);
        Target->AddInstance(TreeTransform, false);

        if ((Index % 2) == 0)
        {
            const float Angle = FMath::DegreesToRadians(FMath::Fmod(static_cast<float>(Index) * 137.5f, 360.0f));
            FTransform UnderstoryTransform = TreeTransform;
            UnderstoryTransform.SetLocation(
                TreeTransform.GetLocation() + FVector(FMath::Cos(Angle) * 85.0f, FMath::Sin(Angle) * 85.0f, -45.0f));
            UnderstoryTransform.SetScale3D(FVector(0.72f));
            UnderstoryArt->AddInstance(UnderstoryTransform, false);
        }
    }

    FTransform HeroTransform;
    if (!Biome->TreeTrunks->GetInstanceTransform(HeroIndex, HeroTransform, false))
    {
        SetAuthoredEnvironmentVisible(Biome, false);
        return false;
    }
    HeroArt->AddInstance(HeroTransform, false);

    SetProceduralEnvironmentVisible(Biome, false);
    SetAuthoredEnvironmentVisible(Biome, true);
    return true;
}

bool UWMAuthoredVisualBridgeSubsystem::IsAuthoredPresentationReady() const
{
    const UWMAuthoredAssetSubsystem* Assets = GetAssetSubsystem();
    return Assets &&
        Assets->IsAuthoredAssetDeclaredPresent(TEXT("presentation.adventure-reveal")) &&
        Assets->IsAuthoredAssetDeclaredPresent(TEXT("presentation.camera-data"));
}

bool UWMAuthoredVisualBridgeSubsystem::IsFullAuthoredTakeoverActive() const
{
    const UWorld* World = GetWorld();
    const UWMVFXSubsystem* VFX = World ? World->GetSubsystem<UWMVFXSubsystem>() : nullptr;
    return bAuthoredEnvironmentActive && bAuthoredAvatarActive && bAuthoredAnimationBlueprintActive &&
        VFX && VFX->IsAuthoredNiagaraEnabled() && IsAuthoredPresentationReady();
}

void UWMAuthoredVisualBridgeSubsystem::RefreshAuthoredVisuals()
{
    UWorld* World = GetWorld();
    UWMAuthoredAssetSubsystem* Assets = GetAssetSubsystem();
    if (!World || !Assets || !Assets->IsCatalogLoaded())
    {
        return;
    }

    bAuthoredAvatarReady = false;
    bAuthoredAvatarActive = false;
    bAuthoredAnimationBlueprintActive = false;
    for (TActorIterator<AWMPlayerCharacter> It(World); It; ++It)
    {
        bAuthoredAvatarActive |= TryPrepareAuthoredAvatar(*It, Assets);
    }

    const bool bPreviouslyAuthoredEnvironment = bAuthoredEnvironmentActive;
    bAuthoredEnvironmentActive = false;
    for (TActorIterator<AWMCaribbeanRainforestPrototype> It(World); It; ++It)
    {
        AWMCaribbeanRainforestPrototype* Biome = *It;
        const bool bApplied = TryApplyAuthoredEnvironment(Biome, Assets);
        bAuthoredEnvironmentActive |= bApplied;
        if (!bApplied && bPreviouslyAuthoredEnvironment)
        {
            SetAuthoredEnvironmentVisible(Biome, false);
            SetProceduralEnvironmentVisible(Biome, true);
        }
    }
}

void UWMAuthoredVisualBridgeSubsystem::TryWriteN2TakeoverReport(const bool bForce)
{
    if (bTakeoverReportWritten || TakeoverReportPath.IsEmpty())
    {
        return;
    }

    UWorld* World = GetWorld();
    UWMVFXSubsystem* VFX = World ? World->GetSubsystem<UWMVFXSubsystem>() : nullptr;
    const bool bAuthoredVfxReady = VFX && VFX->IsAuthoredNiagaraEnabled();
    const bool bPresentationReady = IsAuthoredPresentationReady();

    bool bProceduralAvatarActive = false;
    if (World)
    {
        for (TActorIterator<AWMPlayerCharacter> It(World); It; ++It)
        {
            bProceduralAvatarActive |= It->IsProceduralAvatarActive();
        }
    }

    const bool bFullTakeover = bAuthoredEnvironmentActive && bAuthoredAvatarActive &&
        bAuthoredAnimationBlueprintActive && bAuthoredVfxReady && bPresentationReady && !bProceduralAvatarActive;
    if (!bFullTakeover && !bForce)
    {
        return;
    }

    const FString AbsoluteReportPath = FPaths::IsRelative(TakeoverReportPath)
        ? FPaths::ConvertRelativePathToFull(FPaths::ProjectDir(), TakeoverReportPath)
        : TakeoverReportPath;
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(AbsoluteReportPath), true);

    const FString Json = FString::Printf(
        TEXT("{\n  \"schemaVersion\": 1,\n  \"phase\": \"N2\",\n  \"status\": \"%s\",\n  \"buildCommit\": \"%s\",\n  \"authoredEnvironmentActive\": %s,\n  \"proceduralEnvironmentVisible\": %s,\n  \"authoredAvatarActive\": %s,\n  \"proceduralAvatarActive\": %s,\n  \"authoredAnimationBlueprintActive\": %s,\n  \"authoredVfxReady\": %s,\n  \"authoredPresentationReady\": %s\n}\n"),
        bFullTakeover ? TEXT("TAKEOVER_VERIFIED") : TEXT("BLOCKED"),
        *TakeoverBuildCommit,
        WMAuthoredBridge::JsonBool(bAuthoredEnvironmentActive),
        WMAuthoredBridge::JsonBool(!bAuthoredEnvironmentActive),
        WMAuthoredBridge::JsonBool(bAuthoredAvatarActive),
        WMAuthoredBridge::JsonBool(bProceduralAvatarActive),
        WMAuthoredBridge::JsonBool(bAuthoredAnimationBlueprintActive),
        WMAuthoredBridge::JsonBool(bAuthoredVfxReady),
        WMAuthoredBridge::JsonBool(bPresentationReady));

    bTakeoverReportWritten = FFileHelper::SaveStringToFile(Json, *AbsoluteReportPath);
}
