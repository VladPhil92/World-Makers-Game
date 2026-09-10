#pragma once

#include "CoreMinimal.h"
#include "Performance/WMPerformanceProfileTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "WMPerformanceCaptureSubsystem.generated.h"

UCLASS()
class WORLDMAKERS_API UWMPerformanceCaptureSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override { return bCapturing; }

    UFUNCTION(BlueprintCallable, Category = "World Makers|Performance|Capture")
    bool BeginCapture();

    UFUNCTION(BlueprintCallable, Category = "World Makers|Performance|Capture")
    FWMPerformanceCaptureSummary EndCapture(bool bWriteReport = true);

    UFUNCTION(BlueprintPure, Category = "World Makers|Performance|Capture")
    bool IsCapturing() const { return bCapturing; }

    UFUNCTION(BlueprintPure, Category = "World Makers|Performance|Capture")
    FWMPerformanceCaptureSummary GetLiveSummary() const;

    UFUNCTION(BlueprintPure, Category = "World Makers|Performance|Capture")
    FWMPerformanceCaptureSummary GetLastCaptureSummary() const { return LastSummary; }

    UFUNCTION(BlueprintPure, Category = "World Makers|Performance|Capture")
    FString GetLastReportPath() const { return LastReportPath; }

private:
    void ObserveStructuralCounts();
    bool WriteSummaryReport(const FWMPerformanceCaptureSummary& Summary);

    bool bCapturing = false;
    float StructuralSampleAccumulatorSeconds = 0.0f;
    FName CaptureProfileId;
    FWMPerformanceBudget CaptureBudget;
    FWMPerformanceCaptureAccumulator Accumulator;
    FWMPerformanceCaptureSummary LastSummary;
    FString LastReportPath;

    static constexpr float StructuralSampleIntervalSeconds = 0.50f;
};
