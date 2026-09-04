#include "Automation/AuthorityArenaAutomationDriver.h"

#include "Ability/AuthorityArenaGameplayTags.h"
#include "Ability/AuthorityArenaAttributeSet.h"
#include "Character/AuthorityArenaCharacter.h"
#include "Diagnostics/AuthorityArenaNetworkDiagnosticsSubsystem.h"
#include "Engine/World.h"
#include "Game/AuthorityArenaGameState.h"
#include "Player/AuthorityArenaPlayerController.h"
#include "Player/AuthorityArenaPlayerState.h"

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
    FParse::Value(FCommandLine::Get(), TEXT("AuthorityMoveDuration="), MoveDurationSeconds);
    FParse::Value(FCommandLine::Get(), TEXT("AuthorityExitAfter="), ExitAfterSeconds);
    MoveDurationSeconds = FMath::Clamp(MoveDurationSeconds, 0.25f, 10.0f);
    ExitAfterSeconds = FMath::Clamp(ExitAfterSeconds, 0.0f, 60.0f);
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
    if (!bCombat && !bDashOnly && !bAttackOnly)
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
        if (bAttackOnly)
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
                RequestOnce(bAttackTwoRequested, 2.40, AuthorityArenaTags::Ability_Attack, TEXT("Attack2"));
                RequestOnce(bAttackThreeRequested, 3.00, AuthorityArenaTags::Ability_Attack, TEXT("Attack3"));
                RequestOnce(bAttackFourRequested, 3.60, AuthorityArenaTags::Ability_Attack, TEXT("Attack4"));
            }
        }
    }
    else if (PlayerId == TEXT("Client2") && bCombat)
    {
        RequestOnce(bShieldRequested, 0.10, AuthorityArenaTags::Ability_Shield, TEXT("Shield"));
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

    if (!bExitRequested &&
        ExitAfterSeconds > 0.0f &&
        ElapsedSeconds >= ExitAfterSeconds &&
        Character->GetNetMode() == NM_Client)
    {
        bExitRequested = true;
        const UAuthorityArenaAttributeSet* Attributes = PlayerState->GetAuthorityAttributeSet();
        const FVector Location = Character->GetActorLocation();
        UAuthorityArenaNetworkDiagnosticsSubsystem::EmitEvent(
            this,
            TEXT("ClientScenarioComplete"),
            FString::Printf(
                TEXT("player=%s x=%.2f y=%.2f health=%.2f energy=%.2f"),
                *PlayerState->GetConnectionId(),
                Location.X,
                Location.Y,
                Attributes ? Attributes->GetHealth() : -1.0f,
                Attributes ? Attributes->GetEnergy() : -1.0f));
        FGenericPlatformMisc::RequestExit(false);
    }
}
