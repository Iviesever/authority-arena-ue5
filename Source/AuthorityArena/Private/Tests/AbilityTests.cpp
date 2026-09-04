#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Ability/AuthorityArenaAbilitySystemComponent.h"
#include "Ability/AuthorityArenaAttributeSet.h"
#include "Ability/AuthorityArenaEffects.h"
#include "Ability/AuthorityArenaGameplayTags.h"
#include "Ability/GA_Dash.h"
#include "Ability/GA_ProjectileAttack.h"
#include "Ability/GA_Shield.h"
#include "Character/AuthorityArenaCharacter.h"
#include "Combat/AuthorityArenaCombatComponent.h"
#include "Combat/AuthorityArenaHealthComponent.h"
#include "Combat/AuthorityArenaProjectile.h"
#include "Player/AuthorityArenaPlayerState.h"
#include "UObject/UnrealType.h"

#include <limits>

namespace
{
bool IsAbilityRepNotifyProperty(const UClass* Class, const FName PropertyName)
{
    const FProperty* Property = FindFProperty<FProperty>(Class, PropertyName);
    return Property != nullptr &&
        Property->HasAnyPropertyFlags(CPF_Net) &&
        Property->RepNotifyFunc != NAME_None;
}
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAuthorityArenaGasContractTest,
    "AuthorityArena.GAS.Contract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAuthorityArenaGasContractTest::RunTest(const FString& Parameters)
{
    const AAuthorityArenaPlayerState* PlayerState = GetDefault<AAuthorityArenaPlayerState>();
    const UAuthorityArenaAbilitySystemComponent* AbilitySystem = PlayerState->GetAuthorityAbilitySystem();
    TestNotNull(TEXT("PlayerState owns ASC"), AbilitySystem);
    TestTrue(TEXT("ASC replicates"), AbilitySystem != nullptr && AbilitySystem->GetIsReplicated());
    TestEqual(
        TEXT("ASC uses Mixed replication"),
        AbilitySystem->GetConfiguredReplicationMode(),
        EGameplayEffectReplicationMode::Mixed);
    TestFalse(TEXT("Server dash rejection gate is off by default"),
        AbilitySystem->IsDashRejectionArmed());
    TestNotNull(TEXT("PlayerState owns AttributeSet"), PlayerState->GetAuthorityAttributeSet());
    TestEqual(TEXT("Three native abilities are configured"), AbilitySystem->GetDefaultAbilityClasses().Num(), 3);

    const UAuthorityArenaAttributeSet* Attributes = GetDefault<UAuthorityArenaAttributeSet>();
    TestEqual(TEXT("Default Health"), Attributes->GetHealth(), 100.0f);
    TestEqual(TEXT("Default MaxHealth"), Attributes->GetMaxHealth(), 100.0f);
    TestEqual(TEXT("Default Energy"), Attributes->GetEnergy(), 100.0f);
    TestEqual(TEXT("Default MaxEnergy"), Attributes->GetMaxEnergy(), 100.0f);
    TestTrue(TEXT("Health is RepNotify"), IsAbilityRepNotifyProperty(
        UAuthorityArenaAttributeSet::StaticClass(), TEXT("Health")));
    TestTrue(TEXT("Energy is RepNotify"), IsAbilityRepNotifyProperty(
        UAuthorityArenaAttributeSet::StaticClass(), TEXT("Energy")));
    TestEqual(TEXT("Unshielded damage is unchanged"),
        UAuthorityArenaAttributeSet::ComputeMitigatedDamage(34.0f, false), 34.0f);
    TestEqual(TEXT("Shield halves damage"),
        UAuthorityArenaAttributeSet::ComputeMitigatedDamage(34.0f, true), 17.0f);
    TestEqual(TEXT("Negative damage fails closed"),
        UAuthorityArenaAttributeSet::ComputeMitigatedDamage(-5.0f, false), 0.0f);
    TestEqual(TEXT("NaN damage fails closed"),
        UAuthorityArenaAttributeSet::ComputeMitigatedDamage(
            std::numeric_limits<float>::quiet_NaN(), false), 0.0f);

    const UGA_Dash* Dash = GetDefault<UGA_Dash>();
    TestEqual(TEXT("Dash is LocalPredicted"), Dash->GetNetExecutionPolicy(), EGameplayAbilityNetExecutionPolicy::LocalPredicted);
    TestEqual(TEXT("Dash is instanced per actor"), Dash->GetInstancingPolicy(), EGameplayAbilityInstancingPolicy::InstancedPerActor);
    TestNotNull(TEXT("Dash has GameplayEffect cost"), Dash->GetCostGameplayEffect());
    TestNotNull(TEXT("Dash has GameplayEffect cooldown"), Dash->GetCooldownGameplayEffect());
    TestTrue(TEXT("Dash has native asset tag"), Dash->GetAssetTags().HasTagExact(AuthorityArenaTags::Ability_Dash));

    const UGA_ProjectileAttack* Attack = GetDefault<UGA_ProjectileAttack>();
    TestEqual(TEXT("Attack accepts predicted client activation"), Attack->GetNetExecutionPolicy(), EGameplayAbilityNetExecutionPolicy::LocalPredicted);
    TestNotNull(TEXT("Attack has cooldown GameplayEffect"), Attack->GetCooldownGameplayEffect());
    TestTrue(TEXT("Attack has native asset tag"), Attack->GetAssetTags().HasTagExact(AuthorityArenaTags::Ability_Attack));
    TestTrue(TEXT("Attack target rejection has a stable native tag"),
        AuthorityArenaTags::Failure_Target.GetTag().IsValid());
    TestTrue(TEXT("Attack accepts a live forward target in range"),
        UGA_ProjectileAttack::IsAuthorityTargetEligible(
            FVector::ZeroVector, FVector::ForwardVector, FVector(800.0, 0.0, 0.0), true));
    TestFalse(TEXT("Attack rejects an out-of-range target"),
        UGA_ProjectileAttack::IsAuthorityTargetEligible(
            FVector::ZeroVector, FVector::ForwardVector, FVector(1200.0, 0.0, 0.0), true));
    TestFalse(TEXT("Attack rejects a target behind the shooter"),
        UGA_ProjectileAttack::IsAuthorityTargetEligible(
            FVector::ZeroVector, FVector::ForwardVector, FVector(-100.0, 0.0, 0.0), true));
    TestFalse(TEXT("Attack rejects a dead target"),
        UGA_ProjectileAttack::IsAuthorityTargetEligible(
            FVector::ZeroVector, FVector::ForwardVector, FVector(100.0, 0.0, 0.0), false));

    const UGA_Shield* Shield = GetDefault<UGA_Shield>();
    TestEqual(TEXT("Shield is LocalPredicted"), Shield->GetNetExecutionPolicy(), EGameplayAbilityNetExecutionPolicy::LocalPredicted);
    TestNotNull(TEXT("Shield has GameplayEffect cost"), Shield->GetCostGameplayEffect());
    TestTrue(TEXT("Shield has native asset tag"), Shield->GetAssetTags().HasTagExact(AuthorityArenaTags::Ability_Shield));
    const FGameplayTagContainer& ShieldGrantedTags =
        GetDefault<UAuthorityArenaGE_ShieldState>()->GetGrantedTags();
    TestTrue(TEXT("Shield effect grants State.Shield.Active"),
        ShieldGrantedTags.HasTagExact(AuthorityArenaTags::State_Shield_Active));

    const AAuthorityArenaProjectile* Projectile = GetDefault<AAuthorityArenaProjectile>();
    TestTrue(TEXT("Projectile actor replicates"), Projectile->GetIsReplicated());

    const AAuthorityArenaCharacter* Character = GetDefault<AAuthorityArenaCharacter>();
    TestNotNull(TEXT("Character owns CombatComponent"),
        Character->FindComponentByClass<UAuthorityArenaCombatComponent>());
    TestNotNull(TEXT("Character owns HealthComponent"),
        Character->FindComponentByClass<UAuthorityArenaHealthComponent>());

    return true;
}

#endif
