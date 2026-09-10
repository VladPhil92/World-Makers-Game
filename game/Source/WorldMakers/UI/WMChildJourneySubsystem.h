#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "UI/WMChildJourneyTypes.h"
#include "WMChildJourneySubsystem.generated.h"

UCLASS()
class WORLDMAKERS_API UWMChildJourneySubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    /** Read-only composition of trusted domain projections for child-facing UI. */
    UFUNCTION(BlueprintPure, Category = "World Makers|Child Journey")
    FWMChildJourneySnapshot GetSnapshot() const;

    /** The only M3.6 write: activate an already-available mission chosen from trusted mission state. */
    UFUNCTION(BlueprintCallable, Category = "World Makers|Child Journey")
    bool ActivateRecommendedMission();

    UFUNCTION(BlueprintCallable, Category = "World Makers|Child Journey")
    bool ActivateAdventure(FName SourceId);

    static FText ResolveChildTitle(FName SourceId);
    static FText ResolveChildDescription(FName SourceId);
    static FText ResolveChildStateLabel(EWMChildAdventureState State);
    static FText ResolveEcosystemLabel(FName ReactionId);
    static FText ResolveZoneLabel(FName ZoneId);

private:
    static FName MakeCardId(const TCHAR* Prefix, FName SourceId);
};
