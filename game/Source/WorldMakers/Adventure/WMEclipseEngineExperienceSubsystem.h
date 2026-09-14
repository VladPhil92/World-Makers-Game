#pragma once

#include "Adventure/WMEclipseEngineExperience.h"
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WMEclipseEngineExperienceSubsystem.generated.h"

class AWMEclipseEngineInteractableActor;

/**
 * Player-facing orchestration for The Eclipse Engine.
 * The player deals only in diegetic action IDs. Pedagogical IDs remain behind this boundary.
 * BeginAction never grants evidence; only ResolveTrustedAction may do so after a world mechanic validates an outcome.
 */
UCLASS()
class WORLDMAKERS_API UWMEclipseEngineExperienceSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

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

    /** Trusted mechanism completion entry point. This is the only action path that may reach the Epic evidence ledger. */
    UFUNCTION(BlueprintCallable, Category = "World Makers|Eclipse")
    bool ResolveTrustedAction(FName ActionId, float NumericValue = 1.0f);

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

    bool bCatalogLoaded = false;
    FWMEclipseExperienceCatalog Catalog;
    FWMEclipseHintRuntime HintRuntime;

    UPROPERTY(Transient)
    TArray<TObjectPtr<AWMEclipseEngineInteractableActor>> PrototypeTargets;
};
