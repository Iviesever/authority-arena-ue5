#include "Player/AuthorityArenaPlayerState.h"

#include "Ability/AuthorityArenaAbilitySystemComponent.h"
#include "Ability/AuthorityArenaAttributeSet.h"
#include "Diagnostics/AuthorityArenaNetworkDiagnosticsSubsystem.h"
#include "Net/UnrealNetwork.h"

AAuthorityArenaPlayerState::AAuthorityArenaPlayerState()
{
    bReplicates = true;
    SetNetUpdateFrequency(30.0f);
    AbilitySystemComponent = CreateDefaultSubobject<UAuthorityArenaAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
    AttributeSet = CreateDefaultSubobject<UAuthorityArenaAttributeSet>(TEXT("AttributeSet"));
}

UAbilitySystemComponent* AAuthorityArenaPlayerState::GetAbilitySystemComponent() const
{
    return AbilitySystemComponent;
}

void AAuthorityArenaPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AAuthorityArenaPlayerState, ConnectionId);
    DOREPLIFETIME(AAuthorityArenaPlayerState, DisplayName);
    DOREPLIFETIME(AAuthorityArenaPlayerState, ScoreValue);
    DOREPLIFETIME(AAuthorityArenaPlayerState, DeathCount);
}

bool AAuthorityArenaPlayerState::SetConnectionIdentityAuthority(
    const FString& NewConnectionId,
    const FString& NewDisplayName)
{
    if (!HasAuthority() || NewConnectionId.IsEmpty())
    {
        return false;
    }
    ConnectionId = NewConnectionId.Left(64);
    DisplayName = NewDisplayName.IsEmpty() ? ConnectionId : NewDisplayName.Left(64);
    SetPlayerName(DisplayName);
    OnRep_ConnectionId();
    OnRep_DisplayName();
    return true;
}

bool AAuthorityArenaPlayerState::AddScoreAuthority(const int32 Delta)
{
    if (!HasAuthority() || Delta <= 0)
    {
        return false;
    }
    ScoreValue += Delta;
    OnRep_ScoreValue();
    return true;
}

bool AAuthorityArenaPlayerState::RecordDeathAuthority()
{
    if (!HasAuthority())
    {
        return false;
    }
    ++DeathCount;
    OnRep_DeathCount();
    return true;
}

void AAuthorityArenaPlayerState::OnRep_ConnectionId()
{
    UAuthorityArenaNetworkDiagnosticsSubsystem::EmitEvent(
        this, TEXT("PlayerIdentity"), FString::Printf(TEXT("player=%s"), *ConnectionId));
}

void AAuthorityArenaPlayerState::OnRep_DisplayName()
{
    UAuthorityArenaNetworkDiagnosticsSubsystem::EmitEvent(
        this, TEXT("PlayerName"), FString::Printf(TEXT("name=%s"), *DisplayName));
}

void AAuthorityArenaPlayerState::OnRep_ScoreValue()
{
    UAuthorityArenaNetworkDiagnosticsSubsystem::EmitEvent(
        this, TEXT("Score"), FString::Printf(TEXT("player=%s score=%d"), *ConnectionId, ScoreValue));
}

void AAuthorityArenaPlayerState::OnRep_DeathCount()
{
    UAuthorityArenaNetworkDiagnosticsSubsystem::EmitEvent(
        this, TEXT("Deaths"), FString::Printf(TEXT("player=%s deaths=%d"), *ConnectionId, DeathCount));
}
