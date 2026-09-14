#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "WMEpicJourneySaveGame.generated.h"

/**
 * Privacy-minimized chapter checkpoint. Deliberately stores no evidence events,
 * answers, free text, moral choices, hint history, or behavior telemetry.
 */
USTRUCT(BlueprintType)
struct WORLDMAKERS_API FWMEpicCheckpoint
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "World Makers|Epic")
    FName EpicId;

    /** NAME_None only for a completed epic. */
    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "World Makers|Epic")
    FName ChapterId;

    /** Current chapter index; equals ChapterCount when completed. */
    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "World Makers|Epic")
    int32 ChapterIndex = 0;

    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "World Makers|Epic")
    int32 ChapterCount = 0;

    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "World Makers|Epic")
    bool bCompleted = false;
};

UCLASS()
class WORLDMAKERS_API UWMEpicJourneySaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    static constexpr int32 MinimumSupportedFormatVersion = 1;
    static constexpr int32 CurrentFormatVersion = 2;

    UPROPERTY(SaveGame)
    int32 FormatVersion = CurrentFormatVersion;

    UPROPERTY(SaveGame)
    TArray<FWMEpicCheckpoint> Checkpoints;

    /**
     * Durable, privacy-minimized remote-sync outbox. Contains only coalesced chapter checkpoints;
     * launch tickets, sync tokens, player identifiers and learning evidence are never persisted here.
     */
    UPROPERTY(SaveGame)
    TArray<FWMEpicCheckpoint> PendingSyncCheckpoints;
};