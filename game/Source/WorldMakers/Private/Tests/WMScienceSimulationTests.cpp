#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Science/WMScienceSimulationCore.h"

namespace
{
    FWMScienceSimulationCatalog BuildScienceTestCatalog()
    {
        FWMScienceSimulationCatalog Catalog;

        FWMSubstanceDefinition Iron;
        Iron.SubstanceId = TEXT("substance.iron");
        Iron.MolarMassGPerMol = 55.845f;
        Iron.MeltingPointC = 1538.0f;
        Iron.BoilingPointC = 2862.0f;
        Iron.SolubilityGPer100MlWater = 0.0f;
        Catalog.Substances.Add(Iron.SubstanceId, Iron);

        FWMSubstanceDefinition Oxygen;
        Oxygen.SubstanceId = TEXT("substance.oxygen");
        Oxygen.MolarMassGPerMol = 31.998f;
        Oxygen.MeltingPointC = -218.79f;
        Oxygen.BoilingPointC = -182.95f;
        Oxygen.SolubilityGPer100MlWater = 0.004f;
        Catalog.Substances.Add(Oxygen.SubstanceId, Oxygen);

        FWMSubstanceDefinition IronOxide;
        IronOxide.SubstanceId = TEXT("substance.iron-oxide-fe2o3");
        IronOxide.MolarMassGPerMol = 159.687f;
        IronOxide.MeltingPointC = 1565.0f;
        IronOxide.BoilingPointC = 3000.0f;
        IronOxide.SolubilityGPer100MlWater = 0.0f;
        Catalog.Substances.Add(IronOxide.SubstanceId, IronOxide);

        FWMReactionDefinition Reaction;
        Reaction.ReactionId = TEXT("reaction.iron-oxidation");
        Reaction.EvidenceEventId = TEXT("science.chemistry.mass-conservation-demonstrated");
        Reaction.Reactants = {
            { TEXT("substance.iron"), 4 },
            { TEXT("substance.oxygen"), 3 }
        };
        Reaction.Products = {
            { TEXT("substance.iron-oxide-fe2o3"), 2 }
        };
        Catalog.Reactions.Add(Reaction.ReactionId, Reaction);

        FWMPlantSpeciesDefinition Plant;
        Plant.SpeciesId = TEXT("plant.rainforest-learning-prototype");
        Plant.MinTemperatureC = 15.0f;
        Plant.OptimalTemperatureC = 27.0f;
        Plant.MaxTemperatureC = 40.0f;
        Plant.GerminationHoursAtIdeal = 48.0f;
        Plant.BaseGrowthUnitsPerHour = 0.012f;
        Plant.FloweringBiomass = 1.0f;
        Plant.FruitingBiomass = 1.5f;
        Catalog.PlantSpecies.Add(Plant.SpeciesId, Plant);

        return Catalog;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMScienceMatterAndSolubilityTest,
    "WorldMakers.Science.Chemistry.MatterStateAndSaturation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMScienceMatterAndSolubilityTest::RunTest(const FString& Parameters)
{
    FWMSubstanceDefinition Water;
    Water.SubstanceId = TEXT("substance.water");
    Water.MolarMassGPerMol = 18.01528f;
    Water.MeltingPointC = 0.0f;
    Water.BoilingPointC = 100.0f;
    Water.SolubilityGPer100MlWater = 1000000.0f;

    TestEqual(TEXT("Water below freezing resolves solid"), Water.ResolveMatterState(-10.0f), EWMMatterState::Solid);
    TestEqual(TEXT("Water at room temperature resolves liquid"), Water.ResolveMatterState(20.0f), EWMMatterState::Liquid);
    TestEqual(TEXT("Water above boiling resolves gas"), Water.ResolveMatterState(110.0f), EWMMatterState::Gas);

    FWMSubstanceDefinition Salt;
    Salt.SubstanceId = TEXT("substance.sodium-chloride");
    Salt.MolarMassGPerMol = 58.44f;
    Salt.MeltingPointC = 801.0f;
    Salt.BoilingPointC = 1465.0f;
    Salt.SolubilityGPer100MlWater = 35.9f;

    FWMDissolutionResult Result;
    TestTrue(TEXT("Dissolution simulation accepts bounded input"), FWMScienceSimulation::DissolveInWater(Salt, 50.0f, 100.0f, Result));
    TestTrue(TEXT("Solution becomes saturated"), Result.bSaturated);
    TestTrue(TEXT("Dissolved mass respects configured solubility"), FMath::IsNearlyEqual(Result.DissolvedMassG, 35.9f, 0.001f));
    TestTrue(TEXT("Undissolved mass is conserved"), FMath::IsNearlyEqual(Result.UndissolvedMassG, 14.1f, 0.001f));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMScienceReactionConservationTest,
    "WorldMakers.Science.Chemistry.StoichiometryConservesMass",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMScienceReactionConservationTest::RunTest(const FString& Parameters)
{
    const FWMScienceSimulationCatalog Catalog = BuildScienceTestCatalog();
    TestTrue(TEXT("Science catalog is semantically valid"), Catalog.IsSane());

    TMap<FName, float> Inventory;
    Inventory.Add(TEXT("substance.iron"), 8.0f);
    Inventory.Add(TEXT("substance.oxygen"), 3.0f);

    FWMReactionResult Result;
    TestTrue(TEXT("Balanced oxidation reaction executes"), FWMScienceSimulation::ExecuteReaction(
        Catalog,
        *Catalog.FindReaction(TEXT("reaction.iron-oxidation")),
        Inventory,
        Result));
    TestTrue(TEXT("Limiting reagent extent is one mole-reaction"), FMath::IsNearlyEqual(Result.ReactionExtentMol, 1.0f));
    TestTrue(TEXT("Reaction mass is conserved"), FMath::IsNearlyEqual(Result.ReactantMassConsumedG, Result.ProductMassCreatedG, 0.05f));
    TestTrue(TEXT("Excess iron remains"), FMath::IsNearlyEqual(Result.AfterMoles.FindRef(TEXT("substance.iron")), 4.0f));
    TestTrue(TEXT("Oxygen limiting reagent is consumed"), FMath::IsNearlyZero(Result.AfterMoles.FindRef(TEXT("substance.oxygen"))));
    TestTrue(TEXT("Product stoichiometry is applied"), FMath::IsNearlyEqual(Result.AfterMoles.FindRef(TEXT("substance.iron-oxide-fe2o3")), 2.0f));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMSciencePhysicsTest,
    "WorldMakers.Science.Physics.ForceMomentumEnergyAndCircuit",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMSciencePhysicsTest::RunTest(const FString& Parameters)
{
    FWMPhysicsBodyState Body;
    Body.MassKg = 2.0f;
    TestTrue(TEXT("Constant force step succeeds"), Body.StepConstantForce(FVector(4.0f, 0.0f, 0.0f), 3.0f));
    TestTrue(TEXT("Velocity follows F=ma"), FMath::IsNearlyEqual(Body.VelocityMetersPerSecond.X, 6.0f));
    TestTrue(TEXT("Position follows constant acceleration kinematics"), FMath::IsNearlyEqual(Body.PositionMeters.X, 9.0f));
    TestTrue(TEXT("Momentum is mv"), FMath::IsNearlyEqual(Body.GetMomentumKgMetersPerSecond().X, 12.0f));
    TestTrue(TEXT("Kinetic energy is one-half mv squared"), FMath::IsNearlyEqual(Body.GetKineticEnergyJoules(), 36.0f));

    FWMDirectCurrentCircuit Circuit;
    Circuit.VoltageVolts = 12.0f;
    Circuit.ResistanceOhms = 6.0f;
    float Current = 0.0f;
    float Power = 0.0f;
    TestTrue(TEXT("Ideal DC circuit solves"), Circuit.Solve(Current, Power));
    TestTrue(TEXT("Ohm law current"), FMath::IsNearlyEqual(Current, 2.0f));
    TestTrue(TEXT("Electrical power"), FMath::IsNearlyEqual(Power, 24.0f));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWMScienceBotanyTest,
    "WorldMakers.Science.Botany.GerminationGrowthAndEcologyCoupling",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWMScienceBotanyTest::RunTest(const FString& Parameters)
{
    const FWMScienceSimulationCatalog Catalog = BuildScienceTestCatalog();
    const FWMPlantSpeciesDefinition& Species = *Catalog.FindPlantSpecies(TEXT("plant.rainforest-learning-prototype"));

    FWMPlantEnvironmentInput Ideal;
    Ideal.WaterAvailability = 1.0f;
    Ideal.NutrientAvailability = 1.0f;
    Ideal.LightAvailability = 1.0f;
    Ideal.PollinatorAvailability = 1.0f;
    Ideal.TemperatureC = 27.0f;

    FWMPlantState Plant;
    FWMPlantStepResult GerminationResult;
    TestTrue(TEXT("First germination interval accepted"), Plant.Step(Species, Ideal, 24.0f, GerminationResult));
    TestEqual(TEXT("Seed enters germinating stage"), Plant.Stage, EWMPlantStage::Germinating);
    TestTrue(TEXT("Second germination interval accepted"), Plant.Step(Species, Ideal, 24.0f, GerminationResult));
    TestEqual(TEXT("Ideal germination reaches seedling"), Plant.Stage, EWMPlantStage::Seedling);

    FWMPlantState IdealSeedling = Plant;
    FWMPlantState DrySeedling = Plant;
    FWMPlantStepResult IdealGrowth;
    FWMPlantStepResult DryGrowth;
    FWMPlantEnvironmentInput Dry = Ideal;
    Dry.WaterAvailability = 0.2f;
    TestTrue(TEXT("Ideal growth step accepted"), IdealSeedling.Step(Species, Ideal, 24.0f, IdealGrowth));
    TestTrue(TEXT("Water-limited growth step accepted"), DrySeedling.Step(Species, Dry, 24.0f, DryGrowth));
    TestTrue(TEXT("Limiting water reduces growth"), IdealGrowth.GrowthUnits > DryGrowth.GrowthUnits);

    const FWMEnvironmentStateDelta Delta = FWMScienceSimulation::BuildPlantEnvironmentDelta(IdealGrowth);
    TestTrue(TEXT("Plant growth contributes vegetation health"), Delta.VegetationHealth > 0.0f);
    TestTrue(TEXT("Root development contributes soil protection"), Delta.SoilProtection > 0.0f);
    return true;
}

#endif
