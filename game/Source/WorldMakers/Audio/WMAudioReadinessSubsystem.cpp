#include "Audio/WMAudioReadinessSubsystem.h"

namespace
{
const TSet<FName>& CanonicalCueIds()
{
    static const TSet<FName> CueIds = {
        TEXT("audio.ambience.rainforest.day"),
        TEXT("audio.ambience.rainforest.water"),
        TEXT("audio.ambience.rainforest.wind"),
        TEXT("audio.music.discovery"),
        TEXT("audio.music.build"),
        TEXT("audio.sfx.observe.scan"),
        TEXT("audio.sfx.collect"),
        TEXT("audio.sfx.build.preview-valid"),
        TEXT("audio.sfx.build.preview-invalid"),
        TEXT("audio.sfx.build.place"),
        TEXT("audio.sfx.build.remove"),
        TEXT("audio.sfx.science.success"),
        TEXT("audio.sfx.ecosystem.reaction"),
        TEXT("audio.ui.focus"),
        TEXT("audio.ui.confirm"),
        TEXT("audio.ui.cancel"),
        TEXT("audio.ui.error")
    };
    return CueIds;
}

float ClampVolume(float Value)
{
    return FMath::Clamp(Value, 0.0f, 1.0f);
}
}

void UWMAudioReadinessSubsystem::RequestSemanticCue(const FWMSemanticAudioCue& Cue)
{
    if (!IsCanonicalCueId(Cue.CueId))
    {
        UE_LOG(LogTemp, Warning, TEXT("World Makers rejected unknown semantic audio cue '%s'."), *Cue.CueId.ToString());
        return;
    }

    FWMSemanticAudioCue SanitizedCue = Cue;
    SanitizedCue.Intensity = FMath::Clamp(SanitizedCue.Intensity, 0.0f, 1.0f);
    OnSemanticAudioCueRequested.Broadcast(SanitizedCue);
}

void UWMAudioReadinessSubsystem::SetAudioSettings(const FWMAudioUserSettings& NewSettings)
{
    AudioSettings = NewSettings;
    AudioSettings.MasterVolume = ClampVolume(AudioSettings.MasterVolume);
    AudioSettings.MusicVolume = ClampVolume(AudioSettings.MusicVolume);
    AudioSettings.AmbienceVolume = ClampVolume(AudioSettings.AmbienceVolume);
    AudioSettings.SFXVolume = ClampVolume(AudioSettings.SFXVolume);
    AudioSettings.UIVolume = ClampVolume(AudioSettings.UIVolume);
    AudioSettings.VoiceVolume = ClampVolume(AudioSettings.VoiceVolume);
    OnAudioSettingsChanged.Broadcast(AudioSettings);
}

bool UWMAudioReadinessSubsystem::IsCanonicalCueId(FName CueId)
{
    return CueId != NAME_None && CanonicalCueIds().Contains(CueId);
}
