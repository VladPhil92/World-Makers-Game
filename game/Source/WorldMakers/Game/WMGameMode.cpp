#include "Game/WMGameMode.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Kismet/GameplayStatics.h"
#include "Player/WMPlayerCharacter.h"

namespace
{
    const FName PrototypeGroundTag(TEXT("WM_PrototypeGround"));
}

AWMGameMode::AWMGameMode()
{
    DefaultPawnClass = AWMPlayerCharacter::StaticClass();
}

void AWMGameMode::BeginPlay()
{
    Super::BeginPlay();
    EnsurePrototypeGround();
}

void AWMGameMode::EnsurePrototypeGround()
{
    if (!bSpawnPrototypeGround || !GetWorld())
    {
        return;
    }

    TArray<AActor*> ExistingGround;
    UGameplayStatics::GetAllActorsWithTag(this, PrototypeGroundTag, ExistingGround);
    if (!ExistingGround.IsEmpty())
    {
        return;
    }

    UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (!CubeMesh)
    {
        return;
    }

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AStaticMeshActor* Ground = GetWorld()->SpawnActor<AStaticMeshActor>(
        AStaticMeshActor::StaticClass(),
        FVector(0.0f, 0.0f, -100.0f),
        FRotator::ZeroRotator,
        SpawnParameters);

    if (!Ground)
    {
        return;
    }

    Ground->Tags.Add(PrototypeGroundTag);
    Ground->SetActorScale3D(FVector(30.0f, 30.0f, 1.0f));
    Ground->GetStaticMeshComponent()->SetStaticMesh(CubeMesh);
    Ground->GetStaticMeshComponent()->SetCollisionProfileName(TEXT("BlockAll"));
}
