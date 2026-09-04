#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "AuthorityArenaEffects.generated.h"

UCLASS()
class AUTHORITYARENA_API UAuthorityArenaGE_DashCost : public UGameplayEffect
{
    GENERATED_BODY()
public:
    UAuthorityArenaGE_DashCost();
};

UCLASS()
class AUTHORITYARENA_API UAuthorityArenaGE_DashCooldown : public UGameplayEffect
{
    GENERATED_BODY()
public:
    UAuthorityArenaGE_DashCooldown();
};

UCLASS()
class AUTHORITYARENA_API UAuthorityArenaGE_AttackCost : public UGameplayEffect
{
    GENERATED_BODY()
public:
    UAuthorityArenaGE_AttackCost();
};

UCLASS()
class AUTHORITYARENA_API UAuthorityArenaGE_AttackCooldown : public UGameplayEffect
{
    GENERATED_BODY()
public:
    UAuthorityArenaGE_AttackCooldown();
};

UCLASS()
class AUTHORITYARENA_API UAuthorityArenaGE_ShieldCost : public UGameplayEffect
{
    GENERATED_BODY()
public:
    UAuthorityArenaGE_ShieldCost();
};

UCLASS()
class AUTHORITYARENA_API UAuthorityArenaGE_ShieldCooldown : public UGameplayEffect
{
    GENERATED_BODY()
public:
    UAuthorityArenaGE_ShieldCooldown();
};

UCLASS()
class AUTHORITYARENA_API UAuthorityArenaGE_ShieldState : public UGameplayEffect
{
    GENERATED_BODY()
public:
    UAuthorityArenaGE_ShieldState();
};

UCLASS()
class AUTHORITYARENA_API UAuthorityArenaGE_ProjectileDamage : public UGameplayEffect
{
    GENERATED_BODY()
public:
    UAuthorityArenaGE_ProjectileDamage();
};
