#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "WMMissionJourneySaveGame.generated.h"

/**
 * Privacy-minimized local persistence for mission milestones.
 * Stable IDs and format version only: no child identity, free text, analytics IDs or commerce state.
 */
UCLASS()
class WORLDMAKERS_API UWMMissionJourneySaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    static constexpr int32 CurrentFormatVersion = 1;

    UPROPERTY()
    int32 FormatVersion = CurrentFormatVersion;

    UPROPERTY()
    TArray<FName> CompletedMissionIds;

    UPROPERTY()
    TArray<FName> GrantedRewardIds;

    UPROPERTY()
    FName LastActiveMissionId;
};
