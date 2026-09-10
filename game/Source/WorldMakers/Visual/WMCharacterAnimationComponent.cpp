#include "Visual/WMCharacterAnimationComponent.h"

#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Player/WMPlayerCharacter.h"
#include "Visual/WMAvatarArtTypes.h"

UWMCharacterAnimationComponent::UWMCharacterAnimationComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void UWMCharacterAnimationComponent::BeginPlay()
{
    Super::BeginPlay();
    Runtime.Reset();
}

void UWMCharacterAnimationComponent::TriggerAction(const EWMCharacterInteractionAction Action, const float DurationSeconds)
{
    Runtime.TriggerInteraction(Action, DurationSeconds);
}

bool UWMCharacterAnimationComponent::TriggerActionById(const FName ActionId, const float DurationSeconds)
{
    EWMCharacterInteractionAction Action = EWMCharacterInteractionAction::None;
    if (!FWMCharacterAnimationRuntime::TryParseInteractionAction(ActionId, Action) || Action == EWMCharacterInteractionAction::None)
    {
        return false;
    }

    TriggerAction(Action, DurationSeconds);
    return true;
}

FWMCharacterAnimationInput UWMCharacterAnimationComponent::BuildInput(const AWMPlayerCharacter& Character, const float DeltaSeconds) const
{
    FWMCharacterAnimationInput Input;
    Input.DeltaSeconds = DeltaSeconds;
    Input.SpeedCmPerSec = Character.GetVelocity().Size2D();
    Input.VerticalVelocityCmPerSec = Character.GetVelocity().Z;

    if (const UCharacterMovementComponent* Movement = Character.GetCharacterMovement())
    {
        Input.MaxSpeedCmPerSec = FMath::Max(Movement->GetMaxSpeed(), 1.0f);
        Input.bAirborne = Movement->IsFalling();
    }

    const FRotator ActorRotation = Character.GetActorRotation();
    const FRotator ViewRotation = Character.GetController() ? Character.GetController()->GetControlRotation() : ActorRotation;
    const float ViewYawDelta = FMath::FindDeltaAngleDegrees(ActorRotation.Yaw, ViewRotation.Yaw);
    const float NormalizedPitch = FRotator::NormalizeAxis(ViewRotation.Pitch);

    Input.LookYawDegrees = FMath::Clamp(ViewYawDelta, -60.0f, 60.0f);
    Input.LookPitchDegrees = FMath::Clamp(NormalizedPitch, -35.0f, 35.0f);
    Input.TurnInput = Input.SpeedCmPerSec <= 5.0f ? FMath::Clamp(ViewYawDelta / 60.0f, -1.0f, 1.0f) : 0.0f;
    return Input;
}

void UWMCharacterAnimationComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    AWMPlayerCharacter* Character = Cast<AWMPlayerCharacter>(GetOwner());
    if (!Character)
    {
        return;
    }

    LastSourcePose = Runtime.Update(BuildInput(*Character, DeltaTime));
    if (Character->IsProceduralAvatarActive())
    {
        ApplyProceduralPose(*Character, LastSourcePose);
    }
}

void UWMCharacterAnimationComponent::ApplyProceduralPose(AWMPlayerCharacter& Character, const FWMCharacterAnimationPose& Pose) const
{
    if (!Pose.IsBounded() || !Character.AvatarRigRoot || !Character.AvatarPelvisRig || !Character.AvatarSpineRig ||
        !Character.AvatarChestRig || !Character.AvatarHeadRig || !Character.AvatarUpperArmLeftRig ||
        !Character.AvatarUpperArmRightRig || !Character.AvatarLowerArmLeftRig || !Character.AvatarLowerArmRightRig ||
        !Character.AvatarHandLeftRig || !Character.AvatarHandRightRig || !Character.AvatarThighLeftRig ||
        !Character.AvatarThighRightRig || !Character.AvatarCalfLeftRig || !Character.AvatarCalfRightRig ||
        !Character.AvatarFootLeftRig || !Character.AvatarFootRightRig)
    {
        return;
    }

    const UWMAvatarVisualSettings* Settings = GetDefault<UWMAvatarVisualSettings>();
    const float HeadBaseZ = Settings ? Settings->Proportions.HeadHeightCm * 0.50f : 15.8f;

    Character.AvatarRigRoot->SetRelativeLocation(FVector(0.0f, 0.0f, Pose.RootOffsetZCm));
    Character.AvatarRigRoot->SetRelativeRotation(FRotator(Pose.BodyLeanDegrees, 0.0f, 0.0f));
    Character.AvatarPelvisRig->SetRelativeRotation(FRotator(0.0f, Pose.PelvisYawDegrees, 0.0f));
    Character.AvatarChestRig->SetRelativeRotation(FRotator(0.0f, Pose.ChestYawDegrees, 0.0f));
    Character.AvatarHeadRig->SetRelativeLocation(FVector(0.0f, 0.0f, HeadBaseZ));
    Character.AvatarHeadRig->SetRelativeRotation(FRotator(Pose.HeadPitchDegrees, Pose.HeadYawDegrees, 0.0f));

    Character.AvatarUpperArmLeftRig->SetRelativeRotation(FRotator(Pose.UpperArmLeftPitchDegrees, 0.0f, -5.0f));
    Character.AvatarUpperArmRightRig->SetRelativeRotation(FRotator(Pose.UpperArmRightPitchDegrees, 0.0f, 5.0f));
    Character.AvatarLowerArmLeftRig->SetRelativeRotation(FRotator(Pose.LowerArmLeftPitchDegrees, 0.0f, 0.0f));
    Character.AvatarLowerArmRightRig->SetRelativeRotation(FRotator(Pose.LowerArmRightPitchDegrees, 0.0f, 0.0f));
    Character.AvatarHandLeftRig->SetRelativeRotation(FRotator(Pose.HandLeftPitchDegrees, 0.0f, 0.0f));
    Character.AvatarHandRightRig->SetRelativeRotation(FRotator(Pose.HandRightPitchDegrees, 0.0f, 0.0f));

    Character.AvatarThighLeftRig->SetRelativeRotation(FRotator(Pose.ThighLeftPitchDegrees, 0.0f, 0.0f));
    Character.AvatarThighRightRig->SetRelativeRotation(FRotator(Pose.ThighRightPitchDegrees, 0.0f, 0.0f));
    Character.AvatarCalfLeftRig->SetRelativeRotation(FRotator(Pose.CalfLeftPitchDegrees, 0.0f, 0.0f));
    Character.AvatarCalfRightRig->SetRelativeRotation(FRotator(Pose.CalfRightPitchDegrees, 0.0f, 0.0f));
    Character.AvatarFootLeftRig->SetRelativeRotation(FRotator(Pose.FootLeftPitchDegrees, 0.0f, 0.0f));
    Character.AvatarFootRightRig->SetRelativeRotation(FRotator(Pose.FootRightPitchDegrees, 0.0f, 0.0f));
}

bool UWMCharacterAnimationComponent::IsProductionAnimBPExpected() const
{
    const AWMPlayerCharacter* Character = Cast<AWMPlayerCharacter>(GetOwner());
    return Character && Character->GetMesh() && Character->GetMesh()->GetSkeletalMeshAsset() != nullptr && !Character->IsProceduralAvatarActive();
}
