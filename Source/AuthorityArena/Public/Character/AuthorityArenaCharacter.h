#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AuthorityArenaCharacter.generated.h"

class UCameraComponent;
class UAuthorityArenaAutomationDriver;
class USpringArmComponent;
class UStaticMeshComponent;

UCLASS()
class AUTHORITYARENA_API AAuthorityArenaCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AAuthorityArenaCharacter();

    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
    void MoveForward(float Value);
    void MoveRight(float Value);
    void Turn(float Value);
    void LookUp(float Value);

    UPROPERTY(VisibleAnywhere, Category = "Camera")
    TObjectPtr<USpringArmComponent> CameraBoom;

    UPROPERTY(VisibleAnywhere, Category = "Camera")
    TObjectPtr<UCameraComponent> FollowCamera;

    UPROPERTY(VisibleAnywhere, Category = "Appearance")
    TObjectPtr<UStaticMeshComponent> BodyMesh;

    UPROPERTY(VisibleAnywhere, Category = "Automation")
    TObjectPtr<UAuthorityArenaAutomationDriver> AutomationDriver;
};
