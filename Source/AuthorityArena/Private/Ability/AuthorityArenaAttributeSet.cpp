#include "Ability/AuthorityArenaAttributeSet.h"

#include "Ability/AuthorityArenaGameplayTags.h"
#include "Character/AuthorityArenaCharacter.h"
#include "Combat/AuthorityArenaHealthComponent.h"
#include "Diagnostics/AuthorityArenaNetworkDiagnosticsSubsystem.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

UAuthorityArenaAttributeSet::UAuthorityArenaAttributeSet()
{
    InitHealth(100.0f);
    InitMaxHealth(100.0f);
    InitEnergy(100.0f);
    InitMaxEnergy(100.0f);
    InitIncomingDamage(0.0f);
}

float UAuthorityArenaAttributeSet::ComputeMitigatedDamage(
    const float RawDamage,
    const bool bShieldActive)
{
    if (!FMath::IsFinite(RawDamage) || RawDamage <= 0.0f)
    {
        return 0.0f;
    }
    return bShieldActive ? RawDamage * 0.5f : RawDamage;
}

void UAuthorityArenaAttributeSet::PreAttributeChange(
    const FGameplayAttribute& Attribute,
    float& NewValue)
{
    Super::PreAttributeChange(Attribute, NewValue);
    if (Attribute == GetHealthAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
    }
    else if (Attribute == GetEnergyAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxEnergy());
    }
}

void UAuthorityArenaAttributeSet::PostGameplayEffectExecute(
    const FGameplayEffectModCallbackData& Data)
{
    Super::PostGameplayEffectExecute(Data);

    if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
    {
        const float RawDamage = GetIncomingDamage();
        SetIncomingDamage(0.0f);
        UAbilitySystemComponent* AbilitySystem = GetOwningAbilitySystemComponent();
        const bool bShieldActive = AbilitySystem != nullptr &&
            AbilitySystem->HasMatchingGameplayTag(AuthorityArenaTags::State_Shield_Active);
        const float Damage = ComputeMitigatedDamage(RawDamage, bShieldActive);
        if (Damage > 0.0f)
        {
            SetHealth(FMath::Clamp(GetHealth() - Damage, 0.0f, GetMaxHealth()));
            UAuthorityArenaNetworkDiagnosticsSubsystem::EmitEvent(
                AbilitySystem != nullptr ? AbilitySystem->GetAvatarActor() : nullptr,
                TEXT("DamageApplied"),
                FString::Printf(
                    TEXT("raw=%.2f applied=%.2f shield=%s health=%.2f"),
                    RawDamage,
                    Damage,
                    bShieldActive ? TEXT("true") : TEXT("false"),
                    GetHealth()));
            if (GetHealth() <= 0.0f && AbilitySystem != nullptr && AbilitySystem->IsOwnerActorAuthoritative())
            {
                if (AAuthorityArenaCharacter* Character = Cast<AAuthorityArenaCharacter>(AbilitySystem->GetAvatarActor()))
                {
                    if (UAuthorityArenaHealthComponent* HealthComponent =
                            Character->FindComponentByClass<UAuthorityArenaHealthComponent>())
                    {
                        HealthComponent->HandleHealthDepleted(
                            Data.EffectSpec.GetContext().GetOriginalInstigator());
                    }
                }
            }
        }
    }
    else if (Data.EvaluatedData.Attribute == GetHealthAttribute())
    {
        SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
    }
    else if (Data.EvaluatedData.Attribute == GetEnergyAttribute())
    {
        SetEnergy(FMath::Clamp(GetEnergy(), 0.0f, GetMaxEnergy()));
    }
}

void UAuthorityArenaAttributeSet::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION_NOTIFY(UAuthorityArenaAttributeSet, Health, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UAuthorityArenaAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UAuthorityArenaAttributeSet, Energy, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UAuthorityArenaAttributeSet, MaxEnergy, COND_None, REPNOTIFY_Always);
}

void UAuthorityArenaAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UAuthorityArenaAttributeSet, Health, OldValue);
}

void UAuthorityArenaAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UAuthorityArenaAttributeSet, MaxHealth, OldValue);
}

void UAuthorityArenaAttributeSet::OnRep_Energy(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UAuthorityArenaAttributeSet, Energy, OldValue);
}

void UAuthorityArenaAttributeSet::OnRep_MaxEnergy(const FGameplayAttributeData& OldValue)
{
    GAMEPLAYATTRIBUTE_REPNOTIFY(UAuthorityArenaAttributeSet, MaxEnergy, OldValue);
}
