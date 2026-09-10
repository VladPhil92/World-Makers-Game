#include "UI/WMChildJourneySubsystem.h"

#include "Building/WMBuildUnlockSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Environment/WMBiomeRuntimeSubsystem.h"
#include "Environment/WMEcologicalBuildSubsystem.h"
#include "Environment/WMEnvironmentStateSubsystem.h"
#include "Mission/WMMissionRuntimeSubsystem.h"

#define LOCTEXT_NAMESPACE "WorldMakersChildJourney"

namespace
{
    struct FCreativeUnlockProjection
    {
        FName PieceId;
        FName RewardId;
        FName InterventionId;
    };

    const FCreativeUnlockProjection CreativeUnlocks[] = {
        {TEXT("eco.leaf-roof"), TEXT("reward.eco.leaf-roof-unlock"), TEXT("intervention.ecosystem.soil-buffer")},
        {TEXT("eco.rainforest-planter"), TEXT("reward.eco.rainforest-planter-unlock"), TEXT("intervention.ecosystem.shade-shelter")},
        {TEXT("eco.bamboo-bridge"), TEXT("reward.eco.bamboo-bridge-unlock"), TEXT("intervention.ecosystem.habitat-garden")}
    };
}

FName UWMChildJourneySubsystem::MakeCardId(const TCHAR* Prefix, const FName SourceId)
{
    return SourceId.IsNone() ? NAME_None : FName(*(FString(Prefix) + SourceId.ToString()));
}

FWMChildJourneySnapshot UWMChildJourneySubsystem::GetSnapshot() const
{
    FWMChildJourneySnapshot Snapshot;
    const UWorld* World = GetWorld();
    if (!World) return Snapshot;

    const UWMBiomeRuntimeSubsystem* Biome = World->GetSubsystem<UWMBiomeRuntimeSubsystem>();
    const UWMMissionRuntimeSubsystem* Missions = World->GetSubsystem<UWMMissionRuntimeSubsystem>();
    const UWMEcologicalBuildSubsystem* EcologicalBuild = World->GetSubsystem<UWMEcologicalBuildSubsystem>();
    const UWMEnvironmentStateSubsystem* Environment = World->GetSubsystem<UWMEnvironmentStateSubsystem>();
    const UGameInstance* GameInstance = World->GetGameInstance();
    const UWMBuildUnlockSubsystem* Unlocks = GameInstance ? GameInstance->GetSubsystem<UWMBuildUnlockSubsystem>() : nullptr;

    if (Biome)
    {
        Snapshot.BiomeId = Biome->GetActiveBiomeId();
        Snapshot.ZoneId = Biome->GetCurrentZoneId();

        const TArray<FName> KnownObservationIds = Biome->GetKnownObservationIds();
        const TArray<FName> ObservedIds = Biome->GetObservedIds();
        if (!KnownObservationIds.IsEmpty())
        {
            int32 ObservedCount = 0;
            for (const FName ObservationId : KnownObservationIds)
            {
                if (ObservedIds.Contains(ObservationId)) ++ObservedCount;
            }

            FWMChildAdventureCard DiscoveryCard;
            DiscoveryCard.CardId = TEXT("adventure.discovery.rainforest-detective");
            DiscoveryCard.SourceId = TEXT("journey.rainforest-observations");
            DiscoveryCard.Kind = EWMChildAdventureKind::Discovery;
            DiscoveryCard.State = FWMChildJourneyRules::FromCountProgress(ObservedCount, KnownObservationIds.Num());
            DiscoveryCard.CurrentUnits = ObservedCount;
            DiscoveryCard.TotalUnits = KnownObservationIds.Num();
            DiscoveryCard.ProgressFraction = static_cast<float>(ObservedCount) / static_cast<float>(KnownObservationIds.Num());
            Snapshot.Cards.Add(DiscoveryCard);
        }
    }

    if (Environment)
    {
        Snapshot.EcosystemReactionId = Environment->GetStateSnapshot().ReactionId;
    }

    if (Missions)
    {
        const FName ActiveMissionId = Missions->GetActiveMissionId();
        for (const FWMJourneyMissionReadModel& Mission : Missions->GetJourneyReadModel())
        {
            FWMChildAdventureCard Card;
            Card.CardId = MakeCardId(TEXT("adventure.mission."), Mission.MissionId);
            Card.SourceId = Mission.MissionId;
            Card.Kind = EWMChildAdventureKind::Mission;
            Card.State = FWMChildJourneyRules::FromMissionState(Mission.State);
            Card.ProgressFraction = FMath::Clamp(Mission.ProgressFraction, 0.0f, 1.0f);
            Card.CurrentUnits = FMath::Clamp(FMath::RoundToInt(Card.ProgressFraction * 100.0f), 0, 100);
            Card.TotalUnits = 100;
            Card.bSelectable = Mission.State == EWMJourneyMissionState::Available || Mission.State == EWMJourneyMissionState::Active;

            if (Mission.MissionId == ActiveMissionId && Missions->GetActiveEvaluator() == FName(TEXT("observe-ecosystem")))
            {
                const int32 RequiredCount = Missions->GetRequiredObservationIds().Num();
                if (RequiredCount > 0)
                {
                    Card.CurrentUnits = FMath::Clamp(Missions->GetRecordedObservationCount(), 0, RequiredCount);
                    Card.TotalUnits = RequiredCount;
                    Card.ProgressFraction = static_cast<float>(Card.CurrentUnits) / static_cast<float>(RequiredCount);
                }
            }
            Snapshot.Cards.Add(Card);
        }
    }

    TArray<FWMEcologicalBuildInterventionReadModel> InterventionCards;
    if (EcologicalBuild)
    {
        InterventionCards = EcologicalBuild->GetInterventionReadModel();
        for (const FWMEcologicalBuildInterventionReadModel& Intervention : InterventionCards)
        {
            FWMChildAdventureCard Card;
            Card.CardId = MakeCardId(TEXT("adventure.care."), Intervention.InterventionId);
            Card.SourceId = Intervention.InterventionId;
            Card.Kind = EWMChildAdventureKind::EcosystemCare;
            Card.State = FWMChildJourneyRules::FromInterventionState(
                Intervention.bCompleted,
                Intervention.bPrerequisitesSatisfied,
                Intervention.SatisfiedRequirementCount,
                FMath::Max(Intervention.RequirementCount, 1));
            Card.CurrentUnits = Intervention.bCompleted
                ? FMath::Max(Intervention.RequirementCount, 1)
                : FMath::Clamp(Intervention.SatisfiedRequirementCount, 0, FMath::Max(Intervention.RequirementCount, 1));
            Card.TotalUnits = FMath::Max(Intervention.RequirementCount, 1);
            Card.ProgressFraction = static_cast<float>(Card.CurrentUnits) / static_cast<float>(Card.TotalUnits);
            Snapshot.Cards.Add(Card);
        }
    }

    for (const FCreativeUnlockProjection& Projection : CreativeUnlocks)
    {
        const bool bGranted = Unlocks && Unlocks->IsRewardGranted(Projection.RewardId);
        bool bPrerequisiteVisible = false;
        for (const FWMEcologicalBuildInterventionReadModel& Intervention : InterventionCards)
        {
            if (Intervention.InterventionId == Projection.InterventionId)
            {
                bPrerequisiteVisible = Intervention.bPrerequisitesSatisfied || Intervention.bCompleted;
                break;
            }
        }

        FWMChildAdventureCard Card;
        Card.CardId = MakeCardId(TEXT("adventure.unlock."), Projection.PieceId);
        Card.SourceId = Projection.PieceId;
        Card.Kind = EWMChildAdventureKind::CreativeUnlock;
        Card.State = FWMChildJourneyRules::FromUnlockState(bGranted, bPrerequisiteVisible);
        Card.CurrentUnits = bGranted ? 1 : 0;
        Card.TotalUnits = 1;
        Card.ProgressFraction = bGranted ? 1.0f : 0.0f;
        Snapshot.Cards.Add(Card);
    }

    return Snapshot;
}

bool UWMChildJourneySubsystem::ActivateRecommendedMission()
{
    const FWMChildJourneySnapshot Snapshot = GetSnapshot();
    for (const FWMChildAdventureCard& Card : Snapshot.Cards)
    {
        if (Card.Kind == EWMChildAdventureKind::Mission && Card.State == EWMChildAdventureState::InProgress && Card.bSelectable)
        {
            return ActivateAdventure(Card.SourceId);
        }
    }
    for (const FWMChildAdventureCard& Card : Snapshot.Cards)
    {
        if (Card.Kind == EWMChildAdventureKind::Mission && Card.State == EWMChildAdventureState::Ready && Card.bSelectable)
        {
            return ActivateAdventure(Card.SourceId);
        }
    }
    return false;
}

bool UWMChildJourneySubsystem::ActivateAdventure(const FName SourceId)
{
    UWorld* World = GetWorld();
    UWMMissionRuntimeSubsystem* Missions = World ? World->GetSubsystem<UWMMissionRuntimeSubsystem>() : nullptr;
    if (!Missions || SourceId.IsNone()) return false;

    for (const FWMChildAdventureCard& Card : GetSnapshot().Cards)
    {
        if (Card.SourceId != SourceId || Card.Kind != EWMChildAdventureKind::Mission || !Card.bSelectable)
        {
            continue;
        }
        if (Card.State == EWMChildAdventureState::InProgress && Missions->GetActiveMissionId() == SourceId)
        {
            return true;
        }
        return Card.State == EWMChildAdventureState::Ready && Missions->ActivateMission(SourceId);
    }
    return false;
}

FText UWMChildJourneySubsystem::ResolveChildTitle(const FName SourceId)
{
    if (SourceId == TEXT("journey.rainforest-observations")) return LOCTEXT("RainforestDetectiveTitle", "Rainforest Detective");
    if (SourceId == TEXT("mission.mathematics.measure-and-build-01")) return LOCTEXT("MeasureBuildOneTitle", "Measure and Build");
    if (SourceId == TEXT("mission.mathematics.measure-and-build-02")) return LOCTEXT("MeasureBuildTwoTitle", "The Short Span Challenge");
    if (SourceId == TEXT("mission.science.rainforest-ecosystem-01")) return LOCTEXT("RainforestScienceTitle", "Discover the Rainforest");
    if (SourceId == TEXT("intervention.ecosystem.soil-buffer")) return LOCTEXT("SoilBufferTitle", "Protect the Forest Floor");
    if (SourceId == TEXT("intervention.ecosystem.shade-shelter")) return LOCTEXT("ShadeShelterTitle", "Make a Shady Shelter");
    if (SourceId == TEXT("intervention.ecosystem.habitat-garden")) return LOCTEXT("HabitatGardenTitle", "Create a Habitat Garden");
    if (SourceId == TEXT("eco.leaf-roof")) return LOCTEXT("LeafRoofTitle", "Leaf Roof");
    if (SourceId == TEXT("eco.rainforest-planter")) return LOCTEXT("PlanterTitle", "Rainforest Planter");
    if (SourceId == TEXT("eco.bamboo-bridge")) return LOCTEXT("BambooBridgeTitle", "Bamboo Bridge");
    return LOCTEXT("AdventureFallbackTitle", "An Adventure");
}

FText UWMChildJourneySubsystem::ResolveChildDescription(const FName SourceId)
{
    if (SourceId == TEXT("journey.rainforest-observations")) return LOCTEXT("RainforestDetectiveDescription", "Look closely and find the rainforest clues around you.");
    if (SourceId == TEXT("mission.mathematics.measure-and-build-01")) return LOCTEXT("MeasureBuildOneDescription", "Measure a space, then build something that fits.");
    if (SourceId == TEXT("mission.mathematics.measure-and-build-02")) return LOCTEXT("MeasureBuildTwoDescription", "Try another measuring challenge with a shorter span.");
    if (SourceId == TEXT("mission.science.rainforest-ecosystem-01")) return LOCTEXT("RainforestScienceDescription", "Observe the ceiba, bromeliads and water to understand their habitat.");
    if (SourceId == TEXT("intervention.ecosystem.soil-buffer")) return LOCTEXT("SoilBufferDescription", "Build two walls in the care area to help protect the soil.");
    if (SourceId == TEXT("intervention.ecosystem.shade-shelter")) return LOCTEXT("ShadeShelterDescription", "Use two pillars and a leaf roof to make protective shade.");
    if (SourceId == TEXT("intervention.ecosystem.habitat-garden")) return LOCTEXT("HabitatGardenDescription", "Use planters and a floor piece to create a small habitat garden.");
    if (SourceId == TEXT("eco.leaf-roof")) return LOCTEXT("LeafRoofDescription", "A new roof piece for nature-friendly creations.");
    if (SourceId == TEXT("eco.rainforest-planter")) return LOCTEXT("PlanterDescription", "A new planter piece for habitat builds.");
    if (SourceId == TEXT("eco.bamboo-bridge")) return LOCTEXT("BambooBridgeDescription", "A new bridge piece for creative rainforest routes.");
    return LOCTEXT("AdventureFallbackDescription", "Explore, try ideas and learn from what you make.");
}

FText UWMChildJourneySubsystem::ResolveChildStateLabel(const EWMChildAdventureState State)
{
    switch (State)
    {
        case EWMChildAdventureState::Ready: return LOCTEXT("StateReady", "Ready to explore");
        case EWMChildAdventureState::InProgress: return LOCTEXT("StateInProgress", "In progress");
        case EWMChildAdventureState::Complete: return LOCTEXT("StateComplete", "Complete");
        default: return FText::GetEmpty();
    }
}

FText UWMChildJourneySubsystem::ResolveEcosystemLabel(const FName ReactionId)
{
    if (ReactionId == TEXT("reaction.ecosystem.stressed")) return LOCTEXT("ForestNeedsCare", "The forest needs some care.");
    if (ReactionId == TEXT("reaction.ecosystem.recovering")) return LOCTEXT("ForestRecovering", "The forest is recovering.");
    if (ReactionId == TEXT("reaction.ecosystem.thriving")) return LOCTEXT("ForestThriving", "The forest is thriving.");
    return LOCTEXT("ForestExplore", "Explore the forest and see what you discover.");
}

FText UWMChildJourneySubsystem::ResolveZoneLabel(const FName ZoneId)
{
    if (ZoneId == TEXT("zone.rainforest.build-clearing")) return LOCTEXT("ZoneBuildClearing", "Building Clearing");
    if (ZoneId == TEXT("zone.rainforest.canopy-trail")) return LOCTEXT("ZoneCanopyTrail", "Canopy Trail");
    if (ZoneId == TEXT("zone.rainforest.understory-west")) return LOCTEXT("ZoneUnderstory", "Forest Understory");
    if (ZoneId == TEXT("zone.rainforest.water-edge")) return LOCTEXT("ZoneWaterEdge", "Water's Edge");
    return LOCTEXT("ZoneExploring", "Exploring");
}

#undef LOCTEXT_NAMESPACE
