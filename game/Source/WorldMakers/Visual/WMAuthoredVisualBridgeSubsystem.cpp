#include "Visual/WMAuthoredVisualBridgeSubsystem.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/ProceduralMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Environment/WMCaribbeanRainforestPrototype.h"
#include "Player/WMPlayerCharacter.h"
#include "Visual/WMAuthoredAssetSubsystem.h"

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
}

void UWMAuthoredVisualBridgeSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);
    RefreshAuthoredVisuals();
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
    Component->RegisterComponent();
    return Component;
}

bool UWMAuthoredVisualBridgeSubsystem::TryApplyAuthoredAvatar(
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

    Character->GetMesh()->SetSkeletalMesh(AuthoredMesh);
    Character->GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Character->RefreshAvatarVisualPath();
    return true;
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

    UHierarchicalInstancedStaticMeshComponent* GroundArt = CreateRenderFamily(Biome, TEXT("AuthoredGroundArt"), Ground);
    UHierarchicalInstancedStaticMeshComponent* TerrainArt = CreateRenderFamily(Biome, TEXT("AuthoredTerrainArt"), Terrain);
    UHierarchicalInstancedStaticMeshComponent* TreeArtA = CreateRenderFamily(Biome, TEXT("AuthoredTreeArtA"), TreeA);
    UHierarchicalInstancedStaticMeshComponent* TreeArtB = CreateRenderFamily(Biome, TEXT("AuthoredTreeArtB"), TreeB);
    UHierarchicalInstancedStaticMeshComponent* TreeArtC = CreateRenderFamily(Biome, TEXT("AuthoredTreeArtC"), TreeC);
    UHierarchicalInstancedStaticMeshComponent* UnderstoryArt = CreateRenderFamily(Biome, TEXT("AuthoredUnderstoryArt"), Understory);
    UHierarchicalInstancedStaticMeshComponent* RockArt = CreateRenderFamily(Biome, TEXT("AuthoredRockArt"), Rock);
    UHierarchicalInstancedStaticMeshComponent* WaterArt = CreateRenderFamily(Biome, TEXT("AuthoredWaterEdgeArt"), WaterEdge);
    UHierarchicalInstancedStaticMeshComponent* HeroArt = CreateRenderFamily(Biome, TEXT("AuthoredHeroCeibaArt"), HeroCeiba);

    const bool bComponentsReady = GroundArt && TerrainArt && TreeArtA && TreeArtB && TreeArtC && UnderstoryArt && RockArt && WaterArt && HeroArt;
    if (!bComponentsReady)
    {
        return false;
    }

    if (!WMAuthoredBridge::CopyInstances(Biome->GroundTiles, GroundArt) ||
        !WMAuthoredBridge::CopyInstances(Biome->TerrainMounds, TerrainArt) ||
        !WMAuthoredBridge::CopyInstances(Biome->Rocks, RockArt) ||
        !WMAuthoredBridge::CopyInstances(Biome->WaterEdgeMarkers, WaterArt))
    {
        return false;
    }

    const int32 TreeCount = Biome->TreeTrunks ? Biome->TreeTrunks->GetInstanceCount() : 0;
    if (TreeCount < 1)
    {
        return false;
    }

    const int32 HeroIndex = TreeCount - 1;
    for (int32 Index = 0; Index < HeroIndex; ++Index)
    {
        FTransform TreeTransform;
        if (!Biome->TreeTrunks->GetInstanceTransform(Index, TreeTransform, false))
        {
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
        return false;
    }
    HeroArt->AddInstance(HeroTransform, false);

    for (UProceduralMeshComponent* Procedural : TArray<UProceduralMeshComponent*>{
        Biome->GroundArt,
        Biome->TerrainArt,
        Biome->BarkAndRootsArt,
        Biome->FoliageArt,
        Biome->StoneArt,
        Biome->WaterArt})
    {
        WMAuthoredBridge::SetProceduralFamilyVisible(Procedural, false);
    }

    return true;
}

void UWMAuthoredVisualBridgeSubsystem::RefreshAuthoredVisuals()
{
    UWorld* World = GetWorld();
    UWMAuthoredAssetSubsystem* Assets = GetAssetSubsystem();
    if (!World || !Assets || !Assets->IsCatalogLoaded())
    {
        return;
    }

    bAuthoredAvatarActive = false;
    for (TActorIterator<AWMPlayerCharacter> It(World); It; ++It)
    {
        bAuthoredAvatarActive |= TryApplyAuthoredAvatar(*It, Assets);
    }

    bAuthoredEnvironmentActive = false;
    for (TActorIterator<AWMCaribbeanRainforestPrototype> It(World); It; ++It)
    {
        bAuthoredEnvironmentActive |= TryApplyAuthoredEnvironment(*It, Assets);
    }
}
