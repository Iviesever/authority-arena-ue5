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
class UPointLightComponent;
class UTextRenderComponent;

UCLASS()
class AUTHORITYARENA_API AAuthorityArenaCharacter : public ACharacter, public IAbilitySystemInterface
{
    GENERATED_BODY()

public:
    AAuthorityArenaCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void PossessedBy(AController* NewController) override;
    virtual void OnRep_PlayerState() override;
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

    bool TryActivateAbility(FGameplayTag AbilityTag);

private:
    void InitializeAbilitySystem();
    void UpdateAppearance();
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

    UPROPERTY(VisibleAnywhere, Category = "Appearance")
    TObjectPtr<UTextRenderComponent> PlayerLabel;

    UPROPERTY(VisibleAnywhere, Category = "Appearance")
    TObjectPtr<UPointLightComponent> PlayerLight;

    UPROPERTY(VisibleAnywhere, Category = "Automation")
    TObjectPtr<UAuthorityArenaAutomationDriver> AutomationDriver;

    UPROPERTY(VisibleAnywhere, Category = "Combat")
    TObjectPtr<UAuthorityArenaCombatComponent> CombatComponent;

    UPROPERTY(VisibleAnywhere, Category = "Combat")
    TObjectPtr<UAuthorityArenaHealthComponent> HealthComponent;

    bool bAppearanceInitialized = false;
};
