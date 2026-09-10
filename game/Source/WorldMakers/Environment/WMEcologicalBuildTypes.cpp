#include "Environment/WMEcologicalBuildTypes.h"

#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
    bool ReadVector(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field, FVector& OutVector)
    {
        const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
        if (!Object.IsValid() || !Object->TryGetArrayField(Field, Values) || !Values || Values->Num() != 3)
        {
            return false;
        }
        for (const TSharedPtr<FJsonValue>& Value : *Values)
        {
            if (!Value.IsValid() || Value->Type != EJson::Number) return false;
        }
        OutVector = FVector((*Values)[0]->AsNumber(), (*Values)[1]->AsNumber(), (*Values)[2]->AsNumber());
        return !OutVector.ContainsNaN();
    }

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
            if (!Value.IsValid() || Value->Type != EJson::String || Value->AsString().IsEmpty()) return false;
            OutValues.Add(FName(*Value->AsString()));
        }
        return true;
    }

    bool ReadDelta(const TSharedPtr<FJsonObject>& Object, FWMEnvironmentStateDelta& OutDelta)
    {
        if (!Object.IsValid()) return false;
        double Vegetation = 0.0;
        double Water = 0.0;
        double Soil = 0.0;
        double Shade = 0.0;
        if (!Object->TryGetNumberField(TEXT("vegetationHealth"), Vegetation) ||
            !Object->TryGetNumberField(TEXT("waterFlow"), Water) ||
            !Object->TryGetNumberField(TEXT("soilProtection"), Soil) ||
            !Object->TryGetNumberField(TEXT("shadeCoverage"), Shade))
        {
            return false;
        }
        OutDelta.VegetationHealth = static_cast<float>(Vegetation);
        OutDelta.WaterFlow = static_cast<float>(Water);
        OutDelta.SoilProtection = static_cast<float>(Soil);
        OutDelta.ShadeCoverage = static_cast<float>(Shade);
        return OutDelta.IsSane();
    }
}

bool FWMEcologicalPieceRequirement::IsSane() const
{
    return !PieceId.IsNone() && MinCount >= 1 && MinCount <= 20;
}

bool FWMEcologicalBuildInterventionDefinition::IsSane() const
{
    if (InterventionId.IsNone() || AnchorCm.ContainsNaN() || !FMath::IsFinite(RadiusCm) ||
        RadiusCm < 100.0f || RadiusCm > 1500.0f || Requirements.IsEmpty() || !EffectDelta.IsSane() || RewardId.IsNone())
    {
        return false;
    }

    TSet<FName> RequirementPieceIds;
    for (const FWMEcologicalPieceRequirement& Requirement : Requirements)
    {
        if (!Requirement.IsSane() || RequirementPieceIds.Contains(Requirement.PieceId)) return false;
        RequirementPieceIds.Add(Requirement.PieceId);
    }

    TSet<FName> Prerequisites;
    for (const FName PrerequisiteId : PrerequisiteInterventionIds)
    {
        if (PrerequisiteId.IsNone() || PrerequisiteId == InterventionId || Prerequisites.Contains(PrerequisiteId)) return false;
        Prerequisites.Add(PrerequisiteId);
    }
    return RewardId.ToString().StartsWith(TEXT("reward."));
}

int32 FWMEcologicalBuildInterventionDefinition::CountSatisfiedRequirements(
    const TArray<FWMPlacedBuildPieceSnapshot>& Pieces) const
{
    if (!IsSane()) return 0;

    int32 Satisfied = 0;
    const float RadiusSquared = FMath::Square(RadiusCm);
    for (const FWMEcologicalPieceRequirement& Requirement : Requirements)
    {
        int32 MatchingCount = 0;
        for (const FWMPlacedBuildPieceSnapshot& Piece : Pieces)
        {
            if (!Piece.IsSane() || Piece.PieceId != Requirement.PieceId) continue;
            const FVector Delta = Piece.LocationCm - AnchorCm;
            const float HorizontalDistanceSquared = FMath::Square(Delta.X) + FMath::Square(Delta.Y);
            if (HorizontalDistanceSquared <= RadiusSquared)
            {
                ++MatchingCount;
            }
        }
        if (MatchingCount >= Requirement.MinCount)
        {
            ++Satisfied;
        }
    }
    return Satisfied;
}

bool FWMEcologicalBuildInterventionDefinition::IsSatisfiedBy(
    const TArray<FWMPlacedBuildPieceSnapshot>& Pieces) const
{
    return IsSane() && CountSatisfiedRequirements(Pieces) == Requirements.Num();
}

const FWMEcologicalBuildInterventionDefinition* FWMEcologicalBuildDefinition::FindIntervention(const FName InterventionId) const
{
    return Interventions.FindByPredicate([InterventionId](const FWMEcologicalBuildInterventionDefinition& Intervention)
    {
        return Intervention.InterventionId == InterventionId;
    });
}

bool FWMEcologicalBuildDefinition::IsSane() const
{
    if (SchemaVersion != 1 || BiomeId.IsNone() || Interventions.IsEmpty()) return false;

    TSet<FName> SeenIds;
    TSet<FName> RewardIds;
    for (const FWMEcologicalBuildInterventionDefinition& Intervention : Interventions)
    {
        if (!Intervention.IsSane() || SeenIds.Contains(Intervention.InterventionId) || RewardIds.Contains(Intervention.RewardId))
        {
            return false;
        }
        for (const FName PrerequisiteId : Intervention.PrerequisiteInterventionIds)
        {
            if (!SeenIds.Contains(PrerequisiteId)) return false;
        }
        SeenIds.Add(Intervention.InterventionId);
        RewardIds.Add(Intervention.RewardId);
    }
    return true;
}

bool FWMEcologicalBuildDefinition::TryParseJson(
    const FString& Json,
    FWMEcologicalBuildDefinition& OutDefinition,
    FString& OutError)
{
    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Json);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        OutError = TEXT("Ecological build JSON is not a valid object.");
        return false;
    }

    double SchemaVersionValue = 0.0;
    FString BiomeIdString;
    bool bPrototypeOnlyValue = true;
    const TArray<TSharedPtr<FJsonValue>>* InterventionValues = nullptr;
    if (!Root->TryGetNumberField(TEXT("schemaVersion"), SchemaVersionValue) ||
        !Root->TryGetStringField(TEXT("biomeId"), BiomeIdString) || BiomeIdString.IsEmpty() ||
        !Root->TryGetBoolField(TEXT("prototypeOnly"), bPrototypeOnlyValue) ||
        !Root->TryGetArrayField(TEXT("interventions"), InterventionValues) || !InterventionValues)
    {
        OutError = TEXT("Ecological build header is invalid.");
        return false;
    }

    FWMEcologicalBuildDefinition Candidate;
    Candidate.SchemaVersion = static_cast<int32>(SchemaVersionValue);
    Candidate.BiomeId = FName(*BiomeIdString);
    Candidate.bPrototypeOnly = bPrototypeOnlyValue;

    for (const TSharedPtr<FJsonValue>& InterventionValue : *InterventionValues)
    {
        const TSharedPtr<FJsonObject>* InterventionObjectPtr = nullptr;
        if (!InterventionValue.IsValid() || !InterventionValue->TryGetObject(InterventionObjectPtr) ||
            !InterventionObjectPtr || !InterventionObjectPtr->IsValid())
        {
            OutError = TEXT("Ecological intervention entry is invalid.");
            return false;
        }

        const TSharedPtr<FJsonObject>& InterventionObject = *InterventionObjectPtr;
        FWMEcologicalBuildInterventionDefinition Intervention;
        FString InterventionIdString;
        FString RewardIdString;
        double RadiusValue = 0.0;
        const TArray<TSharedPtr<FJsonValue>>* RequirementValues = nullptr;
        if (!InterventionObject->TryGetStringField(TEXT("interventionId"), InterventionIdString) || InterventionIdString.IsEmpty() ||
            !ReadVector(InterventionObject, TEXT("anchorCm"), Intervention.AnchorCm) ||
            !InterventionObject->TryGetNumberField(TEXT("radiusCm"), RadiusValue) ||
            !ReadNameArray(InterventionObject, TEXT("prerequisiteInterventionIds"), Intervention.PrerequisiteInterventionIds) ||
            !InterventionObject->TryGetArrayField(TEXT("requirements"), RequirementValues) || !RequirementValues ||
            !InterventionObject->HasTypedField<EJson::Object>(TEXT("effectDelta")) ||
            !InterventionObject->TryGetStringField(TEXT("rewardId"), RewardIdString) || RewardIdString.IsEmpty())
        {
            OutError = TEXT("Ecological intervention fields are invalid.");
            return false;
        }

        Intervention.InterventionId = FName(*InterventionIdString);
        Intervention.RadiusCm = static_cast<float>(RadiusValue);
        Intervention.RewardId = FName(*RewardIdString);
        if (!ReadDelta(InterventionObject->GetObjectField(TEXT("effectDelta")), Intervention.EffectDelta))
        {
            OutError = TEXT("Ecological intervention effect delta is invalid.");
            return false;
        }

        for (const TSharedPtr<FJsonValue>& RequirementValue : *RequirementValues)
        {
            const TSharedPtr<FJsonObject>* RequirementObjectPtr = nullptr;
            if (!RequirementValue.IsValid() || !RequirementValue->TryGetObject(RequirementObjectPtr) ||
                !RequirementObjectPtr || !RequirementObjectPtr->IsValid())
            {
                OutError = TEXT("Ecological piece requirement is invalid.");
                return false;
            }
            FString PieceIdString;
            double MinCountValue = 0.0;
            if (!(*RequirementObjectPtr)->TryGetStringField(TEXT("pieceId"), PieceIdString) || PieceIdString.IsEmpty() ||
                !(*RequirementObjectPtr)->TryGetNumberField(TEXT("minCount"), MinCountValue))
            {
                OutError = TEXT("Ecological piece requirement fields are invalid.");
                return false;
            }
            FWMEcologicalPieceRequirement Requirement;
            Requirement.PieceId = FName(*PieceIdString);
            Requirement.MinCount = static_cast<int32>(MinCountValue);
            Intervention.Requirements.Add(MoveTemp(Requirement));
        }

        Candidate.Interventions.Add(MoveTemp(Intervention));
    }

    if (!Candidate.IsSane())
    {
        OutError = TEXT("Ecological build definition failed semantic validation.");
        return false;
    }

    OutDefinition = MoveTemp(Candidate);
    OutError.Reset();
    return true;
}

bool FWMEcologicalBuildProgressModel::ArePrerequisitesSatisfied(
    const FWMEcologicalBuildInterventionDefinition& Intervention) const
{
    for (const FName PrerequisiteId : Intervention.PrerequisiteInterventionIds)
    {
        if (!CompletedInterventionIds.Contains(PrerequisiteId)) return false;
    }
    return true;
}

bool FWMEcologicalBuildProgressModel::CanComplete(
    const FWMEcologicalBuildInterventionDefinition& Intervention,
    const TArray<FWMPlacedBuildPieceSnapshot>& Pieces) const
{
    return !CompletedInterventionIds.Contains(Intervention.InterventionId) &&
        ArePrerequisitesSatisfied(Intervention) && Intervention.IsSatisfiedBy(Pieces);
}

bool FWMEcologicalBuildProgressModel::MarkCompleted(const FName InterventionId)
{
    if (InterventionId.IsNone() || CompletedInterventionIds.Contains(InterventionId)) return false;
    CompletedInterventionIds.Add(InterventionId);
    return true;
}

bool FWMEcologicalBuildProgressModel::IsCompleted(const FName InterventionId) const
{
    return !InterventionId.IsNone() && CompletedInterventionIds.Contains(InterventionId);
}

TArray<FName> FWMEcologicalBuildProgressModel::GetCompletedInterventionIds() const
{
    TArray<FName> Result = CompletedInterventionIds.Array();
    Result.Sort([](const FName& A, const FName& B)
    {
        return A.ToString() < B.ToString();
    });
    return Result;
}

void FWMEcologicalBuildProgressModel::Reset()
{
    CompletedInterventionIds.Reset();
}
