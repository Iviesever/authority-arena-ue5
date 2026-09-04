#include "Character/AuthorityArenaCharacter.h"

#include "Automation/AuthorityArenaAutomationDriver.h"
#include "Ability/AuthorityArenaAbilitySystemComponent.h"
#include "Ability/AuthorityArenaGameplayTags.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Combat/AuthorityArenaCombatComponent.h"
#include "Combat/AuthorityArenaHealthComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Movement/AuthorityArenaCharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Player/AuthorityArenaPlayerState.h"

AAuthorityArenaCharacter::AAuthorityArenaCharacter(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer.SetDefaultSubobjectClass<UAuthorityArenaCharacterMovementComponent>(
        ACharacter::CharacterMovementComponentName))
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetReplicateMovement(true);

    GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
    GetCharacterMovement()->MaxWalkSpeed = 520.0f;
    GetCharacterMovement()->BrakingDecelerationWalking = 1600.0f;
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    GetMesh()->SetVisibility(false);
    GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
    BodyMesh->SetupAttachment(GetCapsuleComponent());
    BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BodyMesh->SetRelativeScale3D(FVector(0.72f, 0.72f, 1.0f));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(
        TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (CylinderMesh.Succeeded())
    {
        BodyMesh->SetStaticMesh(CylinderMesh.Object);
    }

    PlayerLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("PlayerLabel"));
    PlayerLabel->SetupAttachment(GetCapsuleComponent());
    PlayerLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 155.0f));
    PlayerLabel->SetHorizontalAlignment(EHTA_Center);
    PlayerLabel->SetVerticalAlignment(EVRTA_TextCenter);
    PlayerLabel->SetWorldSize(42.0f);
    PlayerLabel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PlayerLabel->SetText(FText::FromString(TEXT("PLAYER")));

    PlayerLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PlayerLight"));
    PlayerLight->SetupAttachment(GetCapsuleComponent());
    PlayerLight->SetRelativeLocation(FVector(0.0f, 0.0f, 35.0f));
    PlayerLight->SetMobility(EComponentMobility::Movable);
    PlayerLight->SetIntensity(18000.0f);
    PlayerLight->SetAttenuationRadius(420.0f);
    PlayerLight->SetCastShadows(false);

    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 650.0f;
    CameraBoom->SetRelativeLocation(FVector(0.0f, 0.0f, 80.0f));
    CameraBoom->bUsePawnControlRotation = true;

    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;

    AutomationDriver = CreateDefaultSubobject<UAuthorityArenaAutomationDriver>(TEXT("AutomationDriver"));
    CombatComponent = CreateDefaultSubobject<UAuthorityArenaCombatComponent>(TEXT("CombatComponent"));
    HealthComponent = CreateDefaultSubobject<UAuthorityArenaHealthComponent>(TEXT("HealthComponent"));
}

void AAuthorityArenaCharacter::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    UpdateAppearance();
    if (bAppearanceInitialized && PlayerLabel != nullptr && GetWorld() != nullptr)
    {
        if (const APlayerController* LocalController = GetWorld()->GetFirstPlayerController())
        {
            if (LocalController->PlayerCameraManager != nullptr)
            {
                const FVector ToCamera =
                    LocalController->PlayerCameraManager->GetCameraLocation() - PlayerLabel->GetComponentLocation();
                PlayerLabel->SetWorldRotation(FRotator(0.0f, ToCamera.Rotation().Yaw, 0.0f));
            }
        }
    }
}

void AAuthorityArenaCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    check(PlayerInputComponent);
    PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AAuthorityArenaCharacter::MoveForward);
    PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AAuthorityArenaCharacter::MoveRight);
    PlayerInputComponent->BindAxis(TEXT("Turn"), this, &AAuthorityArenaCharacter::Turn);
    PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &AAuthorityArenaCharacter::LookUp);
    PlayerInputComponent->BindAction(TEXT("Dash"), IE_Pressed, this, &AAuthorityArenaCharacter::ActivateDash);
    PlayerInputComponent->BindAction(TEXT("Attack"), IE_Pressed, this, &AAuthorityArenaCharacter::ActivateAttack);
    PlayerInputComponent->BindAction(TEXT("Shield"), IE_Pressed, this, &AAuthorityArenaCharacter::ActivateShield);
}

void AAuthorityArenaCharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);
    InitializeAbilitySystem();
}

void AAuthorityArenaCharacter::OnRep_PlayerState()
{
    Super::OnRep_PlayerState();
    InitializeAbilitySystem();
}

UAbilitySystemComponent* AAuthorityArenaCharacter::GetAbilitySystemComponent() const
{
    const AAuthorityArenaPlayerState* ArenaPlayerState = GetPlayerState<AAuthorityArenaPlayerState>();
    return ArenaPlayerState != nullptr ? ArenaPlayerState->GetAbilitySystemComponent() : nullptr;
}

void AAuthorityArenaCharacter::InitializeAbilitySystem()
{
    AAuthorityArenaPlayerState* ArenaPlayerState = GetPlayerState<AAuthorityArenaPlayerState>();
    if (ArenaPlayerState == nullptr)
    {
        return;
    }
    UAuthorityArenaAbilitySystemComponent* AbilitySystem = ArenaPlayerState->GetAuthorityAbilitySystem();
    if (AbilitySystem == nullptr)
    {
        return;
    }
    AbilitySystem->InitAbilityActorInfo(ArenaPlayerState, this);
    if (HasAuthority())
    {
        AbilitySystem->GrantDefaultAbilities();
    }
    UpdateAppearance();
}

void AAuthorityArenaCharacter::UpdateAppearance()
{
    if (bAppearanceInitialized || BodyMesh == nullptr)
    {
        return;
    }
    const AAuthorityArenaPlayerState* ArenaPlayerState =
        GetPlayerState<AAuthorityArenaPlayerState>();
    if (ArenaPlayerState == nullptr || ArenaPlayerState->GetConnectionId().IsEmpty())
    {
        return;
    }

    const FLinearColor PlayerColor = ArenaPlayerState->GetConnectionId() == TEXT("Client1")
        ? FLinearColor(0.04f, 0.30f, 1.0f, 1.0f)
        : FLinearColor(1.0f, 0.18f, 0.03f, 1.0f);
    if (UMaterialInstanceDynamic* Material = BodyMesh->CreateAndSetMaterialInstanceDynamic(0))
    {
        Material->SetVectorParameterValue(TEXT("Color"), PlayerColor);
    }
    PlayerLabel->SetText(FText::FromString(ArenaPlayerState->GetConnectionId().ToUpper()));
    PlayerLabel->SetTextRenderColor(PlayerColor.ToFColor(true));
    PlayerLight->SetLightColor(PlayerColor);
    bAppearanceInitialized = true;
}

void AAuthorityArenaCharacter::MoveForward(const float Value)
{
    if (Controller != nullptr && !FMath::IsNearlyZero(Value))
    {
        const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
        AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), Value);
    }
}

void AAuthorityArenaCharacter::MoveRight(const float Value)
{
    if (Controller != nullptr && !FMath::IsNearlyZero(Value))
    {
        const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
        AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), Value);
    }
}

void AAuthorityArenaCharacter::Turn(const float Value)
{
    AddControllerYawInput(Value);
}

void AAuthorityArenaCharacter::LookUp(const float Value)
{
    AddControllerPitchInput(Value);
}

void AAuthorityArenaCharacter::ActivateDash()
{
    if (UAuthorityArenaAbilitySystemComponent* AbilitySystem =
            Cast<UAuthorityArenaAbilitySystemComponent>(GetAbilitySystemComponent()))
    {
        AbilitySystem->TryActivateAbilityByTag(AuthorityArenaTags::Ability_Dash);
    }
}

void AAuthorityArenaCharacter::ActivateAttack()
{
    if (UAuthorityArenaAbilitySystemComponent* AbilitySystem =
            Cast<UAuthorityArenaAbilitySystemComponent>(GetAbilitySystemComponent()))
    {
        AbilitySystem->TryActivateAbilityByTag(AuthorityArenaTags::Ability_Attack);
    }
}

void AAuthorityArenaCharacter::ActivateShield()
{
    if (UAuthorityArenaAbilitySystemComponent* AbilitySystem =
            Cast<UAuthorityArenaAbilitySystemComponent>(GetAbilitySystemComponent()))
    {
        AbilitySystem->TryActivateAbilityByTag(AuthorityArenaTags::Ability_Shield);
    }
}

bool AAuthorityArenaCharacter::TryActivateAbility(const FGameplayTag AbilityTag)
{
    if (UAuthorityArenaAbilitySystemComponent* AbilitySystem =
            Cast<UAuthorityArenaAbilitySystemComponent>(GetAbilitySystemComponent()))
    {
        return AbilitySystem->TryActivateAbilityByTag(AbilityTag);
    }
    return false;
}
