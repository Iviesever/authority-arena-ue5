#include "Ability/GA_ProjectileAttack.h"

#include "Ability/AuthorityArenaEffects.h"
#include "Ability/AuthorityArenaGameplayTags.h"
#include "Character/AuthorityArenaCharacter.h"
#include "Combat/AuthorityArenaCombatComponent.h"
#include "Diagnostics/AuthorityArenaNetworkDiagnosticsSubsystem.h"

UGA_ProjectileAttack::UGA_ProjectileAttack()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
    CostGameplayEffectClass = UAuthorityArenaGE_AttackCost::StaticClass();
    CooldownGameplayEffectClass = UAuthorityArenaGE_AttackCooldown::StaticClass();

    FGameplayTagContainer Tags;
    Tags.AddTag(AuthorityArenaTags::Ability_Attack);
    SetAssetTags(Tags);
    ActivationBlockedTags.AddTag(AuthorityArenaTags::State_Dead);
    ActivationBlockedTags.AddTag(AuthorityArenaTags::State_Stunned);
}

void UGA_ProjectileAttack::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    if (ActorInfo == nullptr || ActorInfo->AvatarActor.Get() == nullptr ||
        !CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    AAuthorityArenaCharacter* Character = Cast<AAuthorityArenaCharacter>(ActorInfo->AvatarActor.Get());
    if (Character == nullptr)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    UAuthorityArenaNetworkDiagnosticsSubsystem::EmitEvent(
        Character,
        IsPredictingClient() ? TEXT("AttackPredicted") : TEXT("AttackConfirmed"));

    if (HasAuthority(&ActivationInfo))
    {
        if (UAuthorityArenaCombatComponent* Combat =
                Character->FindComponentByClass<UAuthorityArenaCombatComponent>())
        {
            Combat->SpawnProjectileAuthority(Character->GetActorForwardVector());
        }
    }
    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
