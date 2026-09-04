#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerState.h"
#include "AuthorityArenaPlayerState.generated.h"

class UAuthorityArenaAbilitySystemComponent;
class UAuthorityArenaAttributeSet;

UCLASS()
class AUTHORITYARENA_API AAuthorityArenaPlayerState : public APlayerState, public IAbilitySystemInterface
{
    GENERATED_BODY()

public:
    AAuthorityArenaPlayerState();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

    UAuthorityArenaAbilitySystemComponent* GetAuthorityAbilitySystem() const { return AbilitySystemComponent; }
    UAuthorityArenaAttributeSet* GetAuthorityAttributeSet() const { return AttributeSet; }

    const FString& GetConnectionId() const { return ConnectionId; }
    const FString& GetDisplayName() const { return DisplayName; }
    int32 GetScoreValue() const { return ScoreValue; }
    int32 GetDeathCount() const { return DeathCount; }

    bool SetConnectionIdentityAuthority(const FString& NewConnectionId, const FString& NewDisplayName);
    bool AddScoreAuthority(int32 Delta = 1);
    bool RecordDeathAuthority();

private:
    UPROPERTY(VisibleAnywhere, Category = "Abilities")
    TObjectPtr<UAuthorityArenaAbilitySystemComponent> AbilitySystemComponent;

    UPROPERTY(VisibleAnywhere, Category = "Abilities")
    TObjectPtr<UAuthorityArenaAttributeSet> AttributeSet;

    UFUNCTION()
    void OnRep_ConnectionId();

    UFUNCTION()
    void OnRep_DisplayName();

    UFUNCTION()
    void OnRep_ScoreValue();

    UFUNCTION()
    void OnRep_DeathCount();

    UPROPERTY(ReplicatedUsing = OnRep_ConnectionId)
    FString ConnectionId;

    UPROPERTY(ReplicatedUsing = OnRep_DisplayName)
    FString DisplayName;

    UPROPERTY(ReplicatedUsing = OnRep_ScoreValue)
    int32 ScoreValue = 0;

    UPROPERTY(ReplicatedUsing = OnRep_DeathCount)
    int32 DeathCount = 0;
};
