#include "Visual/WMPrototypeMotionStyle.h"

FWMPrototypeMotionPose FWMPrototypeMotionStyle::Evaluate(const float SpeedAlpha, const float PhaseRadians, const bool bAirborne)
{
    FWMPrototypeMotionPose Pose;
    const float Speed = FMath::Clamp(FMath::IsFinite(SpeedAlpha) ? SpeedAlpha : 0.0f, 0.0f, 1.0f);
    const float Phase = FMath::IsFinite(PhaseRadians) ? PhaseRadians : 0.0f;

    if (bAirborne)
    {
        Pose.ArmSwingDegrees = -18.0f;
        Pose.LegSwingDegrees = 10.0f;
        Pose.BodyBobCm = 1.5f;
        Pose.HeadBobCm = 1.0f;
        Pose.BodyLeanDegrees = 6.0f;
        return Pose;
    }

    const float Gait = FMath::Sin(Phase);
    const float Lift = FMath::Abs(FMath::Sin(Phase * 2.0f));
    Pose.ArmSwingDegrees = Gait * 28.0f * Speed;
    Pose.LegSwingDegrees = -Gait * 24.0f * Speed;
    Pose.BodyBobCm = Lift * 2.2f * Speed;
    Pose.HeadBobCm = Lift * 1.2f * Speed;
    Pose.BodyLeanDegrees = 4.0f * Speed;
    return Pose;
}
