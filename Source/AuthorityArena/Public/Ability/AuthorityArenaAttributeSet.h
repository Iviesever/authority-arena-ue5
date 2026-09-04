#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AuthorityArenaAttributeSet.generated.h"

#define AA_ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class AUTHORITYARENA_API UAuthorityArenaAttributeSet : public UAttributeSet
{
    GENERATED_BODY()

public:
    UAuthorityArenaAttributeSet();

    static float ComputeMitigatedDamage(float RawDamage, bool bShieldActive);

    virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
    virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UPROPERTY(ReplicatedUsing = OnRep_Health)
    FGameplayAttributeData Health;
    AA_ATTRIBUTE_ACCESSORS(UAuthorityArenaAttributeSet, Health)

    UPROPERTY(ReplicatedUsing = OnRep_MaxHealth)
    FGameplayAttributeData MaxHealth;
    AA_ATTRIBUTE_ACCESSORS(UAuthorityArenaAttributeSet, MaxHealth)

    UPROPERTY(ReplicatedUsing = OnRep_Energy)
    FGameplayAttributeData Energy;
    AA_ATTRIBUTE_ACCESSORS(UAuthorityArenaAttributeSet, Energy)

    UPROPERTY(ReplicatedUsing = OnRep_MaxEnergy)
    FGameplayAttributeData MaxEnergy;
    AA_ATTRIBUTE_ACCESSORS(UAuthorityArenaAttributeSet, MaxEnergy)

    UPROPERTY()
    FGameplayAttributeData IncomingDamage;
    AA_ATTRIBUTE_ACCESSORS(UAuthorityArenaAttributeSet, IncomingDamage)

private:
    UFUNCTION()
    void OnRep_Health(const FGameplayAttributeData& OldValue);

    UFUNCTION()
    void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);

    UFUNCTION()
    void OnRep_Energy(const FGameplayAttributeData& OldValue);

    UFUNCTION()
    void OnRep_MaxEnergy(const FGameplayAttributeData& OldValue);
};

#undef AA_ATTRIBUTE_ACCESSORS
