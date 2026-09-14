#pragma once

#include "Adventure/WMEclipseEngineExperience.h"
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WMEclipseEngineExperienceSubsystem.generated.h"

class AWMEclipseEngineInteractableActor;

/**
 * Player-facing orchestration for The Eclipse Engine.
 * The player deals only in diegetic action IDs. Pedagogical IDs remain behind this boundary.
 * BeginAction never grants evidence; only validated world/thought/science systems may resolve an action.
 */
UCLASS()
class WORLDMAKERS_API UWMEclipseEngineExperienceSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;

    UFUNCTION(BlueprintCallable, Category = "World Makers|Eclipse")
    bool ReloadExperienceCatalog();

    UFUNCTION(BlueprintCallable, Category = "World Makers|Eclipse")
    bool StartEclipseEngine();

    UFUNCTION(BlueprintCallable, Category = "World Makers|Eclipse")
    bool EnsurePrototypeTargets();

    UFUNCTION(BlueprintPure, Category = "World Makers|Eclipse")
    bool IsExperienceLoaded() const { return bCatalogLoaded; }

    UFUNCTION(BlueprintPure, Category = "World Makers|Eclipse")
    bool CanBeginAction(FName ActionId) const;

    /** Presentation/interaction entry point. Never awards learning evidence. */
    UFUNCTION(BlueprintCallable, Category = "World Makers|Eclipse")
    bool BeginAction(FName ActionId);

    /**
     * Source-proxy gameplay bridge. The interaction step is produced by a world actor, then converted
     * into bounded mechanic state and validated by C++ systems. It is deliberately not Blueprint-callable.
     */
    bool AdvancePrototypeMechanic(FName ActionId, int32 InteractionStep);

    /** Trusted C++ completion boundary. Deliberately not Blueprint/UI-callable. */
    bool ResolveTrustedAction(FName ActionId, float NumericValue = 1.0f);

    /** C++ bridge used by already-validated language/literature/thought runtimes. */
    bool ResolveValidatedEvidence(
        FName ProducerRefId,
        FName PrimitiveId,
        FName EvidenceEventId,
        float NumericValue = 1.0f);

    UFUNCTION(BlueprintCallable, Category = "World Makers|Eclipse")
    FName RequestHint(FName ActionId);

    UFUNCTION(BlueprintPure, Category = "World Makers|Eclipse")
    FName GetCurrentFantasyGoalKey() const;

    UFUNCTION(BlueprintPure, Category = "World Makers|Eclipse")
    FName GetCurrentTensionKey() const;

    UFUNCTION(BlueprintPure, Category = "World Makers|Eclipse")
    TArray<FName> GetCurrentActionIds() const;

    UFUNCTION(BlueprintPure, Category = "World Makers|Eclipse")
    bool IsWorldStateAction(FName ActionId) const;

    const FWMEclipseExperienceCatalog& GetCatalog() const { return Catalog; }

private:
    const FWMEclipseChapterExperience* GetCurrentChapter() const;
    const FWMEclipseActionDefinition* GetCurrentAction(FName ActionId) const;
    void PulseFirstPerson(const FWMEclipseActionDefinition& Action) const;
    void DestroyPrototypeTargets();
    void RefreshPrototypeTargetAvailability();

    bool bCatalogLoaded = false;
    FWMEclipseExperienceCatalog Catalog;
    FWMEclipseHintRuntime HintRuntime;

    UPROPERTY(Transient)
    TArray<TObjectPtr<AWMEclipseEngineInteractableActor>> PrototypeTargets;
};
