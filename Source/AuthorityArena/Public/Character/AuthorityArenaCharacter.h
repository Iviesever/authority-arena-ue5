#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Character.h"
#include "AuthorityArenaCharacter.generated.h"

class UCameraComponent;
class UAuthorityArenaAutomationDriver;
class UAuthorityArenaCombatComponent;
class UAuthorityArenaHealthComponent;
class UAbilitySystemComponent;
class USpringArmComponent;
class UStaticMeshComponent;

UCLASS()
class AUTHORITYARENA_API AAuthorityArenaCharacter : public ACharacter, public IAbilitySystemInterface
{
    GENERATED_BODY()

public:
    AAuthorityArenaCharacter();

    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    virtual void PossessedBy(AController* NewController) override;
    virtual void OnRep_PlayerState() override;
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

    bool TryActivateAbility(FGameplayTag AbilityTag);

private:
    void InitializeAbilitySystem();
    void MoveForward(float Value);
    void MoveRight(float Value);
    void Turn(float Value);
    void LookUp(float Value);
    void ActivateDash();
    void ActivateAttack();
    void ActivateShield();

    UPROPERTY(VisibleAnywhere, Category = "Camera")
    TObjectPtr<USpringArmComponent> CameraBoom;

    UPROPERTY(VisibleAnywhere, Category = "Camera")
    TObjectPtr<UCameraComponent> FollowCamera;

    UPROPERTY(VisibleAnywhere, Category = "Appearance")
    TObjectPtr<UStaticMeshComponent> BodyMesh;

    UPROPERTY(VisibleAnywhere, Category = "Automation")
    TObjectPtr<UAuthorityArenaAutomationDriver> AutomationDriver;

    UPROPERTY(VisibleAnywhere, Category = "Combat")
    TObjectPtr<UAuthorityArenaCombatComponent> CombatComponent;

    UPROPERTY(VisibleAnywhere, Category = "Combat")
    TObjectPtr<UAuthorityArenaHealthComponent> HealthComponent;
};
