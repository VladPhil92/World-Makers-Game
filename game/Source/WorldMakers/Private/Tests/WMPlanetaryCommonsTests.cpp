#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Environment/WMPlanetaryCommonsTypes.h"

namespace
{
    FWMPlanetaryCommonsDefinition BuildDefinition()
    {
        FWMPlanetaryCommonsDefinition D;
        D.SchemaVersion = 1;
        D.SimulationId = TEXT("simulation.planetary-commons-living-world.v1");
        D.bPrototypeOnly = true;
        D.bDeterministicStepModel = true;
        D.InitialWaterQuality = 0.62f;
        D.InitialAirQuality = 0.78f;
        D.InitialSoilHealth = 0.58f;
        D.InitialBiodiversity = 0.55f;
        D.InitialEnergyReliability = 0.60f;
        D.InitialMaterialDemand = 0.20f;
        D.InitialComputeEnergyDemand = 0.10f;
        D.InitialHumanWellbeing = 0.64f;
        D.InitialCivicLegitimacy = 0.55f;
        D.ErosionToWaterRate = 0.05f;
        D.EcologyRelaxationRate = 0.10f;
        D.WellbeingRelaxationRate = 0.08f;
        D.MaterialSoilPressureRate = 0.012f;
        D.ComputeReliabilityPressureRate = 0.010f;

        D.HabitatLinks = {
            { TEXT("habitat-link.river-to-understory"), 0.72f },
            { TEXT("habitat-link.understory-to-canopy"), 0.68f },
            { TEXT("habitat-link.north-to-south-forest"), 0.54f }
        };

        D.SpeciesProfiles = {
            { TEXT("species.prototype.river-frog"), TEXT("draft"), 0.72f, 0.55f, 0.58f, 0.52f, 0.48f, 0.85f },
            { TEXT("species.prototype.canopy-pollinator"), TEXT("draft"), 0.48f, 0.68f, 0.42f, 0.70f, 0.62f, 0.60f },
            { TEXT("species.prototype.forest-cat"), TEXT("draft"), 0.55f, 0.52f, 0.45f, 0.66f, 0.78f, 0.90f }
        };

        FWMCommonsInterventionDefinition Road;
        Road.InterventionId = TEXT("intervention.road-fragmentation");
        Road.MaxApplications = 1;
        Road.Delta.AirQuality = -0.04f;
        Road.Delta.Biodiversity = -0.08f;
        Road.Delta.EnergyReliability = 0.10f;
        Road.Delta.MaterialDemand = 0.20f;
        Road.Delta.HumanWellbeing = 0.05f;
        Road.HabitatLinkDeltas.Add(TEXT("habitat-link.north-to-south-forest"), -0.35f);

        FWMCommonsInterventionDefinition Riparian;
        Riparian.InterventionId = TEXT("intervention.riparian-buffer-restoration");
        Riparian.MaxApplications = 1;
        Riparian.Delta.WaterQuality = 0.14f;
        Riparian.Delta.SoilHealth = 0.10f;
        Riparian.Delta.Biodiversity = 0.05f;
        Riparian.Delta.MaterialDemand = 0.03f;
        Riparian.HabitatLinkDeltas.Add(TEXT("habitat-link.river-to-understory"), 0.12f);

        FWMCommonsInterventionDefinition Corridor;
        Corridor.InterventionId = TEXT("intervention.wildlife-corridor");
        Corridor.MaxApplications = 1;
        Corridor.Delta.Biodiversity = 0.06f;
        Corridor.Delta.MaterialDemand = 0.05f;
        Corridor.HabitatLinkDeltas.Add(TEXT("habitat-link.north-to-south-forest"), 0.30f);

        FWMCommonsInterventionDefinition Compute;
        Compute.InterventionId = TEXT("intervention.compute-expansion");
        Compute.MaxApplications = 1;
        Compute.Delta.ComputeEnergyDemand = 0.25f;
        Compute.Delta.EnergyReliability = -0.08f;
        Compute.Delta.MaterialDemand = 0.08f;
        Compute.Delta.HumanWellbeing = 0.03f;

        D.Interventions = { Road, Riparian, Corridor, Compute };
        return D;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMCommonsDefinitionTest,
    "WorldMakers.Citizenship.C2.Definition.IsSaneAndDerivesHooks",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMCommonsDefinitionTest::RunTest(const FString& Parameters)
{
    const FWMPlanetaryCommonsDefinition Definition = BuildDefinition();
    TestTrue(TEXT("Definition is sane"), Definition.IsSane());

    FWMPlanetaryCommonsModel Model;
    TestTrue(TEXT("Model initializes"), Model.Initialize(Definition));
    const FWMPlanetaryCommonsSnapshot Snapshot = Model.GetSnapshot();
    TestTrue(TEXT("Snapshot is bounded"), Snapshot.IsBounded());
    TestTrue(TEXT("Habitat connectivity is derived"), Snapshot.HabitatConnectivity > 0.0f);
    TestTrue(TEXT("Animal stress is derived"), Snapshot.AnimalStress >= 0.0f);
    TestEqual(TEXT("Unknown hook does not masquerade as valid"), Snapshot.GetHookValue(TEXT("unknown-hook")), -1.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMCommonsLegacyProjectionTest,
    "WorldMakers.Citizenship.C2.LegacyM34.ProjectsAcceptedChangesAsDeltas",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMCommonsLegacyProjectionTest::RunTest(const FString& Parameters)
{
    FWMPlanetaryCommonsModel Model;
    TestTrue(TEXT("Model initializes"), Model.Initialize(BuildDefinition()));

    FWMEnvironmentStateSnapshot First;
    First.BiomeId = TEXT("biome.caribbean-rainforest");
    First.VegetationHealth = 0.40f;
    First.WaterFlow = 0.50f;
    First.SoilProtection = 0.45f;
    First.ShadeCoverage = 0.40f;
    First.HabitatQuality = 0.4375f;
    First.ReactionId = TEXT("ecosystem.recovering");
    TestTrue(TEXT("Legacy seed accepted"), Model.SeedFromLegacyEcosystem(First));

    const float BeforeWater = Model.GetSnapshot().WaterQuality;
    FWMEnvironmentStateSnapshot Second = First;
    Second.WaterFlow = 0.65f;
    Second.SoilProtection = 0.55f;
    Second.VegetationHealth = 0.50f;
    Second.HabitatQuality = 0.525f;
    TestTrue(TEXT("Legacy delta accepted"), Model.ApplyLegacyProjectionDelta(First, Second));
    TestTrue(TEXT("Accepted M3.4 water improvement propagates"), Model.GetSnapshot().WaterQuality > BeforeWater);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMCommonsTradeoffTest,
    "WorldMakers.Citizenship.C2.Causality.InfrastructureTradeoffPropagates",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMCommonsTradeoffTest::RunTest(const FString& Parameters)
{
    FWMPlanetaryCommonsModel Model;
    TestTrue(TEXT("Model initializes"), Model.Initialize(BuildDefinition()));
    const FWMPlanetaryCommonsSnapshot Before = Model.GetSnapshot();

    TestTrue(TEXT("Road intervention accepted once"), Model.ApplyIntervention(TEXT("intervention.road-fragmentation")));
    TestFalse(TEXT("One-shot intervention cannot be farmed"), Model.ApplyIntervention(TEXT("intervention.road-fragmentation")));
    const FWMPlanetaryCommonsSnapshot Immediate = Model.GetSnapshot();
    TestTrue(TEXT("Energy reliability can improve"), Immediate.EnergyReliability > Before.EnergyReliability);
    TestTrue(TEXT("Human wellbeing can improve"), Immediate.HumanWellbeing > Before.HumanWellbeing);
    TestTrue(TEXT("Habitat connectivity falls"), Immediate.HabitatConnectivity < Before.HabitatConnectivity);
    TestTrue(TEXT("Biodiversity falls"), Immediate.Biodiversity < Before.Biodiversity);

    TestTrue(TEXT("Step advances deterministic coupling"), Model.AdvanceStep());
    TestTrue(TEXT("Animal stress responds to the web of relations"), Model.GetSnapshot().AnimalStress > Before.AnimalStress);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMCommonsSpeciesDifferenceTest,
    "WorldMakers.Citizenship.C2.LivingWorld.SpeciesRemainDistinct",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMCommonsSpeciesDifferenceTest::RunTest(const FString& Parameters)
{
    FWMPlanetaryCommonsModel Model;
    TestTrue(TEXT("Model initializes"), Model.Initialize(BuildDefinition()));
    const TArray<FWMSpeciesStateSnapshot>& States = Model.GetSpeciesStates();
    TestEqual(TEXT("Three distinct prototype species are modeled"), States.Num(), 3);
    TestNotEqual(TEXT("Different needs produce different stress"), States[0].Stress, States[2].Stress);
    TestNotEqual(TEXT("Species identities remain explicit"), States[0].SpeciesId, States[1].SpeciesId);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMCommonsRestorationTest,
    "WorldMakers.Citizenship.C2.LivingWorld.RestorationCanReduceStress",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMCommonsRestorationTest::RunTest(const FString& Parameters)
{
    FWMPlanetaryCommonsModel Model;
    TestTrue(TEXT("Model initializes"), Model.Initialize(BuildDefinition()));
    TestTrue(TEXT("Fragmentation accepted"), Model.ApplyIntervention(TEXT("intervention.road-fragmentation")));
    TestTrue(TEXT("Degraded step advances"), Model.AdvanceStep());
    const float DegradedStress = Model.GetSnapshot().AnimalStress;

    TestTrue(TEXT("Riparian restoration accepted"), Model.ApplyIntervention(TEXT("intervention.riparian-buffer-restoration")));
    TestTrue(TEXT("Corridor restoration accepted"), Model.ApplyIntervention(TEXT("intervention.wildlife-corridor")));
    TestTrue(TEXT("Restoration step advances"), Model.AdvanceStep());
    TestTrue(TEXT("Species aggregate stress can fall after restoration"), Model.GetSnapshot().AnimalStress < DegradedStress);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMCommonsBoundedLongRunTest,
    "WorldMakers.Citizenship.C2.Determinism.LongRunRemainsBounded",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMCommonsBoundedLongRunTest::RunTest(const FString& Parameters)
{
    FWMPlanetaryCommonsModel Model;
    TestTrue(TEXT("Model initializes"), Model.Initialize(BuildDefinition()));
    TestTrue(TEXT("Compute expansion accepted"), Model.ApplyIntervention(TEXT("intervention.compute-expansion")));
    for (int32 Index = 0; Index < 250; ++Index)
    {
        TestTrue(TEXT("Every step remains valid"), Model.AdvanceStep());
    }
    TestEqual(TEXT("Discrete step count is deterministic"), Model.GetSnapshot().StepIndex, 250);
    TestTrue(TEXT("Long-run state remains bounded"), Model.GetSnapshot().IsBounded());
    for (const FWMSpeciesStateSnapshot& Species : Model.GetSpeciesStates())
    {
        TestTrue(TEXT("Species state remains bounded"), Species.IsBounded());
    }
    return true;
}

#endif
