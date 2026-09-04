#include "Game/AuthorityArenaGameState.h"

#include "Diagnostics/AuthorityArenaNetworkDiagnosticsSubsystem.h"
#include "Net/UnrealNetwork.h"

AAuthorityArenaGameState::AAuthorityArenaGameState()
{
    bReplicates = true;
}

void AAuthorityArenaGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AAuthorityArenaGameState, MatchPhase);
    DOREPLIFETIME(AAuthorityArenaGameState, RemainingSeconds);
    DOREPLIFETIME(AAuthorityArenaGameState, ScenarioRunId);
    DOREPLIFETIME(AAuthorityArenaGameState, RoundNumber);
    DOREPLIFETIME(AAuthorityArenaGameState, ScenarioStartServerTime);
}

bool AAuthorityArenaGameState::SetMatchPhaseAuthority(const FName NewPhase)
{
    if (!HasAuthority() || NewPhase.IsNone())
    {
        return false;
    }
    MatchPhase = NewPhase;
    OnRep_MatchPhase();
    return true;
}

bool AAuthorityArenaGameState::SetRemainingSecondsAuthority(const int32 NewRemainingSeconds)
{
    if (!HasAuthority())
    {
        return false;
    }
    RemainingSeconds = FMath::Max(0, NewRemainingSeconds);
    OnRep_RemainingSeconds();
    return true;
}

bool AAuthorityArenaGameState::SetScenarioRunIdAuthority(const FString& NewRunId)
{
    if (!HasAuthority() || NewRunId.IsEmpty())
    {
        return false;
    }
    ScenarioRunId = NewRunId;
    OnRep_ScenarioRunId();
    return true;
}

bool AAuthorityArenaGameState::SetRoundNumberAuthority(const int32 NewRoundNumber)
{
    if (!HasAuthority() || NewRoundNumber < 1)
    {
        return false;
    }
    RoundNumber = NewRoundNumber;
    OnRep_RoundNumber();
    return true;
}

bool AAuthorityArenaGameState::SetScenarioStartServerTimeAuthority(const float NewStartTime)
{
    if (!HasAuthority() || !FMath::IsFinite(NewStartTime) || NewStartTime <= 0.0f)
    {
        return false;
    }
    ScenarioStartServerTime = NewStartTime;
    OnRep_ScenarioStartServerTime();
    return true;
}

void AAuthorityArenaGameState::MulticastMatchPulse_Implementation(const FName Pulse)
{
    UAuthorityArenaNetworkDiagnosticsSubsystem::EmitEvent(
        this, TEXT("MatchPulse"), FString::Printf(TEXT("pulse=%s"), *Pulse.ToString()));
}

void AAuthorityArenaGameState::OnRep_MatchPhase()
{
    UAuthorityArenaNetworkDiagnosticsSubsystem::EmitEvent(
        this, TEXT("MatchPhase"), FString::Printf(TEXT("phase=%s"), *MatchPhase.ToString()));
}

void AAuthorityArenaGameState::OnRep_RemainingSeconds()
{
    UAuthorityArenaNetworkDiagnosticsSubsystem::EmitEvent(
        this, TEXT("MatchTime"), FString::Printf(TEXT("remaining=%d"), RemainingSeconds));
}

void AAuthorityArenaGameState::OnRep_ScenarioRunId()
{
    UAuthorityArenaNetworkDiagnosticsSubsystem::EmitEvent(
        this, TEXT("RunIdentity"), FString::Printf(TEXT("run=%s"), *ScenarioRunId));
}

void AAuthorityArenaGameState::OnRep_RoundNumber()
{
    UAuthorityArenaNetworkDiagnosticsSubsystem::EmitEvent(
        this, TEXT("Round"), FString::Printf(TEXT("number=%d"), RoundNumber));
}

void AAuthorityArenaGameState::OnRep_ScenarioStartServerTime()
{
    UAuthorityArenaNetworkDiagnosticsSubsystem::EmitEvent(
        this,
        TEXT("ScenarioStart"),
        FString::Printf(TEXT("server_time=%.3f"), ScenarioStartServerTime));
}
