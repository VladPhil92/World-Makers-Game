#include "Mission/WMMissionTypes.h"

#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
    const FName MeasureAndBuildEvaluator(TEXT("measure-and-build"));
    const FName ObserveEcosystemEvaluator(TEXT("observe-ecosystem"));

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
            if (!Value.IsValid() || Value->Type != EJson::String || Value->AsString().IsEmpty())
            {
                return false;
            }
            OutValues.Add(FName(*Value->AsString()));
        }
        return !OutValues.IsEmpty();
    }

    bool ReadOptionalNameArray(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field, TArray<FName>& OutValues)
    {
        OutValues.Reset();
        if (!Object.IsValid() || !Object->HasField(Field))
        {
            return true;
        }

        const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
        if (!Object->TryGetArrayField(Field, Values) || !Values)
        {
            return false;
        }

        for (const TSharedPtr<FJsonValue>& Value : *Values)
        {
            if (!Value.IsValid() || Value->Type != EJson::String || Value->AsString().IsEmpty())
            {
                return false;
            }
            OutValues.Add(FName(*Value->AsString()));
        }
        return true;
    }

    bool ReadObservationRequirements(
        const TSharedPtr<FJsonObject>& Runtime,
        TArray<FWMObservationEvidenceRequirement>& OutRequirements)
    {
        OutRequirements.Reset();
        const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
        if (!Runtime.IsValid() || !Runtime->TryGetArrayField(TEXT("observationRequirements"), Values) || !Values)
        {
            return false;
        }

        for (const TSharedPtr<FJsonValue>& Value : *Values)
        {
            const TSharedPtr<FJsonObject>* ObjectPtr = nullptr;
            if (!Value.IsValid() || !Value->TryGetObject(ObjectPtr) || !ObjectPtr || !ObjectPtr->IsValid())
            {
                return false;
            }

            FString ObservationIdString;
            FString EvidenceEventIdString;
            FString ObjectiveIdString;
            if (!(*ObjectPtr)->TryGetStringField(TEXT("observationId"), ObservationIdString) || ObservationIdString.IsEmpty() ||
                !(*ObjectPtr)->TryGetStringField(TEXT("evidenceEventId"), EvidenceEventIdString) || EvidenceEventIdString.IsEmpty() ||
                !(*ObjectPtr)->TryGetStringField(TEXT("objectiveId"), ObjectiveIdString) || ObjectiveIdString.IsEmpty())
            {
                return false;
            }

            FWMObservationEvidenceRequirement Requirement;
            Requirement.ObservationId = FName(*ObservationIdString);
            Requirement.EvidenceEventId = FName(*EvidenceEventIdString);
            Requirement.ObjectiveId = FName(*ObjectiveIdString);
            if (!Requirement.IsSane())
            {
                return false;
            }
            OutRequirements.Add(MoveTemp(Requirement));
        }
        return !OutRequirements.IsEmpty();
    }

    void SortNames(TArray<FName>& Values)
    {
        Values.Sort([](const FName& A, const FName& B)
        {
            return A.ToString() < B.ToString();
        });
    }
}

bool FWMObservationEvidenceRequirement::IsSane() const
{
    return !ObservationId.IsNone() && !EvidenceEventId.IsNone() && !ObjectiveId.IsNone();
}

bool FWMMissionRuntimeDefinition::IsMeasureAndBuild() const
{
    return Evaluator == MeasureAndBuildEvaluator;
}

bool FWMMissionRuntimeDefinition::IsObserveEcosystem() const
{
    return Evaluator == ObserveEcosystemEvaluator;
}

const FWMObservationEvidenceRequirement* FWMMissionRuntimeDefinition::FindObservationRequirement(const FName ObservationId) const
{
    return ObservationRequirements.FindByPredicate([ObservationId](const FWMObservationEvidenceRequirement& Requirement)
    {
        return Requirement.ObservationId == ObservationId;
    });
}

bool FWMMissionRuntimeDefinition::IsSane() const
{
    if (MissionId.IsNone() || Evaluator.IsNone() || LearningObjectiveIds.IsEmpty() || RequiredEvidenceEventIds.IsEmpty() || RewardIds.IsEmpty())
    {
        return false;
    }

    TSet<FName> UniquePrerequisites;
    for (const FName PrerequisiteId : PrerequisiteMissionIds)
    {
        if (PrerequisiteId.IsNone() || PrerequisiteId == MissionId || UniquePrerequisites.Contains(PrerequisiteId))
        {
            return false;
        }
        UniquePrerequisites.Add(PrerequisiteId);
    }

    if (IsMeasureAndBuild())
    {
        return TargetSpanCm >= 50.0f && ToleranceCm >= 0.0f && ToleranceCm < TargetSpanCm && ObservationRequirements.IsEmpty();
    }

    if (!IsObserveEcosystem() || TargetSpanCm != 0.0f || ToleranceCm != 0.0f || ObservationRequirements.Num() < 2)
    {
        return false;
    }

    TSet<FName> ObservationIds;
    TSet<FName> EvidenceEventIds;
    for (const FWMObservationEvidenceRequirement& Requirement : ObservationRequirements)
    {
        if (!Requirement.IsSane() || ObservationIds.Contains(Requirement.ObservationId) || EvidenceEventIds.Contains(Requirement.EvidenceEventId) ||
            !RequiredEvidenceEventIds.Contains(Requirement.EvidenceEventId) || !LearningObjectiveIds.Contains(Requirement.ObjectiveId))
        {
            return false;
        }
        ObservationIds.Add(Requirement.ObservationId);
        EvidenceEventIds.Add(Requirement.EvidenceEventId);
    }
    return EvidenceEventIds.Num() == RequiredEvidenceEventIds.Num();
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
    FString EvaluatorString;
    bool bPrototypeOnly = true;
    if (!Runtime.IsValid() || !Runtime->TryGetStringField(TEXT("evaluator"), EvaluatorString) || EvaluatorString.IsEmpty() ||
        !Runtime->TryGetBoolField(TEXT("prototypeOnly"), bPrototypeOnly))
    {
        OutError = TEXT("Mission runtime header is invalid.");
        return false;
    }

    FWMMissionRuntimeDefinition Candidate;
    Candidate.MissionId = FName(*MissionIdString);
    Candidate.Evaluator = FName(*EvaluatorString);
    Candidate.bPrototypeOnly = bPrototypeOnly;

    if (!ReadNameArray(Root, TEXT("learningObjectives"), Candidate.LearningObjectiveIds) ||
        !ReadNameArray(Root, TEXT("evidenceEvents"), Candidate.RequiredEvidenceEventIds) ||
        !ReadNameArray(Runtime, TEXT("rewardIds"), Candidate.RewardIds) ||
        !ReadOptionalNameArray(Runtime, TEXT("prerequisiteMissionIds"), Candidate.PrerequisiteMissionIds))
    {
        OutError = TEXT("Mission runtime stable ID arrays are invalid.");
        return false;
    }

    if (Candidate.IsMeasureAndBuild())
    {
        double TargetSpan = 0.0;
        double Tolerance = 0.0;
        if (!Runtime->TryGetNumberField(TEXT("targetSpanCm"), TargetSpan) ||
            !Runtime->TryGetNumberField(TEXT("toleranceCm"), Tolerance) || Runtime->HasField(TEXT("observationRequirements")))
        {
            OutError = TEXT("measure-and-build evaluator fields are invalid.");
            return false;
        }
        Candidate.TargetSpanCm = static_cast<float>(TargetSpan);
        Candidate.ToleranceCm = static_cast<float>(Tolerance);
    }
    else if (Candidate.IsObserveEcosystem())
    {
        if (Runtime->HasField(TEXT("targetSpanCm")) || Runtime->HasField(TEXT("toleranceCm")) ||
            !ReadObservationRequirements(Runtime, Candidate.ObservationRequirements))
        {
            OutError = TEXT("observe-ecosystem evaluator fields are invalid.");
            return false;
        }
    }
    else
    {
        OutError = TEXT("Mission evaluator is unsupported.");
        return false;
    }

    if (!Candidate.IsSane())
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
    RecordedObservationIds.Reset();
    return true;
}

bool FWMMissionProgressModel::RecordMeasurement(const float MeasuredSpanCm)
{
    if (State != EWMMissionRuntimeState::Active || !Definition.IsMeasureAndBuild() || bMeasurementEvidence ||
        !FMath::IsFinite(MeasuredSpanCm) || MeasuredSpanCm <= 0.0f)
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
    if (State != EWMMissionRuntimeState::Active || !Definition.IsMeasureAndBuild() || !bMeasurementEvidence ||
        !FMath::IsFinite(StructureSpanCm) || StructureSpanCm <= 0.0f)
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

bool FWMMissionProgressModel::RecordObservation(const FName ObservationId)
{
    if (State != EWMMissionRuntimeState::Active || !Definition.IsObserveEcosystem() || ObservationId.IsNone() ||
        RecordedObservationIds.Contains(ObservationId))
    {
        return false;
    }

    const FWMObservationEvidenceRequirement* Requirement = Definition.FindObservationRequirement(ObservationId);
    if (!Requirement)
    {
        return false;
    }

    RecordedObservationIds.Add(ObservationId);

    FWMLearningEvidenceRecord Record;
    Record.MissionId = Definition.MissionId;
    Record.EventId = Requirement->EvidenceEventId;
    Record.ObjectiveId = Requirement->ObjectiveId;
    Record.NumericValue = 1.0f;
    Record.Sequence = NextEvidenceSequence++;
    Evidence.Add(Record);

    if (RecordedObservationIds.Num() == Definition.ObservationRequirements.Num())
    {
        State = EWMMissionRuntimeState::Completed;
        EarnedRewardIds = Definition.RewardIds;
    }
    return true;
}

float FWMMissionProgressModel::GetProgressFraction() const
{
    if (State == EWMMissionRuntimeState::Completed) return 1.0f;
    if (Definition.IsObserveEcosystem())
    {
        return Definition.ObservationRequirements.IsEmpty()
            ? 0.0f
            : static_cast<float>(RecordedObservationIds.Num()) / static_cast<float>(Definition.ObservationRequirements.Num());
    }
    if (bMeasurementEvidence) return 0.5f;
    return 0.0f;
}

bool FWMMissionJourneyModel::ArePrerequisitesSatisfied(const FWMMissionRuntimeDefinition& Definition) const
{
    for (const FName PrerequisiteId : Definition.PrerequisiteMissionIds)
    {
        if (!CompletedMissionIds.Contains(PrerequisiteId))
        {
            return false;
        }
    }
    return true;
}

bool FWMMissionJourneyModel::CanActivate(const FWMMissionRuntimeDefinition& Definition) const
{
    return CompletedMissionIds.Contains(Definition.MissionId) || ArePrerequisitesSatisfied(Definition);
}

EWMJourneyMissionState FWMMissionJourneyModel::ResolveState(
    const FWMMissionRuntimeDefinition& Definition,
    const FName ActiveMissionId,
    const EWMMissionRuntimeState ActiveRuntimeState) const
{
    if (ActiveMissionId == Definition.MissionId && ActiveRuntimeState == EWMMissionRuntimeState::Active)
    {
        return EWMJourneyMissionState::Active;
    }
    if (CompletedMissionIds.Contains(Definition.MissionId))
    {
        return EWMJourneyMissionState::Completed;
    }
    return ArePrerequisitesSatisfied(Definition) ? EWMJourneyMissionState::Available : EWMJourneyMissionState::Locked;
}

bool FWMMissionJourneyModel::ApplyCompletion(const FWMMissionRuntimeDefinition& Definition, TArray<FName>& OutNewRewardIds)
{
    OutNewRewardIds.Reset();
    const bool bNewCompletion = !CompletedMissionIds.Contains(Definition.MissionId);
    CompletedMissionIds.Add(Definition.MissionId);

    for (const FName RewardId : Definition.RewardIds)
    {
        if (!GrantedRewardIds.Contains(RewardId))
        {
            GrantedRewardIds.Add(RewardId);
            OutNewRewardIds.Add(RewardId);
        }
    }
    SortNames(OutNewRewardIds);
    return bNewCompletion;
}

void FWMMissionJourneyModel::Restore(const TArray<FName>& InCompletedMissionIds, const TArray<FName>& InGrantedRewardIds)
{
    CompletedMissionIds.Reset();
    GrantedRewardIds.Reset();
    for (const FName MissionId : InCompletedMissionIds)
    {
        if (!MissionId.IsNone()) CompletedMissionIds.Add(MissionId);
    }
    for (const FName RewardId : InGrantedRewardIds)
    {
        if (!RewardId.IsNone()) GrantedRewardIds.Add(RewardId);
    }
}

void FWMMissionJourneyModel::Export(TArray<FName>& OutCompletedMissionIds, TArray<FName>& OutGrantedRewardIds) const
{
    OutCompletedMissionIds.Reset();
    OutGrantedRewardIds.Reset();
    OutCompletedMissionIds.Reserve(CompletedMissionIds.Num());
    OutGrantedRewardIds.Reserve(GrantedRewardIds.Num());
    for (const FName MissionId : CompletedMissionIds) OutCompletedMissionIds.Add(MissionId);
    for (const FName RewardId : GrantedRewardIds) OutGrantedRewardIds.Add(RewardId);
    SortNames(OutCompletedMissionIds);
    SortNames(OutGrantedRewardIds);
}
