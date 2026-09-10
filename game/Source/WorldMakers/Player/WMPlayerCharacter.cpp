#include "Player/WMPlayerCharacter.h"

#include "Building/WMBuildingComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Environment/WMExplorationComponent.h"
#include "Environment/WMInteractionComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Input/WMTouchGestureLibrary.h"
#include "Materials/MaterialInterface.h"
#include "Mission/WMMissionMeasurementComponent.h"
#include "Mission/WMMissionRuntimeSubsystem.h"
#include "ProceduralMeshComponent.h"
#include "UI/WMBuildHUDWidget.h"
#include "UObject/ConstructorHelpers.h"
#include "Visual/WMAvatarArtTypes.h"
#include "Visual/WMProceduralAvatarGeometry.h"
#include "Visual/WMPrototypeMotionStyle.h"
#include "Visual/WMVisualProfileSettings.h"

namespace WMAvatarArt
{
    void ConfigureRenderOnly(UProceduralMeshComponent* Component)
    {
        if (!Component)
        {
            return;
        }
        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Component->SetCanEverAffectNavigation(false);
        Component->bUseAsyncCooking = false;
    }

    bool Commit(UProceduralMeshComponent* Component, const FWMEnvironmentMeshData& Mesh, const FLinearColor& Color)
    {
        if (!Component || !Mesh.IsSane())
        {
            return false;
        }
        Component->ClearAllMeshSections();
        Component->CreateMeshSection_LinearColor(
            0,
            Mesh.Vertices,
            Mesh.Triangles,
            Mesh.Normals,
            Mesh.UV0,
            Mesh.VertexColors,
            Mesh.Tangents,
            false);
        Component->SetVectorParameterValueOnMaterials(TEXT("Color"), Color);
        return true;
    }
}

AWMPlayerCharacter::AWMPlayerCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
    GetCharacterMovement()->JumpZVelocity = 500.0f;
    GetCharacterMovement()->AirControl = 0.25f;
    GetCharacterMovement()->MaxWalkSpeed = 450.0f;

    // ACharacter already owns the production SkeletalMeshComponent. V4 keeps it collision-free and reserves it
    // as the authored avatar destination; source-visible procedural art is used while no production mesh exists.
    if (GetMesh())
    {
        GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        GetMesh()->SetVisibility(false, true);
    }

    // Legacy V1 six-piece visual remains rollback-only.
    PrototypeBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrototypeBody"));
    PrototypeBody->SetupAttachment(RootComponent);
    PrototypeBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PrototypeBody->SetRelativeLocation(FVector(0.0f, 0.0f, 12.0f));
    PrototypeBody->SetRelativeScale3D(FVector(0.34f, 0.30f, 0.62f));

    PrototypeHead = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrototypeHead"));
    PrototypeHead->SetupAttachment(RootComponent);
    PrototypeHead->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PrototypeHead->SetRelativeLocation(FVector(0.0f, 0.0f, 62.0f));
    PrototypeHead->SetRelativeScale3D(FVector(0.40f));

    PrototypeLeftArm = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrototypeLeftArm"));
    PrototypeLeftArm->SetupAttachment(RootComponent);
    PrototypeLeftArm->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PrototypeLeftArm->SetRelativeLocation(FVector(0.0f, -24.0f, 12.0f));
    PrototypeLeftArm->SetRelativeRotation(FRotator(0.0f, 0.0f, -7.0f));
    PrototypeLeftArm->SetRelativeScale3D(FVector(0.11f, 0.11f, 0.46f));

    PrototypeRightArm = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrototypeRightArm"));
    PrototypeRightArm->SetupAttachment(RootComponent);
    PrototypeRightArm->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PrototypeRightArm->SetRelativeLocation(FVector(0.0f, 24.0f, 12.0f));
    PrototypeRightArm->SetRelativeRotation(FRotator(0.0f, 0.0f, 7.0f));
    PrototypeRightArm->SetRelativeScale3D(FVector(0.11f, 0.11f, 0.46f));

    PrototypeLeftLeg = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrototypeLeftLeg"));
    PrototypeLeftLeg->SetupAttachment(RootComponent);
    PrototypeLeftLeg->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PrototypeLeftLeg->SetRelativeLocation(FVector(0.0f, -10.0f, -45.0f));
    PrototypeLeftLeg->SetRelativeScale3D(FVector(0.15f, 0.15f, 0.50f));

    PrototypeRightLeg = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrototypeRightLeg"));
    PrototypeRightLeg->SetupAttachment(RootComponent);
    PrototypeRightLeg->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PrototypeRightLeg->SetRelativeLocation(FVector(0.0f, 10.0f, -45.0f));
    PrototypeRightLeg->SetRelativeScale3D(FVector(0.15f, 0.15f, 0.50f));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> HeadMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (CylinderMesh.Succeeded())
    {
        PrototypeBody->SetStaticMesh(CylinderMesh.Object);
        PrototypeLeftArm->SetStaticMesh(CylinderMesh.Object);
        PrototypeRightArm->SetStaticMesh(CylinderMesh.Object);
        PrototypeLeftLeg->SetStaticMesh(CylinderMesh.Object);
        PrototypeRightLeg->SetStaticMesh(CylinderMesh.Object);
    }
    if (HeadMesh.Succeeded())
    {
        PrototypeHead->SetStaticMesh(HeadMesh.Object);
    }

    // V4 joint hierarchy. Names intentionally mirror the stable rig contract for V5 handoff.
    AvatarRigRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Rig_root"));
    AvatarRigRoot->SetupAttachment(RootComponent);

    AvatarPelvisRig = CreateDefaultSubobject<USceneComponent>(TEXT("Rig_pelvis"));
    AvatarPelvisRig->SetupAttachment(AvatarRigRoot);

    AvatarSpineRig = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Rig_spine"));
    AvatarSpineRig->SetupAttachment(AvatarPelvisRig);
    WMAvatarArt::ConfigureRenderOnly(AvatarSpineRig);

    AvatarChestRig = CreateDefaultSubobject<USceneComponent>(TEXT("Rig_chest"));
    AvatarChestRig->SetupAttachment(AvatarSpineRig);

    AvatarNeckRig = CreateDefaultSubobject<USceneComponent>(TEXT("Rig_neck"));
    AvatarNeckRig->SetupAttachment(AvatarChestRig);

    AvatarHeadRig = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Rig_head"));
    AvatarHeadRig->SetupAttachment(AvatarNeckRig);
    WMAvatarArt::ConfigureRenderOnly(AvatarHeadRig);

    AvatarJawRig = CreateDefaultSubobject<USceneComponent>(TEXT("Rig_jaw"));
    AvatarJawRig->SetupAttachment(AvatarHeadRig);

    AvatarHairArt = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Art_hair"));
    AvatarHairArt->SetupAttachment(AvatarHeadRig);
    WMAvatarArt::ConfigureRenderOnly(AvatarHairArt);

    AvatarUpperArmLeftRig = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Rig_upperarm_l"));
    AvatarUpperArmLeftRig->SetupAttachment(AvatarChestRig);
    WMAvatarArt::ConfigureRenderOnly(AvatarUpperArmLeftRig);

    AvatarLowerArmLeftRig = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Rig_lowerarm_l"));
    AvatarLowerArmLeftRig->SetupAttachment(AvatarUpperArmLeftRig);
    WMAvatarArt::ConfigureRenderOnly(AvatarLowerArmLeftRig);

    AvatarHandLeftRig = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Rig_hand_l"));
    AvatarHandLeftRig->SetupAttachment(AvatarLowerArmLeftRig);
    WMAvatarArt::ConfigureRenderOnly(AvatarHandLeftRig);

    AvatarUpperArmRightRig = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Rig_upperarm_r"));
    AvatarUpperArmRightRig->SetupAttachment(AvatarChestRig);
    WMAvatarArt::ConfigureRenderOnly(AvatarUpperArmRightRig);

    AvatarLowerArmRightRig = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Rig_lowerarm_r"));
    AvatarLowerArmRightRig->SetupAttachment(AvatarUpperArmRightRig);
    WMAvatarArt::ConfigureRenderOnly(AvatarLowerArmRightRig);

    AvatarHandRightRig = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Rig_hand_r"));
    AvatarHandRightRig->SetupAttachment(AvatarLowerArmRightRig);
    WMAvatarArt::ConfigureRenderOnly(AvatarHandRightRig);

    AvatarThighLeftRig = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Rig_thigh_l"));
    AvatarThighLeftRig->SetupAttachment(AvatarPelvisRig);
    WMAvatarArt::ConfigureRenderOnly(AvatarThighLeftRig);

    AvatarCalfLeftRig = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Rig_calf_l"));
    AvatarCalfLeftRig->SetupAttachment(AvatarThighLeftRig);
    WMAvatarArt::ConfigureRenderOnly(AvatarCalfLeftRig);

    AvatarFootLeftRig = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Rig_foot_l"));
    AvatarFootLeftRig->SetupAttachment(AvatarCalfLeftRig);
    WMAvatarArt::ConfigureRenderOnly(AvatarFootLeftRig);

    AvatarThighRightRig = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Rig_thigh_r"));
    AvatarThighRightRig->SetupAttachment(AvatarPelvisRig);
    WMAvatarArt::ConfigureRenderOnly(AvatarThighRightRig);

    AvatarCalfRightRig = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Rig_calf_r"));
    AvatarCalfRightRig->SetupAttachment(AvatarThighRightRig);
    WMAvatarArt::ConfigureRenderOnly(AvatarCalfRightRig);

    AvatarFootRightRig = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Rig_foot_r"));
    AvatarFootRightRig->SetupAttachment(AvatarCalfRightRig);
    WMAvatarArt::ConfigureRenderOnly(AvatarFootRightRig);

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> AvatarFallbackMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (AvatarFallbackMaterial.Succeeded())
    {
        for (UProceduralMeshComponent* Part : TArray<UProceduralMeshComponent*>{
            AvatarSpineRig, AvatarHeadRig, AvatarHairArt,
            AvatarUpperArmLeftRig, AvatarLowerArmLeftRig, AvatarHandLeftRig,
            AvatarUpperArmRightRig, AvatarLowerArmRightRig, AvatarHandRightRig,
            AvatarThighLeftRig, AvatarCalfLeftRig, AvatarFootLeftRig,
            AvatarThighRightRig, AvatarCalfRightRig, AvatarFootRightRig})
        {
            Part->SetMaterial(0, AvatarFallbackMaterial.Object);
        }
    }

    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 500.0f;
    CameraBoom->bUsePawnControlRotation = true;

    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;

    BuildingComponent = CreateDefaultSubobject<UWMBuildingComponent>(TEXT("BuildingComponent"));
    MissionMeasurementComponent = CreateDefaultSubobject<UWMMissionMeasurementComponent>(TEXT("MissionMeasurementComponent"));
    ExplorationComponent = CreateDefaultSubobject<UWMExplorationComponent>(TEXT("ExplorationComponent"));
    InteractionComponent = CreateDefaultSubobject<UWMInteractionComponent>(TEXT("InteractionComponent"));
}

void AWMPlayerCharacter::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    BuildProceduralAvatarArt();
}

void AWMPlayerCharacter::BeginPlay()
{
    Super::BeginPlay();
    BuildProceduralAvatarArt();
    ApplyVisualProfileToCamera();
    EnsureBuildHUD();
}

void AWMPlayerCharacter::PawnClientRestart()
{
    Super::PawnClientRestart();
    BuildProceduralAvatarArt();
    ApplyVisualProfileToCamera();
    EnsureBuildHUD();
}

void AWMPlayerCharacter::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    UpdatePrototypeMotion(DeltaSeconds);
}

void AWMPlayerCharacter::ApplyVisualProfileToCamera()
{
    const UWMVisualProfileSettings* Profile = GetDefault<UWMVisualProfileSettings>();
    if (Profile && Profile->Atmosphere.IsSane() && FollowCamera)
    {
        FollowCamera->SetFieldOfView(Profile->Atmosphere.CameraFOVDegrees);
    }
}

void AWMPlayerCharacter::BuildProceduralAvatarArt()
{
    const UWMAvatarVisualSettings* AvatarSettings = GetDefault<UWMAvatarVisualSettings>();
    const UWMVisualProfileSettings* VisualSettings = GetDefault<UWMVisualProfileSettings>();
    if (!AvatarSettings || !VisualSettings || !AvatarSettings->Proportions.IsSane() || !FWMAvatarRigContract::IsSane())
    {
        bProceduralAvatarReady = false;
        RefreshAvatarVisualPath();
        return;
    }

    const FWMAvatarProportions& P = AvatarSettings->Proportions;
    const FWMAvatarArtBudget& Budget = AvatarSettings->GetBudget(VisualSettings->DefaultQualityTier);
    if (!Budget.IsSane() || FWMProceduralAvatarGeometry::EstimateTriangleCount(P, Budget) > Budget.MaxTriangles)
    {
        bProceduralAvatarReady = false;
        RefreshAvatarVisualPath();
        return;
    }

    AvatarRigRoot->SetRelativeLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
    AvatarPelvisRig->SetRelativeLocation(FVector(0.0f, 0.0f, -5.0f));
    AvatarSpineRig->SetRelativeLocation(FVector::ZeroVector);
    AvatarChestRig->SetRelativeLocation(FVector(0.0f, 0.0f, P.TorsoHeightCm * 0.84f));
    AvatarNeckRig->SetRelativeLocation(FVector(0.0f, 0.0f, P.TorsoHeightCm * 0.16f));
    AvatarHeadRig->SetRelativeLocation(FVector(0.0f, 0.0f, P.HeadHeightCm * 0.50f));
    AvatarJawRig->SetRelativeLocation(FVector(P.HeadHeightCm * 0.34f, 0.0f, -P.HeadHeightCm * 0.20f));
    AvatarHairArt->SetRelativeLocation(FVector::ZeroVector);

    const float ShoulderY = P.ShoulderWidthCm * 0.50f;
    const float HipY = P.HipWidthCm * 0.30f;
    AvatarUpperArmLeftRig->SetRelativeLocation(FVector(0.0f, -ShoulderY, P.TorsoHeightCm * 0.15f));
    AvatarUpperArmRightRig->SetRelativeLocation(FVector(0.0f, ShoulderY, P.TorsoHeightCm * 0.15f));
    AvatarLowerArmLeftRig->SetRelativeLocation(FVector(0.0f, 0.0f, -P.UpperArmLengthCm));
    AvatarLowerArmRightRig->SetRelativeLocation(FVector(0.0f, 0.0f, -P.UpperArmLengthCm));
    AvatarHandLeftRig->SetRelativeLocation(FVector(0.0f, 0.0f, -P.LowerArmLengthCm));
    AvatarHandRightRig->SetRelativeLocation(FVector(0.0f, 0.0f, -P.LowerArmLengthCm));

    AvatarThighLeftRig->SetRelativeLocation(FVector(0.0f, -HipY, 0.0f));
    AvatarThighRightRig->SetRelativeLocation(FVector(0.0f, HipY, 0.0f));
    AvatarCalfLeftRig->SetRelativeLocation(FVector(0.0f, 0.0f, -P.ThighLengthCm));
    AvatarCalfRightRig->SetRelativeLocation(FVector(0.0f, 0.0f, -P.ThighLengthCm));
    AvatarFootLeftRig->SetRelativeLocation(FVector(0.0f, 0.0f, -P.CalfLengthCm));
    AvatarFootRightRig->SetRelativeLocation(FVector(0.0f, 0.0f, -P.CalfLengthCm));

    FWMEnvironmentMeshData Mesh;
    bool bAllPartsValid = true;

    FWMProceduralAvatarGeometry::BuildTorso(Mesh, P);
    bAllPartsValid &= WMAvatarArt::Commit(AvatarSpineRig, Mesh, AvatarSettings->Palette.Top);

    FWMProceduralAvatarGeometry::BuildHead(Mesh, P, Budget.HeadSegments);
    bAllPartsValid &= WMAvatarArt::Commit(AvatarHeadRig, Mesh, AvatarSettings->Palette.Skin);

    FWMProceduralAvatarGeometry::BuildHairCap(Mesh, P, Budget.HeadSegments);
    bAllPartsValid &= WMAvatarArt::Commit(AvatarHairArt, Mesh, AvatarSettings->Palette.Hair);

    FWMProceduralAvatarGeometry::BuildUpperArm(Mesh, P, Budget.FacetSides);
    bAllPartsValid &= WMAvatarArt::Commit(AvatarUpperArmLeftRig, Mesh, AvatarSettings->Palette.Top);
    bAllPartsValid &= WMAvatarArt::Commit(AvatarUpperArmRightRig, Mesh, AvatarSettings->Palette.Top);

    FWMProceduralAvatarGeometry::BuildLowerArm(Mesh, P, Budget.FacetSides);
    bAllPartsValid &= WMAvatarArt::Commit(AvatarLowerArmLeftRig, Mesh, AvatarSettings->Palette.Skin);
    bAllPartsValid &= WMAvatarArt::Commit(AvatarLowerArmRightRig, Mesh, AvatarSettings->Palette.Skin);

    FWMProceduralAvatarGeometry::BuildHand(Mesh, Budget.HeadSegments);
    bAllPartsValid &= WMAvatarArt::Commit(AvatarHandLeftRig, Mesh, AvatarSettings->Palette.Skin);
    bAllPartsValid &= WMAvatarArt::Commit(AvatarHandRightRig, Mesh, AvatarSettings->Palette.Skin);

    FWMProceduralAvatarGeometry::BuildThigh(Mesh, P, Budget.FacetSides);
    bAllPartsValid &= WMAvatarArt::Commit(AvatarThighLeftRig, Mesh, AvatarSettings->Palette.Bottom);
    bAllPartsValid &= WMAvatarArt::Commit(AvatarThighRightRig, Mesh, AvatarSettings->Palette.Bottom);

    FWMProceduralAvatarGeometry::BuildCalf(Mesh, P, Budget.FacetSides);
    bAllPartsValid &= WMAvatarArt::Commit(AvatarCalfLeftRig, Mesh, AvatarSettings->Palette.Bottom);
    bAllPartsValid &= WMAvatarArt::Commit(AvatarCalfRightRig, Mesh, AvatarSettings->Palette.Bottom);

    FWMProceduralAvatarGeometry::BuildFoot(Mesh, Budget.HeadSegments);
    bAllPartsValid &= WMAvatarArt::Commit(AvatarFootLeftRig, Mesh, AvatarSettings->Palette.Footwear);
    bAllPartsValid &= WMAvatarArt::Commit(AvatarFootRightRig, Mesh, AvatarSettings->Palette.Footwear);

    bProceduralAvatarReady = bAllPartsValid;
    RefreshAvatarVisualPath();
}

void AWMPlayerCharacter::SetLegacyAvatarVisible(const bool bVisible)
{
    for (UStaticMeshComponent* Part : TArray<UStaticMeshComponent*>{PrototypeBody, PrototypeHead, PrototypeLeftArm, PrototypeRightArm, PrototypeLeftLeg, PrototypeRightLeg})
    {
        if (Part)
        {
            Part->SetVisibility(bVisible, true);
            Part->SetHiddenInGame(!bVisible, true);
        }
    }
}

void AWMPlayerCharacter::SetProceduralAvatarVisible(const bool bVisible)
{
    if (AvatarRigRoot)
    {
        AvatarRigRoot->SetVisibility(bVisible, true);
        AvatarRigRoot->SetHiddenInGame(!bVisible, true);
    }
}

void AWMPlayerCharacter::RefreshAvatarVisualPath()
{
    const UWMAvatarVisualSettings* Settings = GetDefault<UWMAvatarVisualSettings>();
    const bool bHasProductionSkeletalMesh = GetMesh() && GetMesh()->GetSkeletalMeshAsset() != nullptr;
    const bool bPreferProcedural = !Settings || Settings->bUseProceduralAvatarArt;
    const bool bUseProcedural = bProceduralAvatarReady && (bPreferProcedural || !bHasProductionSkeletalMesh);
    const bool bUseProductionSkeletalMesh = !bUseProcedural && bHasProductionSkeletalMesh;
    const bool bUseLegacy = !bUseProcedural && !bUseProductionSkeletalMesh;

    bProceduralAvatarActive = bUseProcedural;
    SetProceduralAvatarVisible(bUseProcedural);
    SetLegacyAvatarVisible(bUseLegacy);

    if (GetMesh())
    {
        GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        GetMesh()->SetVisibility(bUseProductionSkeletalMesh, true);
        GetMesh()->SetHiddenInGame(!bUseProductionSkeletalMesh, true);
    }
}

void AWMPlayerCharacter::UpdatePrototypeMotion(const float DeltaSeconds)
{
    if (!GetCharacterMovement() || !FMath::IsFinite(DeltaSeconds) || DeltaSeconds <= 0.0f)
    {
        return;
    }

    const float MaxWalkSpeed = FMath::Max(GetCharacterMovement()->MaxWalkSpeed, 1.0f);
    const float SpeedAlpha = FMath::Clamp(GetVelocity().Size2D() / MaxWalkSpeed, 0.0f, 1.0f);
    const bool bAirborne = GetCharacterMovement()->IsFalling();

    if (!bAirborne && SpeedAlpha > 0.01f)
    {
        const float CyclesPerSecond = FMath::Lerp(1.35f, 2.25f, SpeedAlpha);
        PrototypeMotionPhase = FMath::Fmod(PrototypeMotionPhase + DeltaSeconds * CyclesPerSecond * 2.0f * PI, 2.0f * PI);
    }

    const FWMPrototypeMotionPose Pose = FWMPrototypeMotionStyle::Evaluate(SpeedAlpha, PrototypeMotionPhase, bAirborne);

    if (bProceduralAvatarActive && AvatarRigRoot && AvatarHeadRig && AvatarUpperArmLeftRig && AvatarUpperArmRightRig && AvatarThighLeftRig && AvatarThighRightRig)
    {
        const UWMAvatarVisualSettings* Settings = GetDefault<UWMAvatarVisualSettings>();
        const float HeadBaseZ = Settings ? Settings->Proportions.HeadHeightCm * 0.50f : 15.8f;
        AvatarRigRoot->SetRelativeLocation(FVector(0.0f, 0.0f, Pose.BodyBobCm));
        AvatarRigRoot->SetRelativeRotation(FRotator(Pose.BodyLeanDegrees, 0.0f, 0.0f));
        AvatarHeadRig->SetRelativeLocation(FVector(0.0f, 0.0f, HeadBaseZ + Pose.HeadBobCm));
        AvatarUpperArmLeftRig->SetRelativeRotation(FRotator(Pose.ArmSwingDegrees, 0.0f, -5.0f));
        AvatarUpperArmRightRig->SetRelativeRotation(FRotator(-Pose.ArmSwingDegrees, 0.0f, 5.0f));
        AvatarThighLeftRig->SetRelativeRotation(FRotator(Pose.LegSwingDegrees, 0.0f, 0.0f));
        AvatarThighRightRig->SetRelativeRotation(FRotator(-Pose.LegSwingDegrees, 0.0f, 0.0f));
        return;
    }

    if (!PrototypeBody || !PrototypeHead || !PrototypeLeftArm || !PrototypeRightArm || !PrototypeLeftLeg || !PrototypeRightLeg)
    {
        return;
    }

    PrototypeBody->SetRelativeLocation(FVector(0.0f, 0.0f, 12.0f + Pose.BodyBobCm));
    PrototypeBody->SetRelativeRotation(FRotator(Pose.BodyLeanDegrees, 0.0f, 0.0f));
    PrototypeHead->SetRelativeLocation(FVector(0.0f, 0.0f, 62.0f + Pose.HeadBobCm));
    PrototypeLeftArm->SetRelativeRotation(FRotator(Pose.ArmSwingDegrees, 0.0f, -7.0f));
    PrototypeRightArm->SetRelativeRotation(FRotator(-Pose.ArmSwingDegrees, 0.0f, 7.0f));
    PrototypeLeftLeg->SetRelativeRotation(FRotator(Pose.LegSwingDegrees, 0.0f, 0.0f));
    PrototypeRightLeg->SetRelativeRotation(FRotator(-Pose.LegSwingDegrees, 0.0f, 0.0f));
}

void AWMPlayerCharacter::EnsureBuildHUD()
{
    if (BuildHUD || !IsLocallyControlled() || !BuildingComponent || !MissionMeasurementComponent || !InteractionComponent)
    {
        return;
    }

    APlayerController* PlayerController = Cast<APlayerController>(GetController());
    if (!PlayerController)
    {
        return;
    }

    BuildHUD = CreateWidget<UWMBuildHUDWidget>(PlayerController, UWMBuildHUDWidget::StaticClass());
    if (!BuildHUD)
    {
        return;
    }

    BuildHUD->BindBuildingComponent(BuildingComponent);
    BuildHUD->BindMissionMeasurementComponent(MissionMeasurementComponent);
    BuildHUD->BindInteractionComponent(InteractionComponent);
    BuildHUD->AddToPlayerScreen(10);

    FInputModeGameAndUI InputMode;
    InputMode.SetHideCursorDuringCapture(false);
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    PlayerController->SetInputMode(InputMode);
}

void AWMPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    check(PlayerInputComponent);

    PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AWMPlayerCharacter::MoveForward);
    PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AWMPlayerCharacter::MoveRight);
    PlayerInputComponent->BindAxis(TEXT("Turn"), this, &AWMPlayerCharacter::Turn);
    PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &AWMPlayerCharacter::LookUp);

    PlayerInputComponent->BindAction(TEXT("PlaceBuild"), IE_Pressed, this, &AWMPlayerCharacter::PlaceBuild);
    PlayerInputComponent->BindAction(TEXT("RotateBuild"), IE_Pressed, this, &AWMPlayerCharacter::RotateBuildClockwise);
    PlayerInputComponent->BindAction(TEXT("RotateBuildBack"), IE_Pressed, this, &AWMPlayerCharacter::RotateBuildCounterClockwise);
    PlayerInputComponent->BindAction(TEXT("RemoveBuild"), IE_Pressed, this, &AWMPlayerCharacter::RemoveBuild);
    PlayerInputComponent->BindAction(TEXT("MoveBuild"), IE_Pressed, this, &AWMPlayerCharacter::MoveBuild);
    PlayerInputComponent->BindAction(TEXT("CycleBuildPiece"), IE_Pressed, this, &AWMPlayerCharacter::CycleBuildPiece);
    PlayerInputComponent->BindAction(TEXT("CancelBuildEdit"), IE_Pressed, this, &AWMPlayerCharacter::CancelBuildEdit);
    PlayerInputComponent->BindAction(TEXT("UndoBuild"), IE_Pressed, this, &AWMPlayerCharacter::UndoBuild);
    PlayerInputComponent->BindAction(TEXT("RedoBuild"), IE_Pressed, this, &AWMPlayerCharacter::RedoBuild);
    PlayerInputComponent->BindAction(TEXT("SaveWorld"), IE_Pressed, this, &AWMPlayerCharacter::SavePrototypeWorld);
    PlayerInputComponent->BindAction(TEXT("LoadWorld"), IE_Pressed, this, &AWMPlayerCharacter::LoadPrototypeWorld);
    PlayerInputComponent->BindAction(TEXT("MeasureMission"), IE_Pressed, this, &AWMPlayerCharacter::CaptureMissionMeasurementPoint);
    PlayerInputComponent->BindAction(TEXT("ResetMissionMeasurement"), IE_Pressed, this, &AWMPlayerCharacter::ResetMissionMeasurement);
    PlayerInputComponent->BindAction(TEXT("CycleMission"), IE_Pressed, this, &AWMPlayerCharacter::CycleMission);
    PlayerInputComponent->BindAction(TEXT("ObserveWorld"), IE_Pressed, this, &AWMPlayerCharacter::ObserveWorld);

    PlayerInputComponent->BindTouch(IE_Pressed, this, &AWMPlayerCharacter::HandleTouchPressed);
    PlayerInputComponent->BindTouch(IE_Repeat, this, &AWMPlayerCharacter::HandleTouchRepeat);
    PlayerInputComponent->BindTouch(IE_Released, this, &AWMPlayerCharacter::HandleTouchReleased);
}

void AWMPlayerCharacter::MoveForward(const float Value)
{
    if (!Controller || FMath::IsNearlyZero(Value)) return;
    const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
    AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), Value);
}

void AWMPlayerCharacter::MoveRight(const float Value)
{
    if (!Controller || FMath::IsNearlyZero(Value)) return;
    const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
    AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), Value);
}

void AWMPlayerCharacter::Turn(const float Value) { AddControllerYawInput(Value); }
void AWMPlayerCharacter::LookUp(const float Value) { AddControllerPitchInput(Value); }
void AWMPlayerCharacter::PlaceBuild() { if (BuildingComponent) BuildingComponent->TryPlaceCurrentPiece(); }
void AWMPlayerCharacter::RotateBuildClockwise() { if (BuildingComponent) BuildingComponent->RotatePreview(1.0f); }
void AWMPlayerCharacter::RotateBuildCounterClockwise() { if (BuildingComponent) BuildingComponent->RotatePreview(-1.0f); }
void AWMPlayerCharacter::RemoveBuild() { if (BuildingComponent) BuildingComponent->TryRemoveTargetPiece(); }
void AWMPlayerCharacter::MoveBuild() { if (BuildingComponent) BuildingComponent->TryBeginMoveTargetPiece(); }
void AWMPlayerCharacter::CycleBuildPiece() { if (BuildingComponent) BuildingComponent->CycleSelectedPiece(1); }
void AWMPlayerCharacter::CancelBuildEdit() { if (BuildingComponent) BuildingComponent->CancelMove(); }
void AWMPlayerCharacter::UndoBuild() { if (BuildingComponent) BuildingComponent->UndoLastAction(); }
void AWMPlayerCharacter::RedoBuild() { if (BuildingComponent) BuildingComponent->RedoLastAction(); }
void AWMPlayerCharacter::SavePrototypeWorld() { if (BuildingComponent) BuildingComponent->SaveWorld(); }
void AWMPlayerCharacter::LoadPrototypeWorld() { if (BuildingComponent) BuildingComponent->LoadWorld(); }
void AWMPlayerCharacter::CaptureMissionMeasurementPoint() { if (MissionMeasurementComponent) MissionMeasurementComponent->CapturePointFromView(); }
void AWMPlayerCharacter::ResetMissionMeasurement() { if (MissionMeasurementComponent) MissionMeasurementComponent->ResetMeasurement(); }
void AWMPlayerCharacter::ObserveWorld() { if (InteractionComponent) InteractionComponent->TryInteractFocused(); }

void AWMPlayerCharacter::CycleMission()
{
    if (MissionMeasurementComponent)
    {
        MissionMeasurementComponent->ResetMeasurement();
    }
    if (UWorld* World = GetWorld())
    {
        if (UWMMissionRuntimeSubsystem* Missions = World->GetSubsystem<UWMMissionRuntimeSubsystem>())
        {
            Missions->CycleMission(1);
        }
    }
}

void AWMPlayerCharacter::HandleTouchPressed(const ETouchIndex::Type FingerIndex, const FVector Location)
{
    if (bTouchTracking) return;
    bTouchTracking = true;
    bTouchDragging = false;
    ActiveTouchFinger = FingerIndex;
    TouchStartScreen = FVector2D(Location.X, Location.Y);
    TouchLastScreen = TouchStartScreen;
}

void AWMPlayerCharacter::HandleTouchRepeat(const ETouchIndex::Type FingerIndex, const FVector Location)
{
    if (!bTouchTracking || FingerIndex != ActiveTouchFinger) return;

    const FVector2D CurrentScreen(Location.X, Location.Y);
    if (!bTouchDragging && UWMTouchGestureLibrary::HasExceededDragDeadZone(TouchStartScreen, CurrentScreen, TouchDragDeadZonePx))
    {
        bTouchDragging = true;
    }

    if (bTouchDragging)
    {
        FVector2D Delta = CurrentScreen - TouchLastScreen;
        constexpr float MaxTouchDeltaPx = 96.0f;
        if (Delta.SizeSquared() > FMath::Square(MaxTouchDeltaPx))
        {
            Delta = Delta.GetSafeNormal() * MaxTouchDeltaPx;
        }
        AddControllerYawInput(Delta.X * TouchLookDegreesPerPixel);
        AddControllerPitchInput(-Delta.Y * TouchLookDegreesPerPixel);
    }
    TouchLastScreen = CurrentScreen;
}

void AWMPlayerCharacter::HandleTouchReleased(const ETouchIndex::Type FingerIndex, const FVector Location)
{
    if (!bTouchTracking || FingerIndex != ActiveTouchFinger) return;
    bTouchTracking = false;
    bTouchDragging = false;
    TouchStartScreen = FVector2D::ZeroVector;
    TouchLastScreen = FVector2D::ZeroVector;
}
