#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_ProjectileAttack.generated.h"

UCLASS()
class AUTHORITYARENA_API UGA_ProjectileAttack : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UGA_ProjectileAttack();

    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;
};
