#include "Building/WMBuildUnlockSubsystem.h"

#include "Kismet/GameplayStatics.h"

namespace
{
    const FString CreativeUnlockSaveSlot(TEXT("WM_CreativeUnlocks_Prototype"));
    constexpr int32 CreativeUnlockUserIndex = 0;
}

bool FWMBuildUnlockModel::Grant(const FName RewardId)
{
    if (RewardId.IsNone() || GrantedRewardIds.Contains(RewardId))
    {
        return false;
    }
    GrantedRewardIds.Add(RewardId);
    return true;
}

bool FWMBuildUnlockModel::Has(const FName RewardId) const
{
    return !RewardId.IsNone() && GrantedRewardIds.Contains(RewardId);
}

bool FWMBuildUnlockModel::IsPieceUnlocked(const FWMBuildPieceSpec& Spec) const
{
    return Spec.IsSane() && (Spec.RequiredRewardId.IsNone() || Has(Spec.RequiredRewardId));
}

void FWMBuildUnlockModel::Restore(const TArray<FName>& RewardIds)
{
    GrantedRewardIds.Reset();
    for (const FName RewardId : RewardIds)
    {
        if (!RewardId.IsNone()) GrantedRewardIds.Add(RewardId);
    }
}

TArray<FName> FWMBuildUnlockModel::Export() const
{
    TArray<FName> Result = GrantedRewardIds.Array();
    Result.Sort([](const FName& A, const FName& B)
    {
        return A.ToString() < B.ToString();
    });
    return Result;
}

void UWMBuildUnlockSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    LoadProgress();
}

bool UWMBuildUnlockSubsystem::IsKnownUnlockReward(const FName RewardId) const
{
    if (RewardId.IsNone()) return false;
    const UWMBuildCatalogSettings* Catalog = GetDefault<UWMBuildCatalogSettings>();
    if (!Catalog) return false;

    for (const FWMBuildPieceSpec& Spec : Catalog->Pieces)
    {
        if (Spec.IsSane() && Spec.RequiredRewardId == RewardId)
        {
            return true;
        }
    }
    return false;
}

bool UWMBuildUnlockSubsystem::IsPieceUnlocked(const FName PieceId) const
{
    const UWMBuildCatalogSettings* Catalog = GetDefault<UWMBuildCatalogSettings>();
    FWMBuildPieceSpec Spec;
    return Catalog && Catalog->FindPieceSpec(PieceId, Spec) && Model.IsPieceUnlocked(Spec);
}

bool UWMBuildUnlockSubsystem::EnsureRewardGranted(const FName RewardId)
{
    if (!IsKnownUnlockReward(RewardId))
    {
        return false;
    }

    if (Model.Grant(RewardId))
    {
        SaveProgress();
    }
    return Model.Has(RewardId);
}

TArray<FName> UWMBuildUnlockSubsystem::GetUnlockedPieceIds() const
{
    TArray<FName> Result;
    const UWMBuildCatalogSettings* Catalog = GetDefault<UWMBuildCatalogSettings>();
    if (!Catalog) return Result;

    for (const FWMBuildPieceSpec& Spec : Catalog->Pieces)
    {
        if (Model.IsPieceUnlocked(Spec) && !Result.Contains(Spec.PieceId))
        {
            Result.Add(Spec.PieceId);
        }
    }
    return Result;
}

bool UWMBuildUnlockSubsystem::LoadProgress()
{
    Model.Restore({});
    if (!UGameplayStatics::DoesSaveGameExist(CreativeUnlockSaveSlot, CreativeUnlockUserIndex))
    {
        return true;
    }

    UWMBuildUnlockSaveGame* Save = Cast<UWMBuildUnlockSaveGame>(
        UGameplayStatics::LoadGameFromSlot(CreativeUnlockSaveSlot, CreativeUnlockUserIndex));
    if (!Save || Save->FormatVersion != UWMBuildUnlockSaveGame::CurrentFormatVersion)
    {
        return false;
    }

    TArray<FName> Sanitized;
    for (const FName RewardId : Save->GrantedRewardIds)
    {
        if (IsKnownUnlockReward(RewardId) && !Sanitized.Contains(RewardId))
        {
            Sanitized.Add(RewardId);
        }
    }
    Model.Restore(Sanitized);
    return true;
}

bool UWMBuildUnlockSubsystem::SaveProgress() const
{
    UWMBuildUnlockSaveGame* Save = Cast<UWMBuildUnlockSaveGame>(
        UGameplayStatics::CreateSaveGameObject(UWMBuildUnlockSaveGame::StaticClass()));
    if (!Save) return false;

    Save->FormatVersion = UWMBuildUnlockSaveGame::CurrentFormatVersion;
    Save->GrantedRewardIds = Model.Export();
    return UGameplayStatics::SaveGameToSlot(Save, CreativeUnlockSaveSlot, CreativeUnlockUserIndex);
}
