#include "Ability/AuthorityArenaEffects.h"

#include "Ability/AuthorityArenaAttributeSet.h"
#include "Ability/AuthorityArenaGameplayTags.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

namespace
{
void AddModifier(
    UGameplayEffect& Effect,
    const FGameplayAttribute& Attribute,
    const float Magnitude)
{
    FGameplayModifierInfo& Modifier = Effect.Modifiers.AddDefaulted_GetRef();
    Modifier.Attribute = Attribute;
    Modifier.ModifierOp = EGameplayModOp::Additive;
    Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Magnitude));
}

void SetDuration(UGameplayEffect& Effect, const float DurationSeconds)
{
    Effect.DurationPolicy = EGameplayEffectDurationType::HasDuration;
    Effect.DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(DurationSeconds));
}
} // namespace

UAuthorityArenaGE_DashCost::UAuthorityArenaGE_DashCost()
{
    DurationPolicy = EGameplayEffectDurationType::Instant;
    AddModifier(*this, UAuthorityArenaAttributeSet::GetEnergyAttribute(), -25.0f);
}

UAuthorityArenaGE_DashCooldown::UAuthorityArenaGE_DashCooldown()
{
    SetDuration(*this, 1.25f);
    UTargetTagsGameplayEffectComponent* TargetTags =
        CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTags"));
    GEComponents.Add(TargetTags);
    FInheritedTagContainer Tags;
    Tags.AddTag(AuthorityArenaTags::Cooldown_Dash);
    TargetTags->SetAndApplyTargetTagChanges(Tags);
}

UAuthorityArenaGE_AttackCost::UAuthorityArenaGE_AttackCost()
{
    DurationPolicy = EGameplayEffectDurationType::Instant;
    AddModifier(*this, UAuthorityArenaAttributeSet::GetEnergyAttribute(), -5.0f);
}

UAuthorityArenaGE_AttackCooldown::UAuthorityArenaGE_AttackCooldown()
{
    SetDuration(*this, 0.45f);
    UTargetTagsGameplayEffectComponent* TargetTags =
        CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTags"));
    GEComponents.Add(TargetTags);
    FInheritedTagContainer Tags;
    Tags.AddTag(AuthorityArenaTags::Cooldown_Attack);
    TargetTags->SetAndApplyTargetTagChanges(Tags);
}

UAuthorityArenaGE_ShieldCost::UAuthorityArenaGE_ShieldCost()
{
    DurationPolicy = EGameplayEffectDurationType::Instant;
    AddModifier(*this, UAuthorityArenaAttributeSet::GetEnergyAttribute(), -20.0f);
}

UAuthorityArenaGE_ShieldCooldown::UAuthorityArenaGE_ShieldCooldown()
{
    SetDuration(*this, 3.0f);
    UTargetTagsGameplayEffectComponent* TargetTags =
        CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTags"));
    GEComponents.Add(TargetTags);
    FInheritedTagContainer Tags;
    Tags.AddTag(AuthorityArenaTags::Cooldown_Shield);
    TargetTags->SetAndApplyTargetTagChanges(Tags);
}

UAuthorityArenaGE_ShieldState::UAuthorityArenaGE_ShieldState()
{
    SetDuration(*this, 2.0f);
    UTargetTagsGameplayEffectComponent* TargetTags =
        CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(TEXT("TargetTags"));
    GEComponents.Add(TargetTags);
    FInheritedTagContainer Tags;
    Tags.AddTag(AuthorityArenaTags::State_Shield_Active);
    TargetTags->SetAndApplyTargetTagChanges(Tags);
}

UAuthorityArenaGE_ProjectileDamage::UAuthorityArenaGE_ProjectileDamage()
{
    DurationPolicy = EGameplayEffectDurationType::Instant;
    AddModifier(*this, UAuthorityArenaAttributeSet::GetIncomingDamageAttribute(), 34.0f);
}
