#include "Ability/GA_ProjectileAttack.h"

#include "Ability/AuthorityArenaAbilitySystemComponent.h"
#include "Ability/AuthorityArenaEffects.h"
#include "Ability/AuthorityArenaGameplayTags.h"
#include "Character/AuthorityArenaCharacter.h"
#include "Combat/AuthorityArenaCombatComponent.h"
#include "Diagnostics/AuthorityArenaNetworkDiagnosticsSubsystem.h"
#include "EngineUtils.h"
#include "Player/AuthorityArenaPlayerState.h"

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

bool UGA_ProjectileAttack::CanActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayTagContainer* SourceTags,
    const FGameplayTagContainer* TargetTags,
    FGameplayTagContainer* OptionalRelevantTags) const
{
    if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
    {
        return false;
    }
    UAuthorityArenaAbilitySystemComponent* AbilitySystem = ActorInfo != nullptr
        ? Cast<UAuthorityArenaAbilitySystemComponent>(ActorInfo->AbilitySystemComponent.Get())
        : nullptr;
    if (AbilitySystem == nullptr || !AbilitySystem->IsOwnerActorAuthoritative())
    {
        return true;
    }

    const AAuthorityArenaCharacter* Character = ActorInfo != nullptr
        ? Cast<AAuthorityArenaCharacter>(ActorInfo->AvatarActor.Get())
        : nullptr;
    bool bHasEligibleTarget = false;
    if (Character != nullptr && Character->GetWorld() != nullptr)
    {
        for (TActorIterator<AAuthorityArenaCharacter> It(Character->GetWorld()); It; ++It)
        {
            const AAuthorityArenaCharacter* Candidate = *It;
            const UAuthorityArenaAbilitySystemComponent* CandidateAbilitySystem =
                Candidate != nullptr
                    ? Cast<UAuthorityArenaAbilitySystemComponent>(Candidate->GetAbilitySystemComponent())
                    : nullptr;
            if (Candidate != Character && CandidateAbilitySystem != nullptr &&
                IsAuthorityTargetEligible(
                    Character->GetActorLocation(),
                    Character->GetActorForwardVector(),
                    Candidate->GetActorLocation(),
                    !CandidateAbilitySystem->HasMatchingGameplayTag(AuthorityArenaTags::State_Dead)))
            {
                bHasEligibleTarget = true;
                break;
            }
        }
    }
    if (bHasEligibleTarget)
    {
        return true;
    }
    if (OptionalRelevantTags != nullptr)
    {
        OptionalRelevantTags->AddTag(AuthorityArenaTags::Failure_Target);
    }
    return false;
}

bool UGA_ProjectileAttack::IsAuthorityTargetEligible(
    const FVector& Origin,
    const FVector& Forward,
    const FVector& Target,
    const bool bTargetAlive)
{
    if (!bTargetAlive || Origin.ContainsNaN() || Forward.ContainsNaN() || Target.ContainsNaN())
    {
        return false;
    }
    const FVector Direction = Forward.GetSafeNormal();
    const FVector Offset = Target - Origin;
    if (Direction.IsNearlyZero() || Offset.IsNearlyZero() || Offset.SizeSquared() > 1'000'000.0)
    {
        return false;
    }
    return FVector::DotProduct(Direction, Offset.GetSafeNormal()) >= 0.8f;
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
