#include "UI/WMChildJourneyTypes.h"

bool FWMChildAdventureCard::IsSane() const
{
    return !CardId.IsNone() && !SourceId.IsNone() &&
        CurrentUnits >= 0 && TotalUnits >= 1 && CurrentUnits <= TotalUnits &&
        FMath::IsFinite(ProgressFraction) && ProgressFraction >= 0.0f && ProgressFraction <= 1.0f;
}

int32 FWMChildJourneySnapshot::GetVisibleCardCount() const
{
    return Cards.CountByPredicate([](const FWMChildAdventureCard& Card)
    {
        return Card.State != EWMChildAdventureState::Hidden;
    });
}

int32 FWMChildJourneySnapshot::GetCompletedCardCount() const
{
    return Cards.CountByPredicate([](const FWMChildAdventureCard& Card)
    {
        return Card.State == EWMChildAdventureState::Complete;
    });
}

EWMChildAdventureState FWMChildJourneyRules::FromMissionState(const EWMJourneyMissionState State)
{
    switch (State)
    {
        case EWMJourneyMissionState::Available: return EWMChildAdventureState::Ready;
        case EWMJourneyMissionState::Active: return EWMChildAdventureState::InProgress;
        case EWMJourneyMissionState::Completed: return EWMChildAdventureState::Complete;
        default: return EWMChildAdventureState::Hidden;
    }
}

EWMChildAdventureState FWMChildJourneyRules::FromCountProgress(const int32 CurrentUnits, const int32 TotalUnits)
{
    if (TotalUnits <= 0) return EWMChildAdventureState::Hidden;
    if (CurrentUnits >= TotalUnits) return EWMChildAdventureState::Complete;
    if (CurrentUnits > 0) return EWMChildAdventureState::InProgress;
    return EWMChildAdventureState::Ready;
}

EWMChildAdventureState FWMChildJourneyRules::FromInterventionState(
    const bool bCompleted,
    const bool bPrerequisitesSatisfied,
    const int32 CurrentUnits,
    const int32 TotalUnits)
{
    if (bCompleted) return EWMChildAdventureState::Complete;
    if (!bPrerequisitesSatisfied) return EWMChildAdventureState::Hidden;
    return FromCountProgress(CurrentUnits, TotalUnits);
}

EWMChildAdventureState FWMChildJourneyRules::FromUnlockState(const bool bGranted, const bool bPrerequisiteVisible)
{
    if (bGranted) return EWMChildAdventureState::Complete;
    return bPrerequisiteVisible ? EWMChildAdventureState::Ready : EWMChildAdventureState::Hidden;
}
