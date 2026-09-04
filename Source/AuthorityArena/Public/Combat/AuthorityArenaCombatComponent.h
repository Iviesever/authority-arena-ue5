#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AuthorityArenaCombatComponent.generated.h"

UCLASS(ClassGroup = "AuthorityArena", NotBlueprintable)
class AUTHORITYARENA_API UAuthorityArenaCombatComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAuthorityArenaCombatComponent();

    bool SpawnProjectileAuthority(const FVector& RequestedDirection);
};
