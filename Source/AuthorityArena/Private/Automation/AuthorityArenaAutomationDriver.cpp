#include "Automation/AuthorityArenaAutomationDriver.h"

#include "Ability/AuthorityArenaAbilitySystemComponent.h"
#include "Ability/AuthorityArenaGameplayTags.h"
#include "Character/AuthorityArenaCharacter.h"
#include "Diagnostics/AuthorityArenaNetworkDiagnosticsSubsystem.h"
#include "Engine/World.h"
#include "Game/AuthorityArenaGameState.h"
#include "Player/AuthorityArenaPlayerController.h"
#include "Player/AuthorityArenaPlayerState.h"
#include "GameplayPrediction.h"
#include "UnrealClient.h"

namespace
{
const TCHAR* DescribeNetMode(const ENetMode NetMode)
{
    switch (NetMode)
    {
    case NM_Standalone: return TEXT("Standalone");
    case NM_DedicatedServer: return TEXT("DedicatedServer");
    case NM_ListenServer: return TEXT("ListenServer");
    case NM_Client: return TEXT("Client");
    default: return TEXT("Unknown");
    }
}
} // namespace

UAuthorityArenaAutomationDriver::UAuthorityArenaAutomationDriver()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
    SetIsReplicatedByDefault(false);
}

void UAuthorityArenaAutomationDriver::BeginPlay()
{
    Super::BeginPlay();
    StartTimeSeconds = GetWorld() != nullptr ? GetWorld()->GetTimeSeconds() : 0.0;
    bAutoMove = FParse::Param(FCommandLine::Get(), TEXT("AuthorityAutoMove"));
    bCombat = FParse::Param(FCommandLine::Get(), TEXT("AuthorityCombat"));
    bDashOnly = FParse::Param(FCommandLine::Get(), TEXT("AuthorityDashOnly"));
    bAttackOnly = FParse::Param(FCommandLine::Get(), TEXT("AuthorityAttackOnly"));
    bInvalidAttack = FParse::Param(FCommandLine::Get(), TEXT("AuthorityInvalidAttack"));
    bAttackFlood = FParse::Param(FCommandLine::Get(), TEXT("AuthorityFlood"));
    FParse::Value(FCommandLine::Get(), TEXT("AuthorityMoveDuration="), MoveDurationSeconds);
    FParse::Value(FCommandLine::Get(), TEXT("AuthorityScreenshot="), ScreenshotPath);
    MoveDurationSeconds = FMath::Clamp(MoveDurationSeconds, 0.25f, 10.0f);
}

void UAuthorityArenaAutomationDriver::TickComponent(
    const float DeltaTime,
    const ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    TryEmitRoleSnapshot();

    const AAuthorityArenaCharacter* Character = Cast<AAuthorityArenaCharacter>(GetOwner());
    if (Character == nullptr || GetWorld() == nullptr || !Character->IsLocallyControlled())
    {
        return;
    }

    const double ElapsedSeconds = GetWorld()->GetTimeSeconds() - StartTimeSeconds;
    TickOwnedClient(DeltaTime, ElapsedSeconds);
    TickCombat();
}

void UAuthorityArenaAutomationDriver::TickCombat()
{
    if (!bCombat && !bDashOnly && !bAttackOnly && !bInvalidAttack && !bAttackFlood)
    {
        return;
    }
    AAuthorityArenaCharacter* Character = Cast<AAuthorityArenaCharacter>(GetOwner());
    const AAuthorityArenaPlayerState* PlayerState =
        Character != nullptr ? Character->GetPlayerState<AAuthorityArenaPlayerState>() : nullptr;
    if (Character == nullptr || PlayerState == nullptr || PlayerState->GetConnectionId().IsEmpty())
    {
        return;
    }

    const AAuthorityArenaGameState* GameState = GetWorld()->GetGameState<AAuthorityArenaGameState>();
    if (GameState == nullptr || GameState->GetScenarioStartServerTime() <= 0.0f)
    {
        return;
    }
    const double ElapsedSeconds =
        GameState->GetServerWorldTimeSeconds() - GameState->GetScenarioStartServerTime();
    if (ElapsedSeconds < 0.0)
    {
        return;
    }

    const FString& PlayerId = PlayerState->GetConnectionId();
    auto RequestOnce = [this, Character, &PlayerId, ElapsedSeconds](
        bool& bRequested,
        const double AtSeconds,
        const FGameplayTag AbilityTag,
        const TCHAR* Action)
    {
        if (bRequested || ElapsedSeconds < AtSeconds)
        {
            return;
        }
        bRequested = true;
        const bool bAccepted = Character->TryActivateAbility(AbilityTag);
        UAuthorityArenaNetworkDiagnosticsSubsystem::EmitEvent(
            this,
            TEXT("AbilityInput"),
            FString::Printf(
                TEXT("player=%s action=%s accepted=%s"),
                *PlayerId,
                Action,
                bAccepted ? TEXT("true") : TEXT("false")));
    };

    if (PlayerId == TEXT("Client1"))
    {
        if (bAttackFlood)
        {
            if (AttackFloodAttempt < 4 &&
                ElapsedSeconds >= 0.25 + static_cast<double>(AttackFloodAttempt) * 0.05)
            {
                UAuthorityArenaAbilitySystemComponent* AbilitySystem =
                    PlayerState->GetAuthorityAbilitySystem();
                TArray<FGameplayAbilitySpecHandle> AttackHandles;
                FGameplayTagContainer AttackTags;
                AttackTags.AddTag(AuthorityArenaTags::Ability_Attack);
                if (AbilitySystem != nullptr)
                {
                    AbilitySystem->FindAllAbilitiesWithTags(AttackHandles, AttackTags, true);
                }
                ++AttackFloodAttempt;
                if (AbilitySystem == nullptr || AttackHandles.Num() != 1)
                {
                    UAuthorityArenaNetworkDiagnosticsSubsystem::EmitEvent(
                        this,
                        TEXT("AttackFloodAbilityRequestFailed"),
                        FString::Printf(TEXT("attempt=%u reason=MissingAbilitySpec"), AttackFloodAttempt));
                }
                else
                {
                    FScopedPredictionWindow PredictionWindow(AbilitySystem, true);
                    const FPredictionKey PredictionKey = AbilitySystem->GetPredictionKeyForNewAction();
                    AbilitySystem->CallServerTryActivateAbility(
                        AttackHandles[0], false, PredictionKey);
                    UAuthorityArenaNetworkDiagnosticsSubsystem::EmitEvent(
                        this,
                        TEXT("AttackFloodAbilityRequest"),
                        FString::Printf(
                            TEXT("attempt=%u prediction=%s"),
                            AttackFloodAttempt,
                            *PredictionKey.ToString()));
                }
            }
        }
        else if (bInvalidAttack)
        {
            RequestOnce(bAttackOneRequested, 0.35, AuthorityArenaTags::Ability_Attack, TEXT("InvalidTargetAttack"));
        }
        else if (bAttackOnly)
        {
            RequestOnce(bAttackOneRequested, 0.35, AuthorityArenaTags::Ability_Attack, TEXT("AttackWhileDead"));
        }
        else
        {
            RequestOnce(bDashRequested, 0.35, AuthorityArenaTags::Ability_Dash, TEXT("Dash"));
            if (bCombat)
            {
                RequestOnce(bSecondDashRequested, 0.55, AuthorityArenaTags::Ability_Dash, TEXT("DashRepeat"));
                RequestOnce(bAttackOneRequested, 0.90, AuthorityArenaTags::Ability_Attack, TEXT("Attack1"));
                RequestOnce(bAttackTwoRequested, 3.00, AuthorityArenaTags::Ability_Attack, TEXT("Attack2"));
                RequestOnce(bAttackThreeRequested, 4.00, AuthorityArenaTags::Ability_Attack, TEXT("Attack3"));
                RequestOnce(bAttackFourRequested, 5.00, AuthorityArenaTags::Ability_Attack, TEXT("Attack4"));
            }
        }
    }
    else if (PlayerId == TEXT("Client2") && bCombat)
    {
        RequestOnce(bShieldRequested, 0.10, AuthorityArenaTags::Ability_Shield, TEXT("Shield"));
    }

    if (!bScreenshotRequested && !ScreenshotPath.IsEmpty() && ElapsedSeconds >= 7.0)
    {
        bScreenshotRequested = true;
        FScreenshotRequest::RequestScreenshot(ScreenshotPath, true, false, false);
        UAuthorityArenaNetworkDiagnosticsSubsystem::EmitEvent(
            this,
            TEXT("ScreenshotRequested"),
            FString::Printf(TEXT("player=%s"), *PlayerId));
    }
}

void UAuthorityArenaAutomationDriver::TryEmitRoleSnapshot()
{
    if (bRoleSnapshotEmitted)
    {
        return;
    }

    const AAuthorityArenaCharacter* Character = Cast<AAuthorityArenaCharacter>(GetOwner());
    const AAuthorityArenaPlayerState* PlayerState =
        Character != nullptr ? Character->GetPlayerState<AAuthorityArenaPlayerState>() : nullptr;
    if (Character == nullptr || PlayerState == nullptr || PlayerState->GetConnectionId().IsEmpty())
    {
        return;
    }

    bRoleSnapshotEmitted = true;
    UAuthorityArenaNetworkDiagnosticsSubsystem::EmitEvent(
        this,
        TEXT("RoleSnapshot"),
        FString::Printf(
            TEXT("player=%s local_role=%s remote_role=%s net_mode=%s"),
            *PlayerState->GetConnectionId(),
            *UAuthorityArenaNetworkDiagnosticsSubsystem::DescribeRole(Character->GetLocalRole()),
            *UAuthorityArenaNetworkDiagnosticsSubsystem::DescribeRole(Character->GetRemoteRole()),
            DescribeNetMode(Character->GetNetMode())));
}

void UAuthorityArenaAutomationDriver::TickOwnedClient(
    const float DeltaTime,
    const double ElapsedSeconds)
{
    AAuthorityArenaCharacter* Character = Cast<AAuthorityArenaCharacter>(GetOwner());
    const AAuthorityArenaPlayerState* PlayerState =
        Character != nullptr ? Character->GetPlayerState<AAuthorityArenaPlayerState>() : nullptr;
    if (Character == nullptr || PlayerState == nullptr)
    {
        return;
    }

    if (bAutoMove && ElapsedSeconds < MoveDurationSeconds)
    {
        if (!bMoveStarted)
        {
            bMoveStarted = true;
            UAuthorityArenaNetworkDiagnosticsSubsystem::EmitEvent(
                this,
                TEXT("AutoMoveStart"),
                FString::Printf(TEXT("player=%s"), *PlayerState->GetConnectionId()));
        }
        Character->AddMovementInput(Character->GetActorForwardVector(), 1.0f);

        ViewSampleAccumulator += DeltaTime;
        if (ViewSampleAccumulator >= 0.25f)
        {
            ViewSampleAccumulator = 0.0f;
            if (AAuthorityArenaPlayerController* Controller =
                    Cast<AAuthorityArenaPlayerController>(Character->GetController()))
            {
                Controller->ServerReportViewSample(Character->GetControlRotation(), ++ViewSequence);
            }
        }
    }
    else if (bAutoMove && !bMoveCompleted)
    {
        bMoveCompleted = true;
        const FVector Location = Character->GetActorLocation();
        UAuthorityArenaNetworkDiagnosticsSubsystem::EmitEvent(
            this,
            TEXT("AutoMoveComplete"),
            FString::Printf(
                TEXT("player=%s x=%.2f y=%.2f z=%.2f"),
                *PlayerState->GetConnectionId(),
                Location.X,
                Location.Y,
                Location.Z));
    }

}
