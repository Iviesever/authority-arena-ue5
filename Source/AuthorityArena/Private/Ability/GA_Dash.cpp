#include "Ability/GA_Dash.h"

#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"
#include "Ability/AuthorityArenaAbilitySystemComponent.h"
#include "Ability/AuthorityArenaAttributeSet.h"
#include "Ability/AuthorityArenaEffects.h"
#include "Ability/AuthorityArenaGameplayTags.h"
#include "Character/AuthorityArenaCharacter.h"
#include "Diagnostics/AuthorityArenaNetworkDiagnosticsSubsystem.h"
#include "GameFramework/CharacterMovementComponent.h"

UGA_Dash::UGA_Dash()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
    CostGameplayEffectClass = UAuthorityArenaGE_DashCost::StaticClass();
    CooldownGameplayEffectClass = UAuthorityArenaGE_DashCooldown::StaticClass();

    FGameplayTagContainer Tags;
    Tags.AddTag(AuthorityArenaTags::Ability_Dash);
    SetAssetTags(Tags);
    ActivationBlockedTags.AddTag(AuthorityArenaTags::State_Dead);
    ActivationBlockedTags.AddTag(AuthorityArenaTags::State_Stunned);
    ActivationBlockedTags.AddTag(AuthorityArenaTags::State_Shield_Active);
}

bool UGA_Dash::CanActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayTagContainer* SourceTags,
    const FGameplayTagContainer* TargetTags,
    FGameplayTagContainer* OptionalRelevantTags) const
{
    UAuthorityArenaAbilitySystemComponent* AbilitySystem = ActorInfo != nullptr
        ? Cast<UAuthorityArenaAbilitySystemComponent>(ActorInfo->AbilitySystemComponent.Get())
        : nullptr;
    if (AbilitySystem != nullptr && AbilitySystem->ConsumeNextDashRejectionAuthority())
    {
        AbilitySystem->SetNumericAttributeBase(UAuthorityArenaAttributeSet::GetEnergyAttribute(), 0.0f);
        Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
        if (OptionalRelevantTags != nullptr)
        {
            OptionalRelevantTags->AddTag(AuthorityArenaTags::Failure_Resource);
        }
        return false;
    }
    return Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

void UGA_Dash::ActivateAbility(
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

    FVector Direction = Character->GetLastMovementInputVector().GetSafeNormal();
    if (Direction.IsNearlyZero())
    {
        Direction = Character->GetActorForwardVector().GetSafeNormal();
    }

    UAuthorityArenaNetworkDiagnosticsSubsystem::EmitEvent(
        Character,
        IsPredictingClient() ? TEXT("DashPredicted") : TEXT("DashConfirmed"),
        FString::Printf(TEXT("prediction=%s"), *ActivationInfo.GetActivationPredictionKey().ToString()));

    UAbilityTask_ApplyRootMotionConstantForce* DashTask =
        UAbilityTask_ApplyRootMotionConstantForce::ApplyRootMotionConstantForce(
            this,
            TEXT("DashRootMotion"),
            Direction,
            2600.0f,
            0.22f,
            false,
            nullptr,
            ERootMotionFinishVelocityMode::SetVelocity,
            FVector::ZeroVector,
            0.0f,
            false);
    DashTask->OnFinish.AddDynamic(this, &UGA_Dash::OnDashFinished);
    DashTask->ReadyForActivation();
}

void UGA_Dash::OnDashFinished()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
