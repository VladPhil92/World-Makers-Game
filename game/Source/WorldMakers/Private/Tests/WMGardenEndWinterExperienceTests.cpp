#if WITH_DEV_AUTOMATION_TESTS

#include "Adventure/WMGardenEndWinterExperience.h"
#include "Adventure/WMGardenSystemsRuntime.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Science/WMScienceSimulationCore.h"

namespace
{
    bool LoadGarden(FWMGardenExperienceCatalog& Out, FString& Error)
    {
        FString Json;
        const FString Path=FPaths::Combine(FPaths::ProjectContentDir(),TEXT("WorldMakers/Epics/garden-end-winter-player-experience-v1.json"));
        return FFileHelper::LoadFileToString(Json,*Path)&&FWMGardenExperienceCatalog::TryParseJson(Json,Out,Error);
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWMGardenWorldFirstContractTest,"WorldMakers.Garden.PlayerExperience.WorldFirstContract",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWMGardenWorldFirstContractTest::RunTest(const FString& Parameters)
{
    FWMGardenExperienceCatalog Garden; FString Error;
    TestTrue(TEXT("Garden experience loads"),LoadGarden(Garden,Error));
    TestEqual(TEXT("Garden contains four living-world acts"),Garden.Chapters.Num(),4);
    TestEqual(TEXT("No school UI"),Garden.PresentationRule,FName(TEXT("world-first-no-school-ui")));
    TestEqual(TEXT("Intrinsic reward"),Garden.RewardModel,FName(TEXT("living-world-transformation-and-new-access")));
    TestEqual(TEXT("Reversible experimentation"),Garden.FailureModel,FName(TEXT("reversible-ecosystem-experimentation")));
    for(const FWMGardenChapterExperience& Chapter:Garden.Chapters)
    {
        TestTrue(TEXT("Act has several world interactions"),Chapter.Actions.Num()>=3);
        TestTrue(TEXT("Act culminates in visible world consequence"),Chapter.Actions.Last().bHasWorldStateRoute);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWMGardenMasteryGateTest,"WorldMakers.Garden.PlayerExperience.MathAndPhilosophyAreNecessaryNotProfiled",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWMGardenMasteryGateTest::RunTest(const FString& Parameters)
{
    FWMGardenExperienceCatalog Garden; FString Error;
    TestTrue(TEXT("Garden experience loads"),LoadGarden(Garden,Error));
    TSet<FName> Disciplines;
    int32 GateCount=0;
    for(const FWMGardenChapterExperience& Chapter:Garden.Chapters)
    {
        for(const FWMGardenActionDefinition& Action:Chapter.Actions)
        {
            if(Action.bHasMasteryGate){++GateCount;Disciplines.Add(Action.MasteryGate.DisciplineId);TestFalse(TEXT("Mastery gate is not persistent evidence"),Action.bHasEvidenceRoute);}
        }
    }
    TestEqual(TEXT("Exactly two minimized mastery gates"),GateCount,2);
    TestTrue(TEXT("Mathematics is mechanically necessary"),Disciplines.Contains(TEXT("mathematics")));
    TestTrue(TEXT("Philosophy is mechanically necessary"),Disciplines.Contains(TEXT("philosophy-for-children")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWMGardenLivingSystemsTest,"WorldMakers.Garden.Gameplay.LivingSystemsRequireCausalModeling",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWMGardenLivingSystemsTest::RunTest(const FString& Parameters)
{
    TestFalse(TEXT("Starved cells do not count as restored"),FWMGardenSystemsRuntime::IsCellBalanceRestored(0.2f,0.9f,0.9f,0.9f));
    TestTrue(TEXT("Balanced resources restore cell response"),FWMGardenSystemsRuntime::IsCellBalanceRestored(0.9f,0.9f,0.9f,0.9f));
    TestFalse(TEXT("Blocked transport fails"),FWMGardenSystemsRuntime::IsTransportFlowRestored(0.3f,0.4f));
    TestTrue(TEXT("Strong membrane and transport restore flow"),FWMGardenSystemsRuntime::IsTransportFlowRestored(0.95f,0.95f));
    TestFalse(TEXT("Unequal water scaling fails"),FWMGardenSystemsRuntime::IsIrrigationRatioBalanced(2,3,4,5));
    TestTrue(TEXT("Equivalent water scaling solves physical ratio"),FWMGardenSystemsRuntime::IsIrrigationRatioBalanced(2,3,4,6));

    FWMSubstanceDefinition Salt;
    Salt.SubstanceId=TEXT("substance.sodium-chloride"); Salt.MolarMassGPerMol=58.44f; Salt.MeltingPointC=801.0f; Salt.BoilingPointC=1465.0f; Salt.SolubilityGPer100MlWater=35.9f;
    TestFalse(TEXT("Unsaturated water does not solve threshold"),FWMGardenSystemsRuntime::IsSaturationThresholdFound(Salt,20.0f,100.0f));
    TestTrue(TEXT("Saturation becomes visible through undissolved material"),FWMGardenSystemsRuntime::IsSaturationThresholdFound(Salt,40.0f,100.0f));

    FWMPlantSpeciesDefinition Plant;
    Plant.SpeciesId=TEXT("plant.test"); Plant.MinTemperatureC=15.0f; Plant.OptimalTemperatureC=27.0f; Plant.MaxTemperatureC=40.0f; Plant.GerminationHoursAtIdeal=48.0f; Plant.BaseGrowthUnitsPerHour=0.012f; Plant.FloweringBiomass=1.0f; Plant.FruitingBiomass=1.5f;
    TestFalse(TEXT("Small water difference is not enough to identify bottleneck"),FWMGardenSystemsRuntime::IsGrowthLimitingFactorDemonstrated(Plant,0.55f,0.70f));
    TestTrue(TEXT("Strong controlled response identifies limiting water"),FWMGardenSystemsRuntime::IsGrowthLimitingFactorDemonstrated(Plant,0.20f,0.95f));
    TestTrue(TEXT("Pollination must causally unlock fruiting"),FWMGardenSystemsRuntime::IsPollinationFruitingLinkDemonstrated(Plant));
    return true;
}

#endif
