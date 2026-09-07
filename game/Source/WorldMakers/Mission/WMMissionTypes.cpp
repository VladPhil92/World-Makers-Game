#include "Mission/WMMissionTypes.h"

#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
    bool ReadNameArray(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field, TArray<FName>& OutValues)
    {
        const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
        if (!Object.IsValid() || !Object->TryGetArrayField(Field, Values) || !Values)
        {
            return false;
        }

        OutValues.Reset();
        for (const TSharedPtr<FJsonValue>& Value : *Values)
        {
            if (!Value.IsValid() || Value->Type != EJson::String)
            {
                return false;
            }
            OutValues.Add(FName(*Value->AsString()));
        }
        return !OutValues.IsEmpty();
    }
}

bool FWMMissionRuntimeDefinition::IsSane() const
{
    return !MissionId.IsNone() &&
        LearningObjectiveIds.Num() >= 2 &&
        RequiredEvidenceEventIds.Num() >= 2 &&
        TargetSpanCm >= 50.0f &&
        ToleranceCm >= 0.0f &&
        ToleranceCm < TargetSpanCm &&
        !RewardIds.IsEmpty();
}

bool FWMMissionRuntimeDefinition::TryParseJson(const FString& Json, FWMMissionRuntimeDefinition& OutDefinition, FString& OutError)
{
    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        OutError = TEXT("Mission JSON is not a valid object.");
        return false;
    }

    FString MissionIdString;
    if (!Root->TryGetStringField(TEXT("id"), MissionIdString) || MissionIdString.IsEmpty())
    {
        OutError = TEXT("Mission id is missing.");
        return false;
    }

    if (!Root->HasTypedField<EJson::Object>(TEXT("runtime")))
    {
        OutError = TEXT("Mission runtime block is missing.");
        return false;
    }

    const TSharedPtr<FJsonObject> Runtime = Root->GetObjectField(TEXT("runtime"));
    FString Evaluator;
    double TargetSpan = 0.0;
    double Tolerance = 0.0;
    bool bPrototypeOnly = true;
    if (!Runtime.IsValid() || !Runtime->TryGetStringField(TEXT("evaluator"), Evaluator) || Evaluator != TEXT("measure-and-build") ||
        !Runtime->TryGetNumberField(TEXT("targetSpanCm"), TargetSpan) ||
        !Runtime->TryGetNumberField(TEXT("toleranceCm"), Tolerance) ||
        !Runtime->TryGetBoolField(TEXT("prototypeOnly"), bPrototypeOnly))
    {
        OutError = TEXT("Mission runtime block is invalid.");
        return false;
    }

    FWMMissionRuntimeDefinition Candidate;
    Candidate.MissionId = FName(*MissionIdString);
    Candidate.TargetSpanCm = static_cast<float>(TargetSpan);
    Candidate.ToleranceCm = static_cast<float>(Tolerance);
    Candidate.bPrototypeOnly = bPrototypeOnly;

    if (!ReadNameArray(Root, TEXT("learningObjectives"), Candidate.LearningObjectiveIds) ||
        !ReadNameArray(Root, TEXT("evidenceEvents"), Candidate.RequiredEvidenceEventIds) ||
        !ReadNameArray(Runtime, TEXT("rewardIds"), Candidate.RewardIds) ||
        !Candidate.IsSane())
    {
        OutError = TEXT("Mission runtime definition failed semantic validation.");
        return false;
    }

    OutDefinition = MoveTemp(Candidate);
    OutError.Reset();
    return true;
}

bool FWMMissionProgressModel::Begin(const FWMMissionRuntimeDefinition& InDefinition)
{
    if (!InDefinition.IsSane())
    {
        return false;
    }

    Definition = InDefinition;
    State = EWMMissionRuntimeState::Active;
    bMeasurementEvidence = false;
    bStructureFitEvidence = false;
    LastMeasuredSpanCm = 0.0f;
    LastObservedStructureSpanCm = 0.0f;
    NextEvidenceSequence = 1;
    Evidence.Reset();
    EarnedRewardIds.Reset();
    return true;
}

bool FWMMissionProgressModel::RecordMeasurement(const float MeasuredSpanCm)
{
    if (State != EWMMissionRuntimeState::Active || bMeasurementEvidence || !FMath::IsFinite(MeasuredSpanCm) || MeasuredSpanCm <= 0.0f)
    {
        return false;
    }

    bMeasurementEvidence = true;
    LastMeasuredSpanCm = MeasuredSpanCm;

    FWMLearningEvidenceRecord Record;
    Record.MissionId = Definition.MissionId;
    Record.EventId = TEXT("measurement_used_before_build");
    Record.ObjectiveId = TEXT("math.measure.compare-lengths");
    Record.NumericValue = MeasuredSpanCm;
    Record.Sequence = NextEvidenceSequence++;
    Evidence.Add(Record);
    return true;
}

bool FWMMissionProgressModel::RecordStructureSpan(const float StructureSpanCm)
{
    if (State != EWMMissionRuntimeState::Active || !bMeasurementEvidence || !FMath::IsFinite(StructureSpanCm) || StructureSpanCm <= 0.0f)
    {
        return false;
    }

    LastObservedStructureSpanCm = StructureSpanCm;
    if (bStructureFitEvidence || FMath::Abs(StructureSpanCm - Definition.TargetSpanCm) > Definition.ToleranceCm)
    {
        return false;
    }

    bStructureFitEvidence = true;

    FWMLearningEvidenceRecord Record;
    Record.MissionId = Definition.MissionId;
    Record.EventId = TEXT("structure_fits_target_span");
    Record.ObjectiveId = TEXT("math.spatial.plan-to-constraint");
    Record.NumericValue = StructureSpanCm;
    Record.Sequence = NextEvidenceSequence++;
    Evidence.Add(Record);

    State = EWMMissionRuntimeState::Completed;
    EarnedRewardIds = Definition.RewardIds;
    return true;
}

float FWMMissionProgressModel::GetProgressFraction() const
{
    if (State == EWMMissionRuntimeState::Completed) return 1.0f;
    if (bMeasurementEvidence) return 0.5f;
    return 0.0f;
}
