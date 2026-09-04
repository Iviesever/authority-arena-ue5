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

    static bool IsAuthorityTargetEligible(
        const FVector& Origin,
        const FVector& Forward,
        const FVector& Target,
        bool bTargetAlive);

    virtual bool CanActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayTagContainer* SourceTags = nullptr,
        const FGameplayTagContainer* TargetTags = nullptr,
        FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;
};
