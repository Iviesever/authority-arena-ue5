#include "World/AuthorityArenaWorldBuilder.h"

#include "Components/SceneComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
UStaticMesh* LoadCubeMesh()
{
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
        TEXT("/Engine/BasicShapes/Cube.Cube"));
    return CubeMesh.Succeeded() ? CubeMesh.Object : nullptr;
}
} // namespace

AAuthorityArenaWorldBuilder::AAuthorityArenaWorldBuilder()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    bAlwaysRelevant = true;
    SetReplicateMovement(false);

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    Floor = CreateArenaBlock(TEXT("Floor"), FVector(0.0, 0.0, -50.0), FVector(24.0, 18.0, 1.0), true);
    NorthWall = CreateArenaBlock(TEXT("NorthWall"), FVector(0.0, 950.0, 150.0), FVector(24.0, 1.0, 4.0), true);
    SouthWall = CreateArenaBlock(TEXT("SouthWall"), FVector(0.0, -950.0, 150.0), FVector(24.0, 1.0, 4.0), true);
    EastWall = CreateArenaBlock(TEXT("EastWall"), FVector(1250.0, 0.0, 150.0), FVector(1.0, 18.0, 4.0), true);
    WestWall = CreateArenaBlock(TEXT("WestWall"), FVector(-1250.0, 0.0, 150.0), FVector(1.0, 18.0, 4.0), true);
    CenterMarker = CreateArenaBlock(TEXT("CenterMarker"), FVector(0.0, 0.0, 5.0), FVector(0.15, 18.0, 0.1), false);

    DirectionalLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("DirectionalLight"));
    DirectionalLight->SetupAttachment(SceneRoot);
    DirectionalLight->SetRelativeRotation(FRotator(-55.0f, -35.0f, 0.0f));
    DirectionalLight->SetIntensity(6.0f);
    DirectionalLight->SetLightColor(FLinearColor(1.0f, 0.92f, 0.80f));

    SkyLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("SkyLight"));
    SkyLight->SetupAttachment(SceneRoot);
    SkyLight->SetMobility(EComponentMobility::Movable);
    SkyLight->SetIntensity(1.2f);
    SkyLight->bLowerHemisphereIsBlack = false;

    FillLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("FillLight"));
    FillLight->SetupAttachment(SceneRoot);
    FillLight->SetRelativeLocation(FVector(0.0f, 0.0f, 700.0f));
    FillLight->SetIntensity(45000.0f);
    FillLight->SetAttenuationRadius(3200.0f);
    FillLight->SetLightColor(FLinearColor(0.42f, 0.62f, 1.0f));
    FillLight->SetCastShadows(false);
}

UStaticMeshComponent* AAuthorityArenaWorldBuilder::CreateArenaBlock(
    const FName Name,
    const FVector& RelativeLocation,
    const FVector& RelativeScale,
    const bool bCollisionEnabled)
{
    UStaticMeshComponent* Component = CreateDefaultSubobject<UStaticMeshComponent>(Name);
    Component->SetupAttachment(SceneRoot);
    Component->SetStaticMesh(LoadCubeMesh());
    Component->SetRelativeLocation(RelativeLocation);
    Component->SetRelativeScale3D(RelativeScale);
    Component->SetCollisionEnabled(
        bCollisionEnabled ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
    Component->SetCollisionProfileName(
        bCollisionEnabled ? UCollisionProfile::BlockAll_ProfileName : UCollisionProfile::NoCollision_ProfileName);
    return Component;
}
