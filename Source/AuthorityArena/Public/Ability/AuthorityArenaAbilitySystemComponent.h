#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AuthorityArenaAbilitySystemComponent.generated.h"

UCLASS()
class AUTHORITYARENA_API UAuthorityArenaAbilitySystemComponent : public UAbilitySystemComponent
{
    GENERATED_BODY()

public:
    UAuthorityArenaAbilitySystemComponent();

    EGameplayEffectReplicationMode GetConfiguredReplicationMode() const { return ReplicationMode; }
    const TArray<TSubclassOf<UGameplayAbility>>& GetDefaultAbilityClasses() const { return DefaultAbilityClasses; }

    void GrantDefaultAbilities();
    bool TryActivateAbilityByTag(FGameplayTag AbilityTag);
    void ArmNextDashRejectionAuthority();
    bool ConsumeNextDashRejectionAuthority();
    bool IsDashRejectionArmed() const { return bRejectNextDashAuthority; }

protected:
    virtual void NotifyAbilityFailed(
        const FGameplayAbilitySpecHandle Handle,
        UGameplayAbility* Ability,
        const FGameplayTagContainer& FailureReason) override;

private:
    UPROPERTY()
    TArray<TSubclassOf<UGameplayAbility>> DefaultAbilityClasses;

    bool bRejectNextDashAuthority = false;
};
