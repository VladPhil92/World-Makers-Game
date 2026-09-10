#include "Performance/WMVisualCertificationTypes.h"

bool FWMVisualCertificationBudget::IsSane() const
{
    const float ExpectedFrameMs = TargetFps > 0 ? 1000.0f / static_cast<float>(TargetFps) : 0.0f;
    return TargetFps >= 30 && TargetFps <= 120 &&
        FMath::IsFinite(MaxP95FrameTimeMs) && MaxP95FrameTimeMs > 0.0f &&
        FMath::Abs(MaxP95FrameTimeMs - ExpectedFrameMs) <= 0.05f &&
        FMath::IsFinite(MaxGameThreadP95Ms) && MaxGameThreadP95Ms > 0.0f &&
        FMath::IsFinite(MaxRenderThreadP95Ms) && MaxRenderThreadP95Ms > 0.0f &&
        FMath::IsFinite(MaxGpuP95Ms) && MaxGpuP95Ms > 0.0f &&
        MaxDrawCalls > 0 && MaxDrawCalls <= 5000 &&
        MaxVisibleTriangles > 0 && MaxVisibleTriangles <= 10000000 &&
        MaxResidentTextureMB >= 128 && MaxResidentTextureMB <= 4096 &&
        MaxActiveVfx >= 1 && MaxActiveVfx <= 64;
}

bool FWMVisualCertificationSample::IsSane() const
{
    return FrameSamples >= 0 &&
        FMath::IsFinite(P95FrameTimeMs) && P95FrameTimeMs >= 0.0f &&
        FMath::IsFinite(GameThreadP95Ms) && GameThreadP95Ms >= 0.0f &&
        FMath::IsFinite(RenderThreadP95Ms) && RenderThreadP95Ms >= 0.0f &&
        FMath::IsFinite(GpuP95Ms) && GpuP95Ms >= 0.0f &&
        PeakDrawCalls >= 0 && PeakVisibleTriangles >= 0 && PeakResidentTextureMB >= 0 && PeakActiveVfx >= 0;
}

FWMVisualCertificationVerdict FWMVisualCertificationEvaluator::Evaluate(
    const FWMVisualCertificationSample& Sample,
    const FWMVisualCertificationBudget& Budget,
    const int32 MinimumFrameSamples)
{
    FWMVisualCertificationVerdict Result;
    if (!Sample.IsSane() || !Budget.IsSane() || MinimumFrameSamples < 1)
    {
        return Result;
    }

    Result.bEnoughFrameSamples = Sample.FrameSamples >= MinimumFrameSamples;
    Result.bFrameTimeWithinBudget = Sample.P95FrameTimeMs <= Budget.MaxP95FrameTimeMs;
    Result.bThreadsWithinBudget = Sample.GameThreadP95Ms <= Budget.MaxGameThreadP95Ms && Sample.RenderThreadP95Ms <= Budget.MaxRenderThreadP95Ms;
    Result.bGpuWithinBudget = Sample.GpuP95Ms <= Budget.MaxGpuP95Ms;
    Result.bDrawCallsWithinBudget = Sample.PeakDrawCalls <= Budget.MaxDrawCalls;
    Result.bTrianglesWithinBudget = Sample.PeakVisibleTriangles <= Budget.MaxVisibleTriangles;
    Result.bTextureMemoryWithinBudget = Sample.PeakResidentTextureMB <= Budget.MaxResidentTextureMB;
    Result.bVfxWithinBudget = Sample.PeakActiveVfx <= Budget.MaxActiveVfx;
    Result.bWithinBudget = Result.bEnoughFrameSamples && Result.bFrameTimeWithinBudget && Result.bThreadsWithinBudget &&
        Result.bGpuWithinBudget && Result.bDrawCallsWithinBudget && Result.bTrianglesWithinBudget &&
        Result.bTextureMemoryWithinBudget && Result.bVfxWithinBudget;
    return Result;
}
