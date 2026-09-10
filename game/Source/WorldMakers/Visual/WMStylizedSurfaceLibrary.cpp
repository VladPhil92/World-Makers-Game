#include "Visual/WMStylizedSurfaceLibrary.h"

#include "Components/MeshComponent.h"

namespace WMStylizedSurface
{
    static constexpr int32 BaseColorIndex = 0;
    static constexpr int32 RoughnessIndex = 4;
    static constexpr int32 MetallicIndex = 5;
    static constexpr int32 EmissiveIndex = 6;
    static constexpr int32 WindIndex = 7;
    static constexpr int32 OpacityIndex = 8;
    static constexpr int32 RoleIndex = 9;

    FWMStylizedSurfaceLook MakeLook(
        const FLinearColor& Color,
        const float Roughness,
        const float Emissive = 0.0f,
        const float Wind = 0.0f,
        const float Opacity = 1.0f,
        const bool bTwoSided = false,
        const bool bTranslucent = false)
    {
        FWMStylizedSurfaceLook Look;
        Look.BaseColor = Color;
        Look.Roughness = Roughness;
        Look.EmissiveStrength = Emissive;
        Look.WindResponse = Wind;
        Look.OpacityIntent = Opacity;
        Look.bTwoSidedIntent = bTwoSided;
        Look.bTranslucentIntent = bTranslucent;
        return Look;
    }
}

FWMStylizedSurfaceLook UWMStylizedSurfaceLibrary::ResolveConfiguredLook(const EWMStylizedSurfaceRole Role)
{
    const UWMVisualProfileSettings* Profile = GetDefault<UWMVisualProfileSettings>();
    return Profile ? ResolveLook(*Profile, Role) : FWMStylizedSurfaceLook();
}

FWMStylizedSurfaceLook UWMStylizedSurfaceLibrary::ResolveLook(const UWMVisualProfileSettings& Profile, const EWMStylizedSurfaceRole Role)
{
    const FWMVisualPalette& Palette = Profile.Palette;
    const FWMStylizedSurfaceResponse& Response = Profile.SurfaceResponse;

    switch (Role)
    {
    case EWMStylizedSurfaceRole::GroundEarth:
        return WMStylizedSurface::MakeLook(Palette.Earth, Response.GroundRoughness);
    case EWMStylizedSurfaceRole::Terrain:
        return WMStylizedSurface::MakeLook(FLinearColor::LerpUsingHSV(Palette.Earth, Palette.CanopyDeep, 0.18f), Response.TerrainRoughness);
    case EWMStylizedSurfaceRole::Bark:
        return WMStylizedSurface::MakeLook(Palette.TrunkWarm, Response.BarkRoughness);
    case EWMStylizedSurfaceRole::Foliage:
        return WMStylizedSurface::MakeLook(Palette.LeafBright, Response.FoliageRoughness, 0.0f, Response.FoliageWindResponse, 1.0f, true, false);
    case EWMStylizedSurfaceRole::Stone:
        return WMStylizedSurface::MakeLook(Palette.Stone, Response.StoneRoughness);
    case EWMStylizedSurfaceRole::Water:
        return WMStylizedSurface::MakeLook(Palette.Water, Response.WaterRoughness, 0.0f, 0.25f, Response.WaterOpacityIntent, true, true);
    case EWMStylizedSurfaceRole::BuildEco:
        return WMStylizedSurface::MakeLook(Palette.BuildEco, Response.BuildRoughness);
    case EWMStylizedSurfaceRole::PreviewValid:
        return WMStylizedSurface::MakeLook(Palette.PreviewValid, Response.BuildRoughness, Response.PreviewEmissiveStrength);
    case EWMStylizedSurfaceRole::PreviewInvalid:
        return WMStylizedSurface::MakeLook(Palette.PreviewInvalid, Response.BuildRoughness, Response.PreviewEmissiveStrength);
    case EWMStylizedSurfaceRole::MagicalAccent:
        return WMStylizedSurface::MakeLook(Palette.MagicalAccent, 0.34f, Response.MagicalEmissiveStrength);
    case EWMStylizedSurfaceRole::BuildNeutral:
    default:
        return WMStylizedSurface::MakeLook(Palette.BuildNeutral, Response.BuildRoughness);
    }
}

bool UWMStylizedSurfaceLibrary::ApplyConfiguredSurface(UMeshComponent* Mesh, const EWMStylizedSurfaceRole Role)
{
    const UWMVisualProfileSettings* Profile = GetDefault<UWMVisualProfileSettings>();
    if (!Profile)
    {
        return false;
    }
    return ApplyLook(Mesh, Role, ResolveLook(*Profile, Role));
}

bool UWMStylizedSurfaceLibrary::ApplyLook(UMeshComponent* Mesh, const EWMStylizedSurfaceRole Role, const FWMStylizedSurfaceLook& Look)
{
    if (!Mesh || !Look.IsSane())
    {
        return false;
    }

    // Temporary visible fallback for /Engine/BasicShapes/BasicShapeMaterial.
    // Authored V2 materials consume the Custom Primitive Data contract below.
    Mesh->SetVectorParameterValueOnMaterials(TEXT("Color"), Look.BaseColor);

    Mesh->SetCustomPrimitiveDataVector4(
        WMStylizedSurface::BaseColorIndex,
        FVector4(Look.BaseColor.R, Look.BaseColor.G, Look.BaseColor.B, Look.BaseColor.A));
    Mesh->SetCustomPrimitiveDataFloat(WMStylizedSurface::RoughnessIndex, Look.Roughness);
    Mesh->SetCustomPrimitiveDataFloat(WMStylizedSurface::MetallicIndex, Look.Metallic);
    Mesh->SetCustomPrimitiveDataFloat(WMStylizedSurface::EmissiveIndex, Look.EmissiveStrength);
    Mesh->SetCustomPrimitiveDataFloat(WMStylizedSurface::WindIndex, Look.WindResponse);
    Mesh->SetCustomPrimitiveDataFloat(WMStylizedSurface::OpacityIndex, Look.OpacityIntent);

    const float RoleOrdinal = static_cast<float>(static_cast<uint8>(Role));
    const float MaxRoleOrdinal = static_cast<float>(static_cast<uint8>(EWMStylizedSurfaceRole::MagicalAccent));
    Mesh->SetCustomPrimitiveDataFloat(WMStylizedSurface::RoleIndex, MaxRoleOrdinal > 0.0f ? RoleOrdinal / MaxRoleOrdinal : 0.0f);
    return true;
}

FString UWMStylizedSurfaceLibrary::SurfaceRoleToString(const EWMStylizedSurfaceRole Role)
{
    switch (Role)
    {
    case EWMStylizedSurfaceRole::GroundEarth: return TEXT("ground-earth");
    case EWMStylizedSurfaceRole::Terrain: return TEXT("terrain");
    case EWMStylizedSurfaceRole::Bark: return TEXT("bark");
    case EWMStylizedSurfaceRole::Foliage: return TEXT("foliage");
    case EWMStylizedSurfaceRole::Stone: return TEXT("stone");
    case EWMStylizedSurfaceRole::Water: return TEXT("water");
    case EWMStylizedSurfaceRole::BuildNeutral: return TEXT("build-neutral");
    case EWMStylizedSurfaceRole::BuildEco: return TEXT("build-eco");
    case EWMStylizedSurfaceRole::PreviewValid: return TEXT("preview-valid");
    case EWMStylizedSurfaceRole::PreviewInvalid: return TEXT("preview-invalid");
    case EWMStylizedSurfaceRole::MagicalAccent: return TEXT("magical-accent");
    default: return TEXT("unknown");
    }
}
