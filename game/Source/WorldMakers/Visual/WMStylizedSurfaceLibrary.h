#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Visual/WMVisualProfileSettings.h"
#include "WMStylizedSurfaceLibrary.generated.h"

class UMeshComponent;

/**
 * Stable material-parameter layout shared by proxy fallback rendering and future authored materials.
 * Custom Primitive Data indices:
 * 0-3 BaseColor RGBA, 4 Roughness, 5 Metallic, 6 EmissiveStrength,
 * 7 WindResponse, 8 OpacityIntent, 9 SurfaceRoleNormalized.
 */
UCLASS()
class WORLDMAKERS_API UWMStylizedSurfaceLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "World Makers|Visual|Surface")
    static FWMStylizedSurfaceLook ResolveConfiguredLook(EWMStylizedSurfaceRole Role);

    UFUNCTION(BlueprintCallable, Category = "World Makers|Visual|Surface")
    static bool ApplyConfiguredSurface(UMeshComponent* Mesh, EWMStylizedSurfaceRole Role);

    UFUNCTION(BlueprintPure, Category = "World Makers|Visual|Surface")
    static FString SurfaceRoleToString(EWMStylizedSurfaceRole Role);

    static FWMStylizedSurfaceLook ResolveLook(const UWMVisualProfileSettings& Profile, EWMStylizedSurfaceRole Role);
    static bool ApplyLook(UMeshComponent* Mesh, EWMStylizedSurfaceRole Role, const FWMStylizedSurfaceLook& Look);
};
