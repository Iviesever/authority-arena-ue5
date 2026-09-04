#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GA_Shield.generated.h"

UCLASS()
class AUTHORITYARENA_API UGA_Shield : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UGA_Shield();

    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;
};
