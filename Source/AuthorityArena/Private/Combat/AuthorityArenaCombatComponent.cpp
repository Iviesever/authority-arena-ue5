#include "Combat/AuthorityArenaCombatComponent.h"

#include "Character/AuthorityArenaCharacter.h"
#include "Combat/AuthorityArenaProjectile.h"
#include "Diagnostics/AuthorityArenaNetworkDiagnosticsSubsystem.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

UAuthorityArenaCombatComponent::UAuthorityArenaCombatComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(false);
}

bool UAuthorityArenaCombatComponent::SpawnProjectileAuthority(const FVector& RequestedDirection)
{
    AAuthorityArenaCharacter* Character = Cast<AAuthorityArenaCharacter>(GetOwner());
    if (Character == nullptr || !Character->HasAuthority() || RequestedDirection.ContainsNaN())
    {
        return false;
    }

    const FVector Direction = RequestedDirection.GetSafeNormal();
    if (Direction.IsNearlyZero() || GetWorld() == nullptr)
    {
        return false;
    }

    const FVector SpawnLocation = Character->GetActorLocation() + Direction * 55.0f + FVector(0.0f, 0.0f, 35.0f);
    const FTransform SpawnTransform(Direction.Rotation(), SpawnLocation);
    AAuthorityArenaProjectile* Projectile = GetWorld()->SpawnActorDeferred<AAuthorityArenaProjectile>(
        AAuthorityArenaProjectile::StaticClass(),
        SpawnTransform,
        Character,
        Character,
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if (Projectile == nullptr)
    {
        return false;
    }
    Projectile->InitializeVelocity(Direction);
    UAuthorityArenaNetworkDiagnosticsSubsystem::EmitEvent(
        this,
        TEXT("ProjectileSpawned"),
        FString::Printf(TEXT("owner=%s projectile=%s"), *Character->GetName(), *Projectile->GetName()));
    UGameplayStatics::FinishSpawningActor(Projectile, SpawnTransform);
    return true;
}
