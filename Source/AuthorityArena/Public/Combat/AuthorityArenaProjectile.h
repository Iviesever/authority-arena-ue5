#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AuthorityArenaProjectile.generated.h"

class UProjectileMovementComponent;
class USphereComponent;
class UStaticMeshComponent;

UCLASS(NotBlueprintable)
class AUTHORITYARENA_API AAuthorityArenaProjectile : public AActor
{
    GENERATED_BODY()

public:
    AAuthorityArenaProjectile();

    void InitializeVelocity(const FVector& Direction);

    UFUNCTION(NetMulticast, Unreliable)
    void MulticastImpact(FVector_NetQuantize ImpactLocation);

private:
    UFUNCTION()
    void OnSphereOverlap(
        UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComponent,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult);

    UPROPERTY(VisibleAnywhere, Category = "Projectile")
    TObjectPtr<USphereComponent> Collision;

    UPROPERTY(VisibleAnywhere, Category = "Projectile")
    TObjectPtr<UStaticMeshComponent> Visual;

    UPROPERTY(VisibleAnywhere, Category = "Projectile")
    TObjectPtr<UProjectileMovementComponent> Movement;
};
