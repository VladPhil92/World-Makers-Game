#include "Visual/WMVFXSubsystem.h"

#include "Building/WMBuildWorldStateSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Environment/WMEnvironmentStateSubsystem.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Misc/Paths.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "UObject/SoftObjectPath.h"
#include "Visual/WMAuthoredAssetSubsystem.h"
#include "Visual/WMProceduralVFXActor.h"
#include "Visual/WMVisualProfileSettings.h"

namespace
{
    bool SamePlacedPiece(const FWMPlacedBuildPieceSnapshot& A, const FWMPlacedBuildPieceSnapshot& B)
    {
        return A.PieceId == B.PieceId && A.LocationCm.Equals(B.LocationCm, 0.1f) && FMath::IsNearlyEqual(A.YawDegrees, B.YawDegrees, 0.1f);
    }

    const FWMPlacedBuildPieceSnapshot* FindUnmatched(const TArray<FWMPlacedBuildPieceSnapshot>& Candidates, const TArray<FWMPlacedBuildPieceSnapshot>& Other)
    {
        for (const FWMPlacedBuildPieceSnapshot& Candidate : Candidates)
        {
            const bool bMatched = Other.ContainsByPredicate([&Candidate](const FWMPlacedBuildPieceSnapshot& OtherPiece)
            {
                return SamePlacedPiece(Candidate, OtherPiece);
            });
            if (!bMatched)
            {
                return &Candidate;
            }
        }
        return nullptr;
    }

    FString CanonicalObjectPath(const FString& PackagePath)
    {
        if (PackagePath.Contains(TEXT(".")))
        {
            return PackagePath;
        }
        const FString Name = FPaths::GetBaseFilename(PackagePath);
        return FString::Printf(TEXT("%s.%s"), *PackagePath, *Name);
    }
}

void UWMVFXSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Collection.InitializeDependency<UWMBuildWorldStateSubsystem>();
    Collection.InitializeDependency<UWMEnvironmentStateSubsystem>();
    Super::Initialize(Collection);

    if (UWorld* World = GetWorld())
    {
        if (UWMBuildWorldStateSubsystem* BuildState = World->GetSubsystem<UWMBuildWorldStateSubsystem>())
        {
            LastBuildSnapshot = BuildState->GetPlacedPieces();
            BuildState->OnBuildWorldChanged.AddDynamic(this, &UWMVFXSubsystem::HandleBuildWorldChanged);
        }
        if (UWMEnvironmentStateSubsystem* EnvironmentState = World->GetSubsystem<UWMEnvironmentStateSubsystem>())
        {
            LastEnvironmentSnapshot = EnvironmentState->GetStateSnapshot();
            bHasEnvironmentSnapshot = LastEnvironmentSnapshot.IsBounded();
            EnvironmentState->OnEnvironmentStateChanged.AddDynamic(this, &UWMVFXSubsystem::HandleEnvironmentStateChanged);
        }
    }
}

void UWMVFXSubsystem::Deinitialize()
{
    if (UWorld* World = GetWorld())
    {
        if (UWMBuildWorldStateSubsystem* BuildState = World->GetSubsystem<UWMBuildWorldStateSubsystem>())
        {
            BuildState->OnBuildWorldChanged.RemoveDynamic(this, &UWMVFXSubsystem::HandleBuildWorldChanged);
        }
        if (UWMEnvironmentStateSubsystem* EnvironmentState = World->GetSubsystem<UWMEnvironmentStateSubsystem>())
        {
            EnvironmentState->OnEnvironmentStateChanged.RemoveDynamic(this, &UWMVFXSubsystem::HandleEnvironmentStateChanged);
        }
    }

    for (const TWeakObjectPtr<AWMProceduralVFXActor>& Effect : ActiveProxyEffects)
    {
        if (Effect.IsValid()) Effect->Destroy();
    }
    for (const TWeakObjectPtr<UNiagaraComponent>& Effect : ActiveAuthoredEffects)
    {
        if (Effect.IsValid()) Effect->DeactivateImmediate();
    }
    ActiveProxyEffects.Reset();
    ActiveAuthoredEffects.Reset();
    LastBuildSnapshot.Reset();
    Runtime = FWMVFXRuntime();
    LastAcceptedEventId = NAME_None;
    bLastAcceptedEffectAuthored = false;
    bHasEnvironmentSnapshot = false;
    Super::Deinitialize();
}

FWMVFXBudget UWMVFXSubsystem::ResolveBudget() const
{
    FWMVFXBudget Budget;
    const UWMVisualProfileSettings* VisualSettings = GetDefault<UWMVisualProfileSettings>();
    const EWMVisualQualityTier Tier = VisualSettings ? VisualSettings->DefaultQualityTier : EWMVisualQualityTier::Mid;
    switch (Tier)
    {
        case EWMVisualQualityTier::Low:
            Budget.MaxActiveProxyEffects = 8;
            Budget.MaxEventsPerSecond = 8;
            Budget.MaxDurationSeconds = 1.15f;
            Budget.MaxIntensity = 0.85f;
            break;
        case EWMVisualQualityTier::High:
            Budget.MaxActiveProxyEffects = 28;
            Budget.MaxEventsPerSecond = 28;
            Budget.MaxDurationSeconds = 2.0f;
            Budget.MaxIntensity = 1.0f;
            break;
        case EWMVisualQualityTier::Mid:
        default:
            Budget.MaxActiveProxyEffects = 16;
            Budget.MaxEventsPerSecond = 16;
            Budget.MaxDurationSeconds = 1.5f;
            Budget.MaxIntensity = 1.0f;
            break;
    }
    return Budget;
}

void UWMVFXSubsystem::PruneExpiredEffects()
{
    ActiveProxyEffects.RemoveAll([](const TWeakObjectPtr<AWMProceduralVFXActor>& Effect)
    {
        return !Effect.IsValid();
    });
    ActiveAuthoredEffects.RemoveAll([](const TWeakObjectPtr<UNiagaraComponent>& Effect)
    {
        return !Effect.IsValid() || Effect->IsComplete();
    });
}

int32 UWMVFXSubsystem::GetActiveProxyEffectCount() const
{
    int32 Count = 0;
    for (const TWeakObjectPtr<AWMProceduralVFXActor>& Effect : ActiveProxyEffects)
    {
        if (Effect.IsValid()) ++Count;
    }
    return Count;
}

int32 UWMVFXSubsystem::GetActiveAuthoredEffectCount() const
{
    int32 Count = 0;
    for (const TWeakObjectPtr<UNiagaraComponent>& Effect : ActiveAuthoredEffects)
    {
        if (Effect.IsValid() && !Effect->IsComplete()) ++Count;
    }
    return Count;
}

bool UWMVFXSubsystem::IsAuthoredNiagaraEnabled() const
{
    const UWorld* World = GetWorld();
    const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
    const UWMAuthoredAssetSubsystem* Assets = GameInstance ? GameInstance->GetSubsystem<UWMAuthoredAssetSubsystem>() : nullptr;
    return Assets && Assets->IsCatalogLoaded() && Assets->IsAuthoredAssetDeclaredPresent(TEXT("vfx.science.master"));
}

FString UWMVFXSubsystem::ResolveAuthoredNiagaraObjectPath(const FName EventId)
{
    const FString Id = EventId.ToString();
    if (Id == TEXT("gameplay.build.place")) return CanonicalObjectPath(TEXT("/Game/WorldMakers/VFX/NS_WM_Build_Place"));
    if (Id == TEXT("gameplay.build.remove")) return CanonicalObjectPath(TEXT("/Game/WorldMakers/VFX/NS_WM_Build_Remove"));
    if (Id == TEXT("gameplay.build.move")) return CanonicalObjectPath(TEXT("/Game/WorldMakers/VFX/NS_WM_Build_Move"));
    if (Id == TEXT("mission.measure.reveal")) return CanonicalObjectPath(TEXT("/Game/WorldMakers/VFX/NS_WM_Measure_Reveal"));
    if (Id == TEXT("world.observe.reveal")) return CanonicalObjectPath(TEXT("/Game/WorldMakers/VFX/NS_WM_Observe_Reveal"));
    if (Id == TEXT("science.chemistry.dissolution")) return CanonicalObjectPath(TEXT("/Game/WorldMakers/VFX/NS_WM_Chem_Dissolution"));
    if (Id == TEXT("science.chemistry.saturation")) return CanonicalObjectPath(TEXT("/Game/WorldMakers/VFX/NS_WM_Chem_Saturation"));
    if (Id == TEXT("science.chemistry.filtration")) return CanonicalObjectPath(TEXT("/Game/WorldMakers/VFX/NS_WM_Chem_Filtration"));
    if (Id == TEXT("science.chemistry.reaction")) return CanonicalObjectPath(TEXT("/Game/WorldMakers/VFX/NS_WM_Chem_Reaction"));
    if (Id == TEXT("science.physics.force")) return CanonicalObjectPath(TEXT("/Game/WorldMakers/VFX/NS_WM_Physics_Force"));
    if (Id == TEXT("science.physics.circuit-flow")) return CanonicalObjectPath(TEXT("/Game/WorldMakers/VFX/NS_WM_Physics_CircuitFlow"));
    if (Id == TEXT("science.biology.cell-energy")) return CanonicalObjectPath(TEXT("/Game/WorldMakers/VFX/NS_WM_Bio_CellEnergy"));
    if (Id == TEXT("science.biology.plant-growth")) return CanonicalObjectPath(TEXT("/Game/WorldMakers/VFX/NS_WM_Bio_PlantGrowth"));
    if (Id == TEXT("science.ecology.recovery")) return CanonicalObjectPath(TEXT("/Game/WorldMakers/VFX/NS_WM_Ecology_Recovery"));
    if (Id == TEXT("science.ecology.stress")) return CanonicalObjectPath(TEXT("/Game/WorldMakers/VFX/NS_WM_Ecology_Stress"));
    if (Id == TEXT("fantasy.portal.open")) return CanonicalObjectPath(TEXT("/Game/WorldMakers/VFX/NS_WM_Fantasy_Portal"));
    if (Id == TEXT("fantasy.rune.activate")) return CanonicalObjectPath(TEXT("/Game/WorldMakers/VFX/NS_WM_Fantasy_Rune"));
    return FString();
}

bool UWMVFXSubsystem::TrySpawnAuthoredNiagara(const FWMVFXEvent& Event)
{
    UWorld* World = GetWorld();
    if (!World || !IsAuthoredNiagaraEnabled()) return false;

    const FString ObjectPath = ResolveAuthoredNiagaraObjectPath(Event.EventId);
    if (ObjectPath.IsEmpty()) return false;
    UNiagaraSystem* System = Cast<UNiagaraSystem>(FSoftObjectPath(ObjectPath).TryLoad());
    if (!System) return false;

    const FRotator Rotation = Event.Direction.IsNearlyZero() ? FRotator::ZeroRotator : Event.Direction.Rotation();
    UNiagaraComponent* Component = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
        World, System, Event.LocationCm, Rotation, FVector::OneVector, true, true, ENCPoolMethod::None, true);
    if (!Component) return false;

    Component->SetVariableFloat(TEXT("User.Intensity"), Event.Intensity);
    Component->SetVariableFloat(TEXT("User.MotionScale"), Event.MotionScale);
    ActiveAuthoredEffects.Add(Component);
    return true;
}

bool UWMVFXSubsystem::EmitSemanticEvent(const FName EventId, const FVector LocationCm, const float Intensity, const FVector Direction, const float DurationSeconds)
{
    FWMVFXEvent Event;
    Event.EventId = EventId;
    Event.LocationCm = LocationCm;
    Event.Direction = Direction.IsNearlyZero() ? FVector::ForwardVector : Direction.GetSafeNormal();
    Event.Intensity = FMath::Clamp(Intensity, 0.0f, 1.0f);
    Event.DurationSeconds = FMath::Clamp(DurationSeconds, 0.08f, 2.5f);
    Event.MotionScale = 1.0f;
    return EmitEvent(Event);
}

bool UWMVFXSubsystem::EmitEvent(const FWMVFXEvent& Event)
{
    UWorld* World = GetWorld();
    if (!World) return false;

    PruneExpiredEffects();
    FWMVFXEvent Accepted = Event;
    const FWMVFXBudget Budget = ResolveBudget();
    const int32 ActiveEffectCount = GetActiveProxyEffectCount() + GetActiveAuthoredEffectCount();
    if (!Runtime.TryAccept(Accepted, World->GetTimeSeconds(), ActiveEffectCount, Budget, bReducedMotion)) return false;

    FWMVFXStyle Style;
    if (!FWMVFXRuntime::ResolveStyle(Accepted.EventId, Style)) return false;

    bLastAcceptedEffectAuthored = TrySpawnAuthoredNiagara(Accepted);
    if (!bLastAcceptedEffectAuthored)
    {
        FActorSpawnParameters SpawnParameters;
        SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AWMProceduralVFXActor* Effect = World->SpawnActor<AWMProceduralVFXActor>(AWMProceduralVFXActor::StaticClass(), Accepted.LocationCm, FRotator::ZeroRotator, SpawnParameters);
        if (!Effect || !Effect->InitializeEffect(Accepted, Style))
        {
            if (Effect) Effect->Destroy();
            return false;
        }
        ActiveProxyEffects.Add(Effect);
    }

    LastAcceptedEventId = Accepted.EventId;
    OnVFXAccepted.Broadcast(Accepted.EventId, Accepted.LocationCm, Accepted.Intensity);
    return true;
}

void UWMVFXSubsystem::HandleBuildWorldChanged(const int32 Revision)
{
    if (Revision <= 0 || !GetWorld()) return;
    UWMBuildWorldStateSubsystem* BuildState = GetWorld()->GetSubsystem<UWMBuildWorldStateSubsystem>();
    if (!BuildState) return;

    const TArray<FWMPlacedBuildPieceSnapshot> Current = BuildState->GetPlacedPieces();
    const FWMPlacedBuildPieceSnapshot* Added = FindUnmatched(Current, LastBuildSnapshot);
    const FWMPlacedBuildPieceSnapshot* Removed = FindUnmatched(LastBuildSnapshot, Current);

    if (Current.Num() > LastBuildSnapshot.Num() && Added)
    {
        EmitSemanticEvent(TEXT("gameplay.build.place"), Added->LocationCm, 0.85f, FVector::UpVector, 0.60f);
    }
    else if (Current.Num() < LastBuildSnapshot.Num() && Removed)
    {
        EmitSemanticEvent(TEXT("gameplay.build.remove"), Removed->LocationCm, 0.80f, FVector::UpVector, 0.55f);
    }
    else if (Added && Removed)
    {
        FVector Direction = Added->LocationCm - Removed->LocationCm;
        if (Direction.IsNearlyZero()) Direction = FVector::ForwardVector;
        EmitSemanticEvent(TEXT("gameplay.build.move"), Added->LocationCm, 0.70f, Direction, 0.55f);
    }
    LastBuildSnapshot = Current;
}

void UWMVFXSubsystem::HandleEnvironmentStateChanged(const FWMEnvironmentStateSnapshot Snapshot)
{
    if (!Snapshot.IsBounded()) return;
    if (!bHasEnvironmentSnapshot)
    {
        LastEnvironmentSnapshot = Snapshot;
        bHasEnvironmentSnapshot = true;
        return;
    }

    const float SignedChange =
        (Snapshot.VegetationHealth - LastEnvironmentSnapshot.VegetationHealth) +
        (Snapshot.WaterFlow - LastEnvironmentSnapshot.WaterFlow) +
        (Snapshot.SoilProtection - LastEnvironmentSnapshot.SoilProtection) +
        (Snapshot.ShadeCoverage - LastEnvironmentSnapshot.ShadeCoverage);

    LastEnvironmentSnapshot = Snapshot;
    if (FMath::IsNearlyZero(SignedChange) || !GetWorld()) return;

    FVector Location = FVector::ZeroVector;
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
    {
        if (APawn* Pawn = PC->GetPawn()) Location = Pawn->GetActorLocation() + FVector(0.0f, 0.0f, 30.0f);
    }

    const FName EventId = SignedChange > 0.0f ? FName(TEXT("science.ecology.recovery")) : FName(TEXT("science.ecology.stress"));
    const float Intensity = FMath::Clamp(FMath::Abs(SignedChange) * 0.75f, 0.20f, 1.0f);
    EmitSemanticEvent(EventId, Location, Intensity, FVector::UpVector, 0.95f);
}
