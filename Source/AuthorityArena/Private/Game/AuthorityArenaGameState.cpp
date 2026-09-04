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
