#include "Environment/WMEcologicalBuildSubsystem.h"

#include "Building/WMBuildUnlockSubsystem.h"
#include "Building/WMBuildWorldStateSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Environment/WMEnvironmentStateSubsystem.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
    const FName DefaultPrototypeBiomeId(TEXT("biome.caribbean-rainforest"));
}

void UWMEcologicalBuildSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Collection.InitializeDependency<UWMBuildWorldStateSubsystem>();
    Collection.InitializeDependency<UWMEnvironmentStateSubsystem>();

    ReloadAndActivatePrototypeDefinition();
    if (UWorld* World = GetWorld())
    {
        if (UWMBuildWorldStateSubsystem* BuildState = World->GetSubsystem<UWMBuildWorldStateSubsystem>())
        {
            BuildState->OnBuildWorldChanged.AddDynamic(this, &UWMEcologicalBuildSubsystem::HandleBuildWorldChanged);
        }
    }
    EvaluateCurrentBuild();
}

void UWMEcologicalBuildSubsystem::Deinitialize()
{
    if (UWorld* World = GetWorld())
    {
        if (UWMBuildWorldStateSubsystem* BuildState = World->GetSubsystem<UWMBuildWorldStateSubsystem>())
        {
            BuildState->OnBuildWorldChanged.RemoveDynamic(this, &UWMEcologicalBuildSubsystem::HandleBuildWorldChanged);
        }
    }
    DefinitionCatalog.Reset();
    ActiveBiomeId = NAME_None;
    Progress.Reset();
    Super::Deinitialize();
}

bool UWMEcologicalBuildSubsystem::ReloadAndActivatePrototypeDefinition()
{
    return ReloadDefinitionCatalog() && ActivateDefinition(DefaultPrototypeBiomeId);
}

bool UWMEcologicalBuildSubsystem::ReloadDefinitionCatalog()
{
    DefinitionCatalog.Reset();
    ActiveBiomeId = NAME_None;
    Progress.Reset();

    const FString Directory = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("WorldMakers/Biomes"));
    TArray<FString> Files;
    IFileManager::Get().FindFiles(Files, *FPaths::Combine(Directory, TEXT("*.build-interventions.json")), true, false);
    Files.Sort();

    for (const FString& File : Files)
    {
        FString Json;
        if (!FFileHelper::LoadFileToString(Json, *FPaths::Combine(Directory, File))) continue;

        FWMEcologicalBuildDefinition Definition;
        FString Error;
        if (!FWMEcologicalBuildDefinition::TryParseJson(Json, Definition, Error) || Definition.BiomeId.IsNone()) continue;
        if (DefinitionCatalog.Contains(Definition.BiomeId))
        {
            DefinitionCatalog.Reset();
            return false;
        }
        DefinitionCatalog.Add(Definition.BiomeId, MoveTemp(Definition));
    }
    return !DefinitionCatalog.IsEmpty();
}

bool UWMEcologicalBuildSubsystem::ActivateDefinition(const FName BiomeId)
{
    if (!DefinitionCatalog.Contains(BiomeId)) return false;
    ActiveBiomeId = BiomeId;
    Progress.Reset();
    return true;
}

const FWMEcologicalBuildDefinition* UWMEcologicalBuildSubsystem::GetActiveDefinition() const
{
    return DefinitionCatalog.Find(ActiveBiomeId);
}

void UWMEcologicalBuildSubsystem::HandleBuildWorldChanged(const int32 Revision)
{
    if (Revision > 0)
    {
        EvaluateCurrentBuild();
    }
}

bool UWMEcologicalBuildSubsystem::EvaluateCurrentBuild()
{
    UWorld* World = GetWorld();
    const FWMEcologicalBuildDefinition* Definition = GetActiveDefinition();
    if (!World || !Definition) return false;

    UWMBuildWorldStateSubsystem* BuildState = World->GetSubsystem<UWMBuildWorldStateSubsystem>();
    UWMEnvironmentStateSubsystem* Environment = World->GetSubsystem<UWMEnvironmentStateSubsystem>();
    UGameInstance* GameInstance = World->GetGameInstance();
    UWMBuildUnlockSubsystem* Unlocks = GameInstance ? GameInstance->GetSubsystem<UWMBuildUnlockSubsystem>() : nullptr;
    if (!BuildState || !Environment || !Unlocks || Environment->GetActiveBiomeId() != Definition->BiomeId)
    {
        return false;
    }

    const TArray<FWMPlacedBuildPieceSnapshot> Pieces = BuildState->GetPlacedPieces();
    bool bCompletedAny = false;
    for (const FWMEcologicalBuildInterventionDefinition& Intervention : Definition->Interventions)
    {
        if (!Progress.CanComplete(Intervention, Pieces)) continue;
        if (!Unlocks->IsKnownUnlockReward(Intervention.RewardId)) continue;
        if (!Environment->ApplyTrustedEffect(Intervention.InterventionId, Intervention.EffectDelta, 1)) continue;
        if (!Unlocks->EnsureRewardGranted(Intervention.RewardId)) continue;
        if (Progress.MarkCompleted(Intervention.InterventionId))
        {
            bCompletedAny = true;
            OnInterventionCompleted.Broadcast(Intervention.InterventionId, Intervention.RewardId);
        }
    }
    return bCompletedAny;
}

TArray<FWMEcologicalBuildInterventionReadModel> UWMEcologicalBuildSubsystem::GetInterventionReadModel() const
{
    TArray<FWMEcologicalBuildInterventionReadModel> Result;
    const FWMEcologicalBuildDefinition* Definition = GetActiveDefinition();
    const UWorld* World = GetWorld();
    const UWMBuildWorldStateSubsystem* BuildState = World ? World->GetSubsystem<UWMBuildWorldStateSubsystem>() : nullptr;
    if (!Definition || !BuildState) return Result;

    const TArray<FWMPlacedBuildPieceSnapshot> Pieces = BuildState->GetPlacedPieces();
    Result.Reserve(Definition->Interventions.Num());
    for (const FWMEcologicalBuildInterventionDefinition& Intervention : Definition->Interventions)
    {
        FWMEcologicalBuildInterventionReadModel Entry;
        Entry.InterventionId = Intervention.InterventionId;
        Entry.bCompleted = Progress.IsCompleted(Intervention.InterventionId);
        Entry.bPrerequisitesSatisfied = Progress.ArePrerequisitesSatisfied(Intervention);
        Entry.SatisfiedRequirementCount = Intervention.CountSatisfiedRequirements(Pieces);
        Entry.RequirementCount = Intervention.Requirements.Num();
        Entry.RewardId = Intervention.RewardId;
        Result.Add(MoveTemp(Entry));
    }
    return Result;
}

FName UWMEcologicalBuildSubsystem::GetNextOpenInterventionId() const
{
    const FWMEcologicalBuildDefinition* Definition = GetActiveDefinition();
    if (!Definition) return NAME_None;

    for (const FWMEcologicalBuildInterventionDefinition& Intervention : Definition->Interventions)
    {
        if (!Progress.IsCompleted(Intervention.InterventionId) && Progress.ArePrerequisitesSatisfied(Intervention))
        {
            return Intervention.InterventionId;
        }
    }
    return NAME_None;
}
