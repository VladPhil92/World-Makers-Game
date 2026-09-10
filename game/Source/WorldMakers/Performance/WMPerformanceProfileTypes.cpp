#include "Performance/WMPerformanceProfileTypes.h"

#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
    bool ReadInt(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field, int32& OutValue)
    {
        double Value = 0.0;
        if (!Object.IsValid() || !Object->TryGetNumberField(Field, Value)) return false;
        OutValue = static_cast<int32>(Value);
        return FMath::IsNearlyEqual(Value, static_cast<double>(OutValue));
    }

    bool ReadFloat(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field, float& OutValue)
    {
        double Value = 0.0;
        if (!Object.IsValid() || !Object->TryGetNumberField(Field, Value) || !FMath::IsFinite(Value)) return false;
        OutValue = static_cast<float>(Value);
        return FMath::IsFinite(OutValue);
    }

    bool ReadName(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field, FName& OutName)
    {
        FString Value;
        if (!Object.IsValid() || !Object->TryGetStringField(Field, Value) || Value.IsEmpty()) return false;
        OutName = FName(*Value);
        return !OutName.IsNone();
    }

    float Percentile95(TArray<float> Values)
    {
        if (Values.IsEmpty()) return 0.0f;
        Values.Sort();
        const int32 Index = FMath::Clamp(FMath::CeilToInt(static_cast<float>(Values.Num()) * 0.95f) - 1, 0, Values.Num() - 1);
        return Values[Index];
    }
}

bool FWMPerformanceBudget::IsSane() const
{
    return TargetFps >= 30 && TargetFps <= 120 && FMath::IsFinite(FrameTimeBudgetMs) &&
        FrameTimeBudgetMs >= 8.0f && FrameTimeBudgetMs <= 40.0f &&
        MaxWorldActors >= 100 && MaxWorldActors <= 10000 &&
        MaxPlacedBuildPieces >= 50 && MaxPlacedBuildPieces <= 5000 &&
        MaxActiveInteractables >= 1 && MaxActiveInteractables <= 256;
}

bool FWMScalabilityProfile::IsSane() const
{
    const auto QualitySane = [](const int32 Value) { return Value >= 0 && Value <= 3; };
    return ScreenPercentage >= 50 && ScreenPercentage <= 100 &&
        TexturePoolMB >= 128 && TexturePoolMB <= 4096 &&
        QualitySane(ViewDistanceQuality) && QualitySane(AntiAliasingQuality) &&
        QualitySane(ShadowQuality) && QualitySane(PostProcessQuality) &&
        QualitySane(TextureQuality) && QualitySane(EffectsQuality) && QualitySane(FoliageQuality) &&
        FMath::IsFinite(FoliageDensityScale) && FoliageDensityScale >= 0.1f && FoliageDensityScale <= 1.0f &&
        FMath::IsFinite(GrassDensityScale) && GrassDensityScale >= 0.1f && GrassDensityScale <= 1.0f &&
        FMath::IsFinite(ShadowDistanceScale) && ShadowDistanceScale >= 0.1f && ShadowDistanceScale <= 1.0f;
}

TMap<FString, FString> FWMScalabilityProfile::BuildAllowlistedCVarAssignments() const
{
    TMap<FString, FString> Result;
    Result.Add(TEXT("sg.ViewDistanceQuality"), FString::FromInt(ViewDistanceQuality));
    Result.Add(TEXT("sg.AntiAliasingQuality"), FString::FromInt(AntiAliasingQuality));
    Result.Add(TEXT("sg.ShadowQuality"), FString::FromInt(ShadowQuality));
    Result.Add(TEXT("sg.PostProcessQuality"), FString::FromInt(PostProcessQuality));
    Result.Add(TEXT("sg.TextureQuality"), FString::FromInt(TextureQuality));
    Result.Add(TEXT("sg.EffectsQuality"), FString::FromInt(EffectsQuality));
    Result.Add(TEXT("sg.FoliageQuality"), FString::FromInt(FoliageQuality));
    Result.Add(TEXT("r.ScreenPercentage"), FString::FromInt(ScreenPercentage));
    Result.Add(TEXT("r.Streaming.PoolSize"), FString::FromInt(TexturePoolMB));
    Result.Add(TEXT("foliage.DensityScale"), FString::SanitizeFloat(FoliageDensityScale));
    Result.Add(TEXT("grass.DensityScale"), FString::SanitizeFloat(GrassDensityScale));
    Result.Add(TEXT("r.Shadow.DistanceScale"), FString::SanitizeFloat(ShadowDistanceScale));
    return Result;
}

bool FWMPerformanceProfileDefinition::IsSane() const
{
    if (ProfileId.IsNone() || DeviceClass.IsNone() || Tier.IsNone() || !Budget.IsSane() || !Scalability.IsSane()) return false;
    const FString Id = ProfileId.ToString();
    const FString Device = DeviceClass.ToString();
    return Id.StartsWith(TEXT("performance.")) && (Device == TEXT("tablet") || Device == TEXT("desktop"));
}

const FWMPerformanceProfileDefinition* FWMPerformanceProfileCatalog::FindProfile(const FName ProfileId) const
{
    return Profiles.FindByPredicate([ProfileId](const FWMPerformanceProfileDefinition& Profile)
    {
        return Profile.ProfileId == ProfileId;
    });
}

bool FWMPerformanceProfileCatalog::IsSane() const
{
    if (SchemaVersion != 1 || Profiles.Num() < 4) return false;
    TSet<FName> SeenIds;
    int32 TabletCount = 0;
    for (const FWMPerformanceProfileDefinition& Profile : Profiles)
    {
        if (!Profile.IsSane() || SeenIds.Contains(Profile.ProfileId)) return false;
        SeenIds.Add(Profile.ProfileId);
        if (Profile.DeviceClass == FName(TEXT("tablet"))) ++TabletCount;
    }
    return TabletCount >= 3;
}

bool FWMPerformanceProfileCatalog::TryParseJson(const FString& Json, FWMPerformanceProfileCatalog& OutCatalog, FString& OutError)
{
    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        OutError = TEXT("Performance profile JSON is not a valid object.");
        return false;
    }

    int32 SchemaVersionValue = 0;
    const TArray<TSharedPtr<FJsonValue>>* ProfileValues = nullptr;
    if (!ReadInt(Root, TEXT("schemaVersion"), SchemaVersionValue) ||
        !Root->TryGetArrayField(TEXT("profiles"), ProfileValues) || !ProfileValues)
    {
        OutError = TEXT("Performance profile catalog header is invalid.");
        return false;
    }

    FWMPerformanceProfileCatalog Candidate;
    Candidate.SchemaVersion = SchemaVersionValue;
    for (const TSharedPtr<FJsonValue>& Value : *ProfileValues)
    {
        const TSharedPtr<FJsonObject>* ObjectPtr = nullptr;
        if (!Value.IsValid() || !Value->TryGetObject(ObjectPtr) || !ObjectPtr || !ObjectPtr->IsValid())
        {
            OutError = TEXT("Performance profile entry is invalid.");
            return false;
        }

        const TSharedPtr<FJsonObject>& Object = *ObjectPtr;
        FWMPerformanceProfileDefinition Profile;
        if (!ReadName(Object, TEXT("id"), Profile.ProfileId) ||
            !ReadName(Object, TEXT("deviceClass"), Profile.DeviceClass) ||
            !ReadName(Object, TEXT("tier"), Profile.Tier) ||
            !ReadInt(Object, TEXT("targetFps"), Profile.Budget.TargetFps) ||
            !ReadFloat(Object, TEXT("frameTimeBudgetMs"), Profile.Budget.FrameTimeBudgetMs) ||
            !ReadInt(Object, TEXT("maxWorldActors"), Profile.Budget.MaxWorldActors) ||
            !ReadInt(Object, TEXT("maxPlacedBuildPieces"), Profile.Budget.MaxPlacedBuildPieces) ||
            !ReadInt(Object, TEXT("maxActiveInteractables"), Profile.Budget.MaxActiveInteractables) ||
            !ReadInt(Object, TEXT("screenPercentage"), Profile.Scalability.ScreenPercentage) ||
            !ReadInt(Object, TEXT("texturePoolMB"), Profile.Scalability.TexturePoolMB) ||
            !ReadInt(Object, TEXT("viewDistanceQuality"), Profile.Scalability.ViewDistanceQuality) ||
            !ReadInt(Object, TEXT("antiAliasingQuality"), Profile.Scalability.AntiAliasingQuality) ||
            !ReadInt(Object, TEXT("shadowQuality"), Profile.Scalability.ShadowQuality) ||
            !ReadInt(Object, TEXT("postProcessQuality"), Profile.Scalability.PostProcessQuality) ||
            !ReadInt(Object, TEXT("textureQuality"), Profile.Scalability.TextureQuality) ||
            !ReadInt(Object, TEXT("effectsQuality"), Profile.Scalability.EffectsQuality) ||
            !ReadInt(Object, TEXT("foliageQuality"), Profile.Scalability.FoliageQuality) ||
            !ReadFloat(Object, TEXT("foliageDensityScale"), Profile.Scalability.FoliageDensityScale) ||
            !ReadFloat(Object, TEXT("grassDensityScale"), Profile.Scalability.GrassDensityScale) ||
            !ReadFloat(Object, TEXT("shadowDistanceScale"), Profile.Scalability.ShadowDistanceScale))
        {
            OutError = TEXT("Performance profile fields are invalid.");
            return false;
        }
        Candidate.Profiles.Add(MoveTemp(Profile));
    }

    if (!Candidate.IsSane())
    {
        OutError = TEXT("Performance profile catalog failed semantic validation.");
        return false;
    }

    OutCatalog = MoveTemp(Candidate);
    OutError.Reset();
    return true;
}

FString FWMPerformanceCaptureSummary::ToJson() const
{
    TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
    Root->SetNumberField(TEXT("schemaVersion"), 1);
    Root->SetStringField(TEXT("profileId"), ProfileId.ToString());
    Root->SetNumberField(TEXT("frameSampleCount"), FrameSampleCount);
    Root->SetNumberField(TEXT("averageFrameTimeMs"), AverageFrameTimeMs);
    Root->SetNumberField(TEXT("p95FrameTimeMs"), P95FrameTimeMs);
    Root->SetNumberField(TEXT("worstFrameTimeMs"), WorstFrameTimeMs);
    Root->SetNumberField(TEXT("maxWorldActors"), MaxWorldActors);
    Root->SetNumberField(TEXT("maxPlacedBuildPieces"), MaxPlacedBuildPieces);
    Root->SetNumberField(TEXT("maxActiveInteractables"), MaxActiveInteractables);
    Root->SetBoolField(TEXT("frameTimeWithinBudget"), bFrameTimeWithinBudget);
    Root->SetBoolField(TEXT("actorCountWithinBudget"), bActorCountWithinBudget);
    Root->SetBoolField(TEXT("buildPieceCountWithinBudget"), bBuildPieceCountWithinBudget);
    Root->SetBoolField(TEXT("interactableCountWithinBudget"), bInteractableCountWithinBudget);
    Root->SetBoolField(TEXT("withinBudget"), bWithinBudget);

    FString Output;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
    FJsonSerializer::Serialize(Root, Writer);
    return Output;
}

void FWMPerformanceCaptureAccumulator::Reset()
{
    FrameTimesMs.Reset();
    MaxWorldActors = 0;
    MaxPlacedBuildPieces = 0;
    MaxActiveInteractables = 0;
}

void FWMPerformanceCaptureAccumulator::AddFrameTimeMs(const float FrameTimeMs)
{
    if (FMath::IsFinite(FrameTimeMs) && FrameTimeMs > 0.0f && FrameTimesMs.Num() < 18000)
    {
        FrameTimesMs.Add(FrameTimeMs);
    }
}

void FWMPerformanceCaptureAccumulator::ObserveStructuralCounts(
    const int32 WorldActorCount,
    const int32 PlacedBuildPieceCount,
    const int32 ActiveInteractableCount)
{
    MaxWorldActors = FMath::Max(MaxWorldActors, FMath::Max(0, WorldActorCount));
    MaxPlacedBuildPieces = FMath::Max(MaxPlacedBuildPieces, FMath::Max(0, PlacedBuildPieceCount));
    MaxActiveInteractables = FMath::Max(MaxActiveInteractables, FMath::Max(0, ActiveInteractableCount));
}

FWMPerformanceCaptureSummary FWMPerformanceCaptureAccumulator::BuildSummary(
    const FName ProfileId,
    const FWMPerformanceBudget& Budget) const
{
    FWMPerformanceCaptureSummary Summary;
    Summary.ProfileId = ProfileId;
    Summary.FrameSampleCount = FrameTimesMs.Num();
    Summary.MaxWorldActors = MaxWorldActors;
    Summary.MaxPlacedBuildPieces = MaxPlacedBuildPieces;
    Summary.MaxActiveInteractables = MaxActiveInteractables;

    if (!FrameTimesMs.IsEmpty())
    {
        double Sum = 0.0;
        float Worst = 0.0f;
        for (const float Value : FrameTimesMs)
        {
            Sum += Value;
            Worst = FMath::Max(Worst, Value);
        }
        Summary.AverageFrameTimeMs = static_cast<float>(Sum / FrameTimesMs.Num());
        Summary.P95FrameTimeMs = Percentile95(FrameTimesMs);
        Summary.WorstFrameTimeMs = Worst;
    }

    Summary.bFrameTimeWithinBudget = Summary.HasSamples() && Summary.P95FrameTimeMs <= Budget.FrameTimeBudgetMs;
    Summary.bActorCountWithinBudget = MaxWorldActors <= Budget.MaxWorldActors;
    Summary.bBuildPieceCountWithinBudget = MaxPlacedBuildPieces <= Budget.MaxPlacedBuildPieces;
    Summary.bInteractableCountWithinBudget = MaxActiveInteractables <= Budget.MaxActiveInteractables;
    Summary.bWithinBudget = Summary.bFrameTimeWithinBudget && Summary.bActorCountWithinBudget &&
        Summary.bBuildPieceCountWithinBudget && Summary.bInteractableCountWithinBudget;
    return Summary;
}
