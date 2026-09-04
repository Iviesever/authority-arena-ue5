#include "Player/AuthorityArenaPlayerController.h"

#include "Diagnostics/AuthorityArenaNetworkDiagnosticsSubsystem.h"
#include "Engine/World.h"
#include "Game/AuthorityArenaGameMode.h"

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
}

void AAuthorityArenaPlayerController::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bAutomationRespawnRequested ||
        AutomationRespawnRequestSeconds <= 0.0f ||
        GetWorld() == nullptr ||
        GetNetMode() != NM_Client ||
        !IsLocalController())
    {
        return;
    }

    if (GetWorld()->GetTimeSeconds() - AutomationStartSeconds >= AutomationRespawnRequestSeconds)
    {
        bAutomationRespawnRequested = true;
        UAuthorityArenaNetworkDiagnosticsSubsystem::EmitEvent(this, TEXT("RespawnRequested"));
        ServerRequestRespawn();
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

void AAuthorityArenaPlayerController::ClientRequestRejected_Implementation(
    const FName Action,
    const FName Reason)
{
    UAuthorityArenaNetworkDiagnosticsSubsystem::EmitEvent(
        this,
        TEXT("RequestRejected"),
        FString::Printf(TEXT("action=%s reason=%s"), *Action.ToString(), *Reason.ToString()));
}
