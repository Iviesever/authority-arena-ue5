#include "Ability/AuthorityArenaAbilitySystemComponent.h"

#include "Ability/GA_Dash.h"
#include "Ability/GA_ProjectileAttack.h"
#include "Ability/GA_Shield.h"
#include "Diagnostics/AuthorityArenaNetworkDiagnosticsSubsystem.h"

UAuthorityArenaAbilitySystemComponent::UAuthorityArenaAbilitySystemComponent()
{
    SetIsReplicatedByDefault(true);
    SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
    DefaultAbilityClasses = {
        UGA_Dash::StaticClass(),
        UGA_ProjectileAttack::StaticClass(),
        UGA_Shield::StaticClass(),
    };
}

void UAuthorityArenaAbilitySystemComponent::GrantDefaultAbilities()
{
    if (!IsOwnerActorAuthoritative())
    {
        return;
    }
    for (const TSubclassOf<UGameplayAbility> AbilityClass : DefaultAbilityClasses)
    {
        if (AbilityClass != nullptr && FindAbilitySpecFromClass(AbilityClass) == nullptr)
        {
            GiveAbility(FGameplayAbilitySpec(AbilityClass, 1));
        }
    }
}

bool UAuthorityArenaAbilitySystemComponent::TryActivateAbilityByTag(const FGameplayTag AbilityTag)
{
    FGameplayTagContainer Tags;
    Tags.AddTag(AbilityTag);
    return TryActivateAbilitiesByTag(Tags, true);
}

void UAuthorityArenaAbilitySystemComponent::ArmNextDashRejectionAuthority()
{
    if (IsOwnerActorAuthoritative())
    {
        bRejectNextDashAuthority = true;
    }
}

bool UAuthorityArenaAbilitySystemComponent::ConsumeNextDashRejectionAuthority()
{
    if (!IsOwnerActorAuthoritative() || !bRejectNextDashAuthority)
    {
        return false;
    }
    bRejectNextDashAuthority = false;
    return true;
}

void UAuthorityArenaAbilitySystemComponent::NotifyAbilityFailed(
    const FGameplayAbilitySpecHandle Handle,
    UGameplayAbility* Ability,
    const FGameplayTagContainer& FailureReason)
{
    Super::NotifyAbilityFailed(Handle, Ability, FailureReason);
    UAuthorityArenaNetworkDiagnosticsSubsystem::EmitEvent(
        this,
        TEXT("AbilityRejected"),
        FString::Printf(
            TEXT("ability=%s reasons=%s"),
            *GetNameSafe(Ability),
            *FailureReason.ToStringSimple()));
}
