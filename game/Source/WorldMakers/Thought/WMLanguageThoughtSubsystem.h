#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Thought/WMLanguageThoughtRuntime.h"
#include "WMLanguageThoughtSubsystem.generated.h"

UCLASS()
class WORLDMAKERS_API UWMLanguageThoughtSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "World Makers|Language Thought")
    bool ReloadLanguageThoughtCatalog();

    UFUNCTION(BlueprintPure, Category = "World Makers|Language Thought")
    bool IsCatalogLoaded() const { return bCatalogLoaded; }

    UFUNCTION(BlueprintPure, Category = "World Makers|Language Thought")
    int32 GetCommunicationChallengeCount() const { return Catalog.CommunicationChallenges.Num(); }

    UFUNCTION(BlueprintPure, Category = "World Makers|Language Thought")
    int32 GetNarrativeStoryCount() const { return Catalog.NarrativeStories.Num(); }

    UFUNCTION(BlueprintPure, Category = "World Makers|Language Thought")
    int32 GetEthicalDilemmaCount() const { return Catalog.EthicalDilemmas.Num(); }

    UFUNCTION(BlueprintPure, Category = "World Makers|Language Thought")
    int32 GetPhilosophyProblemCount() const { return Catalog.PhilosophyProblems.Num(); }

    UFUNCTION(BlueprintCallable, Category = "World Makers|Language Thought")
    bool EvaluateCommunicationAndRecord(FName ChallengeId, FName ChoiceId);

    UFUNCTION(BlueprintCallable, Category = "World Makers|Language Thought")
    bool TraverseNarrativeAndRecord(FName StoryId, FName CurrentNodeId, FName ChoiceId, FName& OutNextNodeId);

    UFUNCTION(BlueprintCallable, Category = "World Makers|Language Thought")
    bool EvaluateEthicalReasoningAndRecord(
        FName DilemmaId,
        FName OptionId,
        const TArray<FName>& ReasonIds,
        const TArray<FName>& PerspectiveIds,
        bool bAcknowledgedTradeoff);

    UFUNCTION(BlueprintCallable, Category = "World Makers|Language Thought")
    bool EvaluatePhilosophicalArgumentAndRecord(
        FName ProblemId,
        FName ClaimId,
        const TArray<FName>& ReasonLinkIds,
        const TArray<FName>& AssumptionIds,
        const TArray<FName>& CounterexampleIds,
        int32 RevisionCount);

    const FWMLanguageThoughtCatalog& GetCatalog() const { return Catalog; }

private:
    /** Routes M5.4 evidence through M5.5 ordered adventures when one is active; otherwise preserves legacy M5.2 behavior. */
    bool SubmitEvidenceToActiveMission(const FWMThoughtEvidenceResult& Result, FName ProducerRefId);

    bool bCatalogLoaded = false;
    FWMLanguageThoughtCatalog Catalog;
};
