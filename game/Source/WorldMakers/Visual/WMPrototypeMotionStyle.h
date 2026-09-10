#pragma once

#include "CoreMinimal.h"

/** Lightweight pose output used only by the temporary primitive avatar. */
struct WORLDMAKERS_API FWMPrototypeMotionPose
{
    float ArmSwingDegrees = 0.0f;
    float LegSwingDegrees = 0.0f;
    float BodyBobCm = 0.0f;
    float HeadBobCm = 0.0f;
    float BodyLeanDegrees = 0.0f;
};

/**
 * Deterministic source-only motion pass for the M1.8 primitive avatar.
 * V4/V5 replace this with a production skeletal mesh, rig and animation graph.
 */
struct WORLDMAKERS_API FWMPrototypeMotionStyle
{
    static FWMPrototypeMotionPose Evaluate(float SpeedAlpha, float PhaseRadians, bool bAirborne);
};
