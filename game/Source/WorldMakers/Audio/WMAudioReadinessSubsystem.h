#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "WMAudioReadinessSubsystem.generated.h"

UENUM(BlueprintType)
enum class EWMAudioBus : uint8
{
    Master,
    Music,
    Ambience,
    SFX,
    UI,
    Voice
};

USTRUCT(BlueprintType)
struct FWMAudioUserSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World Makers|Audio")
    float MasterVolume = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World Makers|Audio")
    float MusicVolume = 0.85f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World Makers|Audio")
    float AmbienceVolume = 0.9f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World Makers|Audio")
    float SFXVolume = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World Makers|Audio")
    float UIVolume = 0.9f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World Makers|Audio")
    float VoiceVolume = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World Makers|Audio")
    bool bMuteWhenBackgrounded = true;
};

USTRUCT(BlueprintType)
struct FWMSemanticAudioCue
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World Makers|Audio")
    FName CueId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World Makers|Audio")
    EWMAudioBus Bus = EWMAudioBus::SFX;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World Makers|Audio")
    float Intensity = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World Makers|Audio")
    FVector WorldLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World Makers|Audio")
    bool bSpatial = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWMSemanticAudioCueRequested, const FWMSemanticAudioCue&, Cue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWMAudioSettingsChanged, const FWMAudioUserSettings&, Settings);

/**
 * Asset-independent audio boundary used before native SoundWave/MetaSound authoring.
 * Gameplay emits stable semantic cue IDs; authored audio remains replaceable behind
 * those IDs and can be materialized later without changing gameplay rules.
 */
UCLASS()
class WORLDMAKERS_API UWMAudioReadinessSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable, Category="World Makers|Audio")
    FWMSemanticAudioCueRequested OnSemanticAudioCueRequested;

    UPROPERTY(BlueprintAssignable, Category="World Makers|Audio")
    FWMAudioSettingsChanged OnAudioSettingsChanged;

    UFUNCTION(BlueprintCallable, Category="World Makers|Audio")
    void RequestSemanticCue(const FWMSemanticAudioCue& Cue);

    UFUNCTION(BlueprintCallable, Category="World Makers|Audio")
    void SetAudioSettings(const FWMAudioUserSettings& NewSettings);

    UFUNCTION(BlueprintPure, Category="World Makers|Audio")
    FWMAudioUserSettings GetAudioSettings() const { return AudioSettings; }

    UFUNCTION(BlueprintPure, Category="World Makers|Audio")
    static bool IsCanonicalCueId(FName CueId);

private:
    UPROPERTY()
    FWMAudioUserSettings AudioSettings;
};
