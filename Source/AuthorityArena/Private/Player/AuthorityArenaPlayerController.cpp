#include "Player/AuthorityArenaPlayerController.h"

#include "Ability/AuthorityArenaAbilitySystemComponent.h"
#include "Ability/AuthorityArenaAttributeSet.h"
#include "Ability/AuthorityArenaGameplayTags.h"
#include "AuthorityRules.h"
#include "Character/AuthorityArenaCharacter.h"
#include "Diagnostics/AuthorityArenaNetworkDiagnosticsSubsystem.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Game/AuthorityArenaGameMode.h"
#include "Game/AuthorityArenaGameState.h"
#include "Player/AuthorityArenaPlayerState.h"

AAuthorityArenaPlayerController::AAuthorityArenaPlayerController()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AAuthorityArenaPlayerController::BeginPlay()
{
    Super::BeginPlay();
    AutomationStartSeconds = GetWorld() != nullptr ? GetWorld()->GetTimeSeconds() : 0.0;
    FParse::Value(
        FCommandLine::Get(),
        TEXT("AuthorityRequestRespawnAfter="),
        AutomationRespawnRequestSeconds);
    bAuthorityAbuse = FParse::Param(FCommandLine::Get(), TEXT("AuthorityAbuse"));
    bAuthorityFlood = FParse::Param(FCommandLine::Get(), TEXT("AuthorityFlood"));
    bDuplicateRespawnAutomation =
        FParse::Param(FCommandLine::Get(), TEXT("AuthorityDuplicateRespawn"));
}

void AAuthorityArenaPlayerController::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (GetWorld() == nullptr || GetNetMode() != NM_Client || !IsLocalController())
    {
        return;
    }
    TickRespawnAutomation();
    TickAuthorityProbeAutomation();
}

void AAuthorityArenaPlayerController::TickRespawnAutomation()
{
    if (AutomationRespawnRequestSeconds <= 0.0f)
    {
        return;
    }
    const double Elapsed = GetWorld()->GetTimeSeconds() - AutomationStartSeconds;
    if (!bAutomationRespawnRequested && Elapsed >= AutomationRespawnRequestSeconds)
    {
        bAutomationRespawnRequested = true;
        UAuthorityArenaNetworkDiagnosticsSubsystem::EmitEvent(this, TEXT("RespawnRequested"));
        ServerRequestRespawn();
    }
    if (bDuplicateRespawnAutomation &&
        bAutomationRespawnRequested &&
        !bAutomationSecondRespawnRequested &&
        Elapsed >= AutomationRespawnRequestSeconds + 0.10f)
    {
        bAutomationSecondRespawnRequested = true;
        UAuthorityArenaNetworkDiagnosticsSubsystem::EmitEvent(
            this, TEXT("RespawnRequestedDuplicate"));
        ServerRequestRespawn();
    }
}

void AAuthorityArenaPlayerController::TickAuthorityProbeAutomation()
{
    if (!bAuthorityAbuse && !bAuthorityFlood)
    {
        return;
    }
    const AAuthorityArenaPlayerState* ArenaPlayerState = GetPlayerState<AAuthorityArenaPlayerState>();
    const AAuthorityArenaGameState* GameState = GetWorld()->GetGameState<AAuthorityArenaGameState>();
    const AAuthorityArenaCharacter* ArenaCharacter = Cast<AAuthorityArenaCharacter>(GetPawn());
    if (ArenaPlayerState == nullptr || ArenaPlayerState->GetConnectionId() != TEXT("Client1") ||
        GameState == nullptr || ArenaCharacter == nullptr ||
        GameState->GetScenarioStartServerTime() <= 0.0f)
    {
        return;
    }

    AAuthorityArenaCharacter* Peer = nullptr;
    for (TActorIterator<AAuthorityArenaCharacter> It(GetWorld()); It; ++It)
    {
        AAuthorityArenaCharacter* Candidate = *It;
        const AAuthorityArenaPlayerState* CandidateState =
            Candidate->GetPlayerState<AAuthorityArenaPlayerState>();
        if (Candidate != ArenaCharacter && CandidateState != nullptr &&
            CandidateState->GetConnectionId() == TEXT("Client2"))
        {
            Peer = Candidate;
            break;
        }
    }
    if (Peer == nullptr)
    {
        return;
    }

    const UAuthorityArenaAttributeSet* Attributes = ArenaPlayerState->GetAuthorityAttributeSet();
    const float Health = Attributes != nullptr ? Attributes->GetHealth() : -1.0f;
    const int32 Score = ArenaPlayerState->GetScoreValue();
    const double Elapsed =
        GameState->GetServerWorldTimeSeconds() - GameState->GetScenarioStartServerTime();

    auto SendOnce = [this, Elapsed, Health, Score](
        bool& bSent,
        const double AtSeconds,
        AActor* Target,
        const float Damage,
        const float ClaimedHealth,
        const int32 ClaimedScore,
        const uint32 Sequence)
    {
        if (!bSent && Elapsed >= AtSeconds)
        {
            bSent = true;
            ServerSubmitAuthorityProbe(
                Target, Damage, ClaimedHealth, ClaimedScore, Sequence);
        }
    };

    if (bAuthorityAbuse)
    {
        SendOnce(bProbeOneSent, 0.10, Peer, 34.0f, 999.0f, 99, 1);
        SendOnce(bProbeTwoSent, 0.45, Peer, 999.0f, Health, Score, 2);
        SendOnce(bProbeThreeSent, 0.80, nullptr, 34.0f, Health, Score, 3);
        SendOnce(bProbeFourSent, 1.15, Peer, 34.0f, Health, Score, 4);
        SendOnce(bProbeFiveSent, 1.50, Peer, 34.0f, Health, Score, 4);
    }
    else if (bAuthorityFlood)
    {
        SendOnce(bProbeOneSent, 2.50, Peer, 34.0f, Health, Score, 1);
        SendOnce(bProbeTwoSent, 2.55, Peer, 34.0f, Health, Score, 2);
        SendOnce(bProbeThreeSent, 2.60, Peer, 34.0f, Health, Score, 3);
        SendOnce(bProbeFourSent, 2.65, Peer, 34.0f, Health, Score, 4);
    }
}

bool AAuthorityArenaPlayerController::TryMarkRespawnPendingAuthority()
{
    if (!HasAuthority() || bRespawnPending)
    {
        return false;
    }
    bRespawnPending = true;
    return true;
}

void AAuthorityArenaPlayerController::ClearRespawnPendingAuthority()
{
    if (HasAuthority())
    {
        bRespawnPending = false;
    }
}

void AAuthorityArenaPlayerController::ServerRequestRespawn_Implementation()
{
    if (GetPawn() != nullptr)
    {
        ClientRequestRejected(TEXT("Respawn"), TEXT("NotDead"));
        return;
    }
    if (AAuthorityArenaGameMode* GameMode = GetWorld() != nullptr
            ? GetWorld()->GetAuthGameMode<AAuthorityArenaGameMode>()
            : nullptr)
    {
        GameMode->RequestRespawn(this);
        return;
    }
    ClientRequestRejected(TEXT("Respawn"), TEXT("NoAuthorityGameMode"));
}

void AAuthorityArenaPlayerController::ServerReportViewSample_Implementation(
    const FRotator ViewRotation,
    const uint32 Sequence)
{
    if (Sequence <= LastAcceptedViewSequence || ViewRotation.ContainsNaN())
    {
        return;
    }
    LastAcceptedViewSequence = Sequence;
    UAuthorityArenaNetworkDiagnosticsSubsystem::EmitEvent(
        this,
        TEXT("ViewSample"),
        FString::Printf(TEXT("sequence=%u yaw=%.2f"), Sequence, ViewRotation.Yaw));
}

void AAuthorityArenaPlayerController::ServerSubmitAuthorityProbe_Implementation(
    AActor* ClaimedTarget,
    const float ClaimedDamage,
    const float ClaimedHealth,
    const int32 ClaimedScore,
    const uint32 Sequence)
{
    const AAuthorityArenaCharacter* ArenaCharacter = Cast<AAuthorityArenaCharacter>(GetPawn());
    const AAuthorityArenaPlayerState* ArenaPlayerState = GetPlayerState<AAuthorityArenaPlayerState>();
    const UAuthorityArenaAttributeSet* Attributes =
        ArenaPlayerState != nullptr ? ArenaPlayerState->GetAuthorityAttributeSet() : nullptr;
    const UAuthorityArenaAbilitySystemComponent* CharacterAbilitySystem =
        ArenaCharacter != nullptr
            ? Cast<UAuthorityArenaAbilitySystemComponent>(ArenaCharacter->GetAbilitySystemComponent())
            : nullptr;
    const AAuthorityArenaCharacter* Target = Cast<AAuthorityArenaCharacter>(ClaimedTarget);
    const UAuthorityArenaAbilitySystemComponent* TargetAbilitySystem =
        Target != nullptr
            ? Cast<UAuthorityArenaAbilitySystemComponent>(Target->GetAbilitySystemComponent())
            : nullptr;
    const double Now = GetWorld() != nullptr ? GetWorld()->GetTimeSeconds() : 0.0;
    const double SquaredDistance = ArenaCharacter != nullptr && Target != nullptr
        ? FVector::DistSquared(ArenaCharacter->GetActorLocation(), Target->GetActorLocation())
        : 0.0;

    const AuthorityArena::Core::AuthorityProbeRequest Request{
        .has_authority = HasAuthority(),
        .is_owner = ArenaCharacter != nullptr && ArenaCharacter->GetController() == this,
        .is_alive = CharacterAbilitySystem != nullptr &&
            !CharacterAbilitySystem->HasMatchingGameplayTag(AuthorityArenaTags::State_Dead),
        .target_valid = Target != nullptr && Target != ArenaCharacter,
        .target_alive = TargetAbilitySystem != nullptr &&
            !TargetAbilitySystem->HasMatchingGameplayTag(AuthorityArenaTags::State_Dead),
        .now_seconds = Now,
        .last_request_seconds = LastAuthorityProbeSeconds,
        .minimum_interval_seconds = 0.45,
        .squared_distance = SquaredDistance,
        .maximum_squared_distance = 1'000'000.0,
        .claimed_damage = ClaimedDamage,
        .server_damage = 34.0,
        .claimed_health = ClaimedHealth,
        .server_health = Attributes != nullptr ? Attributes->GetHealth() : 0.0,
        .claimed_score = ClaimedScore,
        .server_score = ArenaPlayerState != nullptr ? ArenaPlayerState->GetScoreValue() : 0,
        .sequence = Sequence,
        .last_sequence = LastAuthorityProbeSequence,
    };
    const AuthorityArena::Core::DecisionCode Decision =
        AuthorityArena::Core::ValidateAuthorityProbe(Request);
    if (Sequence > LastAuthorityProbeSequence)
    {
        LastAuthorityProbeSequence = Sequence;
    }

    const FString Reason = UTF8_TO_TCHAR(AuthorityArena::Core::ToString(Decision));
    if (Decision == AuthorityArena::Core::DecisionCode::Allowed)
    {
        LastAuthorityProbeSeconds = Now;
        UAuthorityArenaNetworkDiagnosticsSubsystem::EmitEvent(
            this,
            TEXT("AuthorityProbeAccepted"),
            FString::Printf(TEXT("sequence=%u"), Sequence));
        return;
    }

    UAuthorityArenaNetworkDiagnosticsSubsystem::EmitEvent(
        this,
        TEXT("AuthorityProbeRejected"),
        FString::Printf(TEXT("sequence=%u reason=%s"), Sequence, *Reason));
    ClientRequestRejected(TEXT("AuthorityProbe"), FName(*Reason));
}

void AAuthorityArenaPlayerController::ClientRequestRejected_Implementation(
    const FName Action,
    const FName Reason)
{
    UAuthorityArenaNetworkDiagnosticsSubsystem::EmitEvent(
        this,
        TEXT("RequestRejected"),
        FString::Printf(TEXT("action=%s reason=%s"), *Action.ToString(), *Reason.ToString()));
}
