#include "Environment/WMEnvironmentStateTypes.h"

#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
    bool IsUnitValue(const float Value)
    {
        return FMath::IsFinite(Value) && Value >= 0.0f && Value <= 1.0f;
    }

    bool ReadUnitField(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field, float& OutValue)
    {
        double Value = 0.0;
        if (!Object.IsValid() || !Object->TryGetNumberField(Field, Value) || !FMath::IsFinite(Value) || Value < 0.0 || Value > 1.0)
        {
            return false;
        }
        OutValue = static_cast<float>(Value);
        return true;
    }

    bool ReadDeltaField(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field, float& OutValue)
    {
        double Value = 0.0;
        if (!Object.IsValid() || !Object->TryGetNumberField(Field, Value) || !FMath::IsFinite(Value) || Value < -1.0 || Value > 1.0)
        {
            return false;
        }
        OutValue = static_cast<float>(Value);
        return true;
    }

    bool ReadVector(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field, FVector& OutVector)
    {
        const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
        if (!Object.IsValid() || !Object->TryGetArrayField(Field, Values) || !Values || Values->Num() != 3)
        {
            return false;
        }
        for (const TSharedPtr<FJsonValue>& Value : *Values)
        {
            if (!Value.IsValid() || Value->Type != EJson::Number)
            {
                return false;
            }
        }
        OutVector = FVector((*Values)[0]->AsNumber(), (*Values)[1]->AsNumber(), (*Values)[2]->AsNumber());
        return !OutVector.ContainsNaN();
    }
}

bool FWMEnvironmentStateSnapshot::IsBounded() const
{
    return !BiomeId.IsNone() && IsUnitValue(VegetationHealth) && IsUnitValue(WaterFlow) &&
        IsUnitValue(SoilProtection) && IsUnitValue(ShadeCoverage) && IsUnitValue(HabitatQuality) && !ReactionId.IsNone();
}

bool FWMEnvironmentActionDefinition::IsSane() const
{
    const bool bHasDelta = !FMath::IsNearlyZero(VegetationDelta) || !FMath::IsNearlyZero(WaterFlowDelta) ||
        !FMath::IsNearlyZero(SoilProtectionDelta) || !FMath::IsNearlyZero(ShadeCoverageDelta);
    return !ActionId.IsNone() && !TargetId.IsNone() && !PromptKey.IsNone() && !LocationCm.ContainsNaN() &&
        FMath::IsFinite(InteractionRadiusCm) && InteractionRadiusCm >= 100.0f && InteractionRadiusCm <= 1500.0f &&
        FMath::IsFinite(FocusRadiusCm) && FocusRadiusCm >= 20.0f && FocusRadiusCm <= 400.0f &&
        MaxApplications >= 1 && MaxApplications <= 5 &&
        FMath::IsFinite(VegetationDelta) && FMath::Abs(VegetationDelta) <= 1.0f &&
        FMath::IsFinite(WaterFlowDelta) && FMath::Abs(WaterFlowDelta) <= 1.0f &&
        FMath::IsFinite(SoilProtectionDelta) && FMath::Abs(SoilProtectionDelta) <= 1.0f &&
        FMath::IsFinite(ShadeCoverageDelta) && FMath::Abs(ShadeCoverageDelta) <= 1.0f && bHasDelta;
}

bool FWMEnvironmentReactionBand::IsSane() const
{
    return !ReactionId.IsNone() && IsUnitValue(MaxHabitatQuality);
}

float FWMEnvironmentStateDefinition::DeriveHabitatQuality(
    const float VegetationHealth,
    const float WaterFlow,
    const float SoilProtection,
    const float ShadeCoverage)
{
    return FMath::Clamp((VegetationHealth + WaterFlow + SoilProtection + ShadeCoverage) * 0.25f, 0.0f, 1.0f);
}

const FWMEnvironmentActionDefinition* FWMEnvironmentStateDefinition::FindAction(const FName ActionId) const
{
    return Actions.FindByPredicate([ActionId](const FWMEnvironmentActionDefinition& Action)
    {
        return Action.ActionId == ActionId;
    });
}

FName FWMEnvironmentStateDefinition::ResolveReactionId(const float HabitatQuality) const
{
    if (!IsUnitValue(HabitatQuality))
    {
        return NAME_None;
    }
    for (const FWMEnvironmentReactionBand& Band : ReactionBands)
    {
        if (HabitatQuality <= Band.MaxHabitatQuality + KINDA_SMALL_NUMBER)
        {
            return Band.ReactionId;
        }
    }
    return ReactionBands.IsEmpty() ? NAME_None : ReactionBands.Last().ReactionId;
}

bool FWMEnvironmentStateDefinition::IsSane() const
{
    if (SchemaVersion != 1 || BiomeId.IsNone() || !IsUnitValue(InitialVegetationHealth) || !IsUnitValue(InitialWaterFlow) ||
        !IsUnitValue(InitialSoilProtection) || !IsUnitValue(InitialShadeCoverage) || Actions.Num() < 3 || ReactionBands.Num() < 2)
    {
        return false;
    }

    TSet<FName> ActionIds;
    TSet<FName> TargetIds;
    TSet<FName> PromptKeys;
    for (const FWMEnvironmentActionDefinition& Action : Actions)
    {
        if (!Action.IsSane() || ActionIds.Contains(Action.ActionId) || TargetIds.Contains(Action.TargetId) || PromptKeys.Contains(Action.PromptKey))
        {
            return false;
        }
        ActionIds.Add(Action.ActionId);
        TargetIds.Add(Action.TargetId);
        PromptKeys.Add(Action.PromptKey);
    }

    float PreviousMax = -1.0f;
    TSet<FName> ReactionIds;
    for (const FWMEnvironmentReactionBand& Band : ReactionBands)
    {
        if (!Band.IsSane() || ReactionIds.Contains(Band.ReactionId) || Band.MaxHabitatQuality <= PreviousMax)
        {
            return false;
        }
        ReactionIds.Add(Band.ReactionId);
        PreviousMax = Band.MaxHabitatQuality;
    }
    return FMath::IsNearlyEqual(ReactionBands.Last().MaxHabitatQuality, 1.0f);
}

bool FWMEnvironmentStateDefinition::TryParseJson(
    const FString& Json,
    FWMEnvironmentStateDefinition& OutDefinition,
    FString& OutError)
{
    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        OutError = TEXT("Environment state JSON is not a valid object.");
        return false;
    }

    double SchemaVersionValue = 0.0;
    FString BiomeIdString;
    bool bPrototypeOnlyValue = true;
    if (!Root->TryGetNumberField(TEXT("schemaVersion"), SchemaVersionValue) ||
        !Root->TryGetStringField(TEXT("biomeId"), BiomeIdString) || BiomeIdString.IsEmpty() ||
        !Root->TryGetBoolField(TEXT("prototypeOnly"), bPrototypeOnlyValue) ||
        !Root->HasTypedField<EJson::Object>(TEXT("baseline")))
    {
        OutError = TEXT("Environment state header is invalid.");
        return false;
    }

    FWMEnvironmentStateDefinition Candidate;
    Candidate.SchemaVersion = static_cast<int32>(SchemaVersionValue);
    Candidate.BiomeId = FName(*BiomeIdString);
    Candidate.bPrototypeOnly = bPrototypeOnlyValue;

    const TSharedPtr<FJsonObject> Baseline = Root->GetObjectField(TEXT("baseline"));
    if (!ReadUnitField(Baseline, TEXT("vegetationHealth"), Candidate.InitialVegetationHealth) ||
        !ReadUnitField(Baseline, TEXT("waterFlow"), Candidate.InitialWaterFlow) ||
        !ReadUnitField(Baseline, TEXT("soilProtection"), Candidate.InitialSoilProtection) ||
        !ReadUnitField(Baseline, TEXT("shadeCoverage"), Candidate.InitialShadeCoverage))
    {
        OutError = TEXT("Environment baseline must contain four normalized primary dimensions.");
        return false;
    }

    const TArray<TSharedPtr<FJsonValue>>* ActionValues = nullptr;
    if (!Root->TryGetArrayField(TEXT("actions"), ActionValues) || !ActionValues)
    {
        OutError = TEXT("Environment actions are missing.");
        return false;
    }
    for (const TSharedPtr<FJsonValue>& ActionValue : *ActionValues)
    {
        const TSharedPtr<FJsonObject>* ActionObjectPtr = nullptr;
        if (!ActionValue.IsValid() || !ActionValue->TryGetObject(ActionObjectPtr) || !ActionObjectPtr || !ActionObjectPtr->IsValid())
        {
            OutError = TEXT("Environment action entry is invalid.");
            return false;
        }
        const TSharedPtr<FJsonObject>& ActionObject = *ActionObjectPtr;
        FWMEnvironmentActionDefinition Action;
        FString ActionIdString;
        FString TargetIdString;
        FString PromptKeyString;
        double InteractionRadiusValue = 0.0;
        double FocusRadiusValue = 0.0;
        double MaxApplicationsValue = 0.0;
        if (!ActionObject->TryGetStringField(TEXT("actionId"), ActionIdString) || ActionIdString.IsEmpty() ||
            !ActionObject->TryGetStringField(TEXT("targetId"), TargetIdString) || TargetIdString.IsEmpty() ||
            !ActionObject->TryGetStringField(TEXT("promptKey"), PromptKeyString) || PromptKeyString.IsEmpty() ||
            !ReadVector(ActionObject, TEXT("locationCm"), Action.LocationCm) ||
            !ActionObject->TryGetNumberField(TEXT("interactionRadiusCm"), InteractionRadiusValue) ||
            !ActionObject->TryGetNumberField(TEXT("focusRadiusCm"), FocusRadiusValue) ||
            !ActionObject->TryGetNumberField(TEXT("maxApplications"), MaxApplicationsValue) ||
            !ActionObject->HasTypedField<EJson::Object>(TEXT("delta")))
        {
            OutError = TEXT("Environment action fields are invalid.");
            return false;
        }

        const TSharedPtr<FJsonObject> Delta = ActionObject->GetObjectField(TEXT("delta"));
        if (!ReadDeltaField(Delta, TEXT("vegetationHealth"), Action.VegetationDelta) ||
            !ReadDeltaField(Delta, TEXT("waterFlow"), Action.WaterFlowDelta) ||
            !ReadDeltaField(Delta, TEXT("soilProtection"), Action.SoilProtectionDelta) ||
            !ReadDeltaField(Delta, TEXT("shadeCoverage"), Action.ShadeCoverageDelta))
        {
            OutError = TEXT("Environment action delta must contain four bounded dimensions.");
            return false;
        }

        Action.ActionId = FName(*ActionIdString);
        Action.TargetId = FName(*TargetIdString);
        Action.PromptKey = FName(*PromptKeyString);
        Action.InteractionRadiusCm = static_cast<float>(InteractionRadiusValue);
        Action.FocusRadiusCm = static_cast<float>(FocusRadiusValue);
        Action.MaxApplications = static_cast<int32>(MaxApplicationsValue);
        Candidate.Actions.Add(MoveTemp(Action));
    }

    const TArray<TSharedPtr<FJsonValue>>* ReactionValues = nullptr;
    if (!Root->TryGetArrayField(TEXT("reactionBands"), ReactionValues) || !ReactionValues)
    {
        OutError = TEXT("Environment reaction bands are missing.");
        return false;
    }
    for (const TSharedPtr<FJsonValue>& ReactionValue : *ReactionValues)
    {
        const TSharedPtr<FJsonObject>* ReactionObjectPtr = nullptr;
        if (!ReactionValue.IsValid() || !ReactionValue->TryGetObject(ReactionObjectPtr) || !ReactionObjectPtr || !ReactionObjectPtr->IsValid())
        {
            OutError = TEXT("Environment reaction band entry is invalid.");
            return false;
        }
        FString ReactionIdString;
        double MaxValue = 0.0;
        if (!(*ReactionObjectPtr)->TryGetStringField(TEXT("reactionId"), ReactionIdString) || ReactionIdString.IsEmpty() ||
            !(*ReactionObjectPtr)->TryGetNumberField(TEXT("maxHabitatQuality"), MaxValue))
        {
            OutError = TEXT("Environment reaction band fields are invalid.");
            return false;
        }
        FWMEnvironmentReactionBand Band;
        Band.ReactionId = FName(*ReactionIdString);
        Band.MaxHabitatQuality = static_cast<float>(MaxValue);
        Candidate.ReactionBands.Add(MoveTemp(Band));
    }

    if (!Candidate.IsSane())
    {
        OutError = TEXT("Environment state definition failed semantic validation.");
        return false;
    }

    OutDefinition = MoveTemp(Candidate);
    OutError.Reset();
    return true;
}

bool FWMEnvironmentStateModel::Initialize(const FWMEnvironmentStateDefinition& InDefinition)
{
    if (!InDefinition.IsSane())
    {
        return false;
    }

    Definition = InDefinition;
    Snapshot = FWMEnvironmentStateSnapshot();
    Snapshot.BiomeId = Definition.BiomeId;
    Snapshot.VegetationHealth = Definition.InitialVegetationHealth;
    Snapshot.WaterFlow = Definition.InitialWaterFlow;
    Snapshot.SoilProtection = Definition.InitialSoilProtection;
    Snapshot.ShadeCoverage = Definition.InitialShadeCoverage;
    ApplicationCounts.Reset();
    bInitialized = true;
    RefreshDerivedState();
    return Snapshot.IsBounded();
}

bool FWMEnvironmentStateModel::CanApplyAction(const FName ActionId) const
{
    if (!bInitialized)
    {
        return false;
    }
    const FWMEnvironmentActionDefinition* Action = Definition.FindAction(ActionId);
    return Action && ApplicationCounts.FindRef(ActionId) < Action->MaxApplications;
}

bool FWMEnvironmentStateModel::ApplyAction(const FName ActionId)
{
    if (!CanApplyAction(ActionId))
    {
        return false;
    }

    const FWMEnvironmentActionDefinition* Action = Definition.FindAction(ActionId);
    if (!Action)
    {
        return false;
    }

    Snapshot.VegetationHealth = FMath::Clamp(Snapshot.VegetationHealth + Action->VegetationDelta, 0.0f, 1.0f);
    Snapshot.WaterFlow = FMath::Clamp(Snapshot.WaterFlow + Action->WaterFlowDelta, 0.0f, 1.0f);
    Snapshot.SoilProtection = FMath::Clamp(Snapshot.SoilProtection + Action->SoilProtectionDelta, 0.0f, 1.0f);
    Snapshot.ShadeCoverage = FMath::Clamp(Snapshot.ShadeCoverage + Action->ShadeCoverageDelta, 0.0f, 1.0f);
    ApplicationCounts.FindOrAdd(ActionId) += 1;
    RefreshDerivedState();
    return Snapshot.IsBounded();
}

int32 FWMEnvironmentStateModel::GetAppliedCount(const FName ActionId) const
{
    return ApplicationCounts.FindRef(ActionId);
}

void FWMEnvironmentStateModel::RefreshDerivedState()
{
    Snapshot.HabitatQuality = FWMEnvironmentStateDefinition::DeriveHabitatQuality(
        Snapshot.VegetationHealth,
        Snapshot.WaterFlow,
        Snapshot.SoilProtection,
        Snapshot.ShadeCoverage);
    Snapshot.ReactionId = Definition.ResolveReactionId(Snapshot.HabitatQuality);
}
