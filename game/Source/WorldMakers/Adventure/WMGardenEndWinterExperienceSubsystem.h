#pragma once

#include "Adventure/WMGardenEndWinterExperience.h"
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WMGardenEndWinterExperienceSubsystem.generated.h"

class AWMGardenEndWinterInteractableActor;

UCLASS()
class WORLDMAKERS_API UWMGardenEndWinterExperienceSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;

    UFUNCTION(BlueprintCallable, Category = "World Makers|Garden") bool ReloadExperienceCatalog();
    UFUNCTION(BlueprintCallable, Category = "World Makers|Garden") bool StartGardenEndWinter();
    UFUNCTION(BlueprintCallable, Category = "World Makers|Garden") bool EnsurePrototypeTargets();
    UFUNCTION(BlueprintPure, Category = "World Makers|Garden") bool IsExperienceLoaded() const { return bCatalogLoaded; }
    UFUNCTION(BlueprintPure, Category = "World Makers|Garden") bool CanBeginAction(FName ActionId) const;
    UFUNCTION(BlueprintCallable, Category = "World Makers|Garden") bool BeginAction(FName ActionId);
    UFUNCTION(BlueprintCallable, Category = "World Makers|Garden") FName RequestHint(FName ActionId);
    UFUNCTION(BlueprintPure, Category = "World Makers|Garden") FName GetCurrentFantasyGoalKey() const;
    UFUNCTION(BlueprintPure, Category = "World Makers|Garden") FName GetCurrentTensionKey() const;
    UFUNCTION(BlueprintPure, Category = "World Makers|Garden") bool IsWorldStateAction(FName ActionId) const;

    bool AdvancePrototypeMechanic(FName ActionId, int32 InteractionStep);
    bool ResolveTrustedAction(FName ActionId, float NumericValue = 1.0f);
    bool ResolveValidatedEvidence(FName ProducerRefId, FName PrimitiveId, FName EvidenceEventId, float NumericValue = 1.0f);
    bool ResolveMasteryGate(FName ActionId, FName GateId);

    const FWMGardenExperienceCatalog& GetCatalog() const { return Catalog; }

private:
    const FWMGardenChapterExperience* GetCurrentChapter() const;
    const FWMGardenActionDefinition* GetCurrentAction(FName ActionId) const;
    bool AreCurrentMasteryGatesSatisfied() const;
    void PulseFirstPerson(const FWMGardenActionDefinition& Action) const;
    void RefreshPrototypeTargetAvailability();
    void DestroyPrototypeTargets();

    bool bCatalogLoaded = false;
    FWMGardenExperienceCatalog Catalog;
    FWMGardenHintRuntime HintRuntime;
    TSet<FName> SatisfiedMasteryGates;

    UPROPERTY(Transient)
    TArray<TObjectPtr<AWMGardenEndWinterInteractableActor>> PrototypeTargets;
};
