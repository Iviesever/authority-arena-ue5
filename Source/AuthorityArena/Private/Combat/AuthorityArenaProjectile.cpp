#include "Combat/AuthorityArenaProjectile.h"

#include "Ability/AuthorityArenaEffects.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Character/AuthorityArenaCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Diagnostics/AuthorityArenaNetworkDiagnosticsSubsystem.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "UObject/ConstructorHelpers.h"

AAuthorityArenaProjectile::AAuthorityArenaProjectile()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    SetReplicateMovement(true);
    SetLifeSpan(3.0f);

    Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
    SetRootComponent(Collision);
    Collision->InitSphereRadius(22.0f);
    Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Collision->SetCollisionObjectType(ECC_WorldDynamic);
    Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
    Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    Collision->SetGenerateOverlapEvents(true);
    Collision->OnComponentBeginOverlap.AddDynamic(this, &AAuthorityArenaProjectile::OnSphereOverlap);

    Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
    Visual->SetupAttachment(Collision);
    Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Visual->SetRelativeScale3D(FVector(0.4f));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(
        TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (SphereMesh.Succeeded())
    {
        Visual->SetStaticMesh(SphereMesh.Object);
    }

    Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
    Movement->UpdatedComponent = Collision;
    Movement->InitialSpeed = 1400.0f;
    Movement->MaxSpeed = 1400.0f;
    Movement->ProjectileGravityScale = 0.0f;
    Movement->bRotationFollowsVelocity = true;
    Movement->bInitialVelocityInLocalSpace = false;
}

void AAuthorityArenaProjectile::InitializeVelocity(const FVector& Direction)
{
    if (HasAuthority() && Movement != nullptr)
    {
        Movement->Velocity = Direction.GetSafeNormal() * Movement->InitialSpeed;
    }
}

void AAuthorityArenaProjectile::MulticastImpact_Implementation(const FVector_NetQuantize ImpactLocation)
{
    UAuthorityArenaNetworkDiagnosticsSubsystem::EmitEvent(
        this,
        TEXT("ProjectileImpact"),
        FString::Printf(TEXT("x=%.2f y=%.2f z=%.2f"), ImpactLocation.X, ImpactLocation.Y, ImpactLocation.Z));
}

void AAuthorityArenaProjectile::OnSphereOverlap(
    UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComponent,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{
    if (!HasAuthority() || OtherActor == nullptr || OtherActor == GetOwner())
    {
        return;
    }
    AAuthorityArenaCharacter* TargetCharacter = Cast<AAuthorityArenaCharacter>(OtherActor);
    if (TargetCharacter == nullptr)
    {
        return;
    }

    UAbilitySystemComponent* SourceAbilitySystem =
        UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
    UAbilitySystemComponent* TargetAbilitySystem =
        UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetCharacter);
    if (SourceAbilitySystem == nullptr || TargetAbilitySystem == nullptr)
    {
        return;
    }

    FGameplayEffectContextHandle Context = SourceAbilitySystem->MakeEffectContext();
    Context.AddInstigator(GetInstigator(), GetInstigator());
    Context.AddSourceObject(this);
    const FGameplayEffectSpecHandle Spec = SourceAbilitySystem->MakeOutgoingSpec(
        UAuthorityArenaGE_ProjectileDamage::StaticClass(),
        1.0f,
        Context);
    if (!Spec.IsValid())
    {
        return;
    }
    SourceAbilitySystem->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetAbilitySystem);
    const FVector ImpactLocation = SweepResult.bBlockingHit
        ? FVector(SweepResult.ImpactPoint)
        : GetActorLocation();
    MulticastImpact(FVector_NetQuantize(ImpactLocation));
    Destroy();
}
