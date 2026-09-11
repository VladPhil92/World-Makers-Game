#pragma once

#include "CoreMinimal.h"
#include "WMVisualCertificationTypes.generated.h"

USTRUCT(BlueprintType)
struct WORLDMAKERS_API FWMVisualCertificationBudget
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="World Makers|Visual Certification") int32 TargetFps = 30;
    UPROPERTY(BlueprintReadOnly, Category="World Makers|Visual Certification") float MaxP95FrameTimeMs = 33.34f;
    UPROPERTY(BlueprintReadOnly, Category="World Makers|Visual Certification") float MaxGameThreadP95Ms = 24.0f;
    UPROPERTY(BlueprintReadOnly, Category="World Makers|Visual Certification") float MaxRenderThreadP95Ms = 24.0f;
    UPROPERTY(BlueprintReadOnly, Category="World Makers|Visual Certification") float MaxGpuP95Ms = 30.0f;
    UPROPERTY(BlueprintReadOnly, Category="World Makers|Visual Certification") int32 MaxDrawCalls = 700;
    UPROPERTY(BlueprintReadOnly, Category="World Makers|Visual Certification") int32 MaxVisibleTriangles = 750000;
    UPROPERTY(BlueprintReadOnly, Category="World Makers|Visual Certification") int32 MaxResidentTextureMB = 512;
    UPROPERTY(BlueprintReadOnly, Category="World Makers|Visual Certification") int32 MaxActiveVfx = 8;

    bool IsSane() const;
};

USTRUCT(BlueprintType)
struct WORLDMAKERS_API FWMVisualCertificationSample
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="World Makers|Visual Certification") int32 FrameSamples = 0;
    UPROPERTY(BlueprintReadOnly, Category="World Makers|Visual Certification") float P95FrameTimeMs = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="World Makers|Visual Certification") float GameThreadP95Ms = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="World Makers|Visual Certification") float RenderThreadP95Ms = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="World Makers|Visual Certification") float GpuP95Ms = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="World Makers|Visual Certification") int32 PeakDrawCalls = 0;
    UPROPERTY(BlueprintReadOnly, Category="World Makers|Visual Certification") int32 PeakVisibleTriangles = 0;
    UPROPERTY(BlueprintReadOnly, Category="World Makers|Visual Certification") int32 PeakResidentTextureMB = 0;
    UPROPERTY(BlueprintReadOnly, Category="World Makers|Visual Certification") int32 PeakActiveVfx = 0;

    bool IsSane() const;
};

USTRUCT(BlueprintType)
struct WORLDMAKERS_API FWMVisualCertificationVerdict
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="World Makers|Visual Certification") bool bEnoughFrameSamples = false;
    UPROPERTY(BlueprintReadOnly, Category="World Makers|Visual Certification") bool bFrameTimeWithinBudget = false;
    UPROPERTY(BlueprintReadOnly, Category="World Makers|Visual Certification") bool bThreadsWithinBudget = false;
    UPROPERTY(BlueprintReadOnly, Category="World Makers|Visual Certification") bool bGpuWithinBudget = false;
    UPROPERTY(BlueprintReadOnly, Category="World Makers|Visual Certification") bool bDrawCallsWithinBudget = false;
    UPROPERTY(BlueprintReadOnly, Category="World Makers|Visual Certification") bool bTrianglesWithinBudget = false;
    UPROPERTY(BlueprintReadOnly, Category="World Makers|Visual Certification") bool bTextureMemoryWithinBudget = false;
    UPROPERTY(BlueprintReadOnly, Category="World Makers|Visual Certification") bool bVfxWithinBudget = false;
    UPROPERTY(BlueprintReadOnly, Category="World Makers|Visual Certification") bool bWithinBudget = false;
};

struct WORLDMAKERS_API FWMVisualCertificationEvaluator
{
    static FWMVisualCertificationVerdict Evaluate(const FWMVisualCertificationSample& Sample, const FWMVisualCertificationBudget& Budget, int32 MinimumFrameSamples = 1800);
};
