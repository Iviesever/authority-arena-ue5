#include "Ability/GA_Shield.h"

#include "Ability/AuthorityArenaEffects.h"
#include "Ability/AuthorityArenaGameplayTags.h"
#include "Diagnostics/AuthorityArenaNetworkDiagnosticsSubsystem.h"

UGA_Shield::UGA_Shield()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
    CostGameplayEffectClass = UAuthorityArenaGE_ShieldCost::StaticClass();
    CooldownGameplayEffectClass = UAuthorityArenaGE_ShieldCooldown::StaticClass();

    FGameplayTagContainer Tags;
    Tags.AddTag(AuthorityArenaTags::Ability_Shield);
    SetAssetTags(Tags);
    ActivationBlockedTags.AddTag(AuthorityArenaTags::State_Dead);
    ActivationBlockedTags.AddTag(AuthorityArenaTags::State_Stunned);
}

void UGA_Shield::ActivateAbility(
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

    ApplyGameplayEffectToOwner(
        Handle,
        ActorInfo,
        ActivationInfo,
        GetDefault<UAuthorityArenaGE_ShieldState>(),
        GetAbilityLevel(Handle, ActorInfo));
    UAuthorityArenaNetworkDiagnosticsSubsystem::EmitEvent(
        ActorInfo->AvatarActor.Get(),
        IsPredictingClient() ? TEXT("ShieldPredicted") : TEXT("ShieldConfirmed"));
    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
