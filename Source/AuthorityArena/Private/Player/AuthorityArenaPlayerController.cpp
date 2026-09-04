#include "Player/AuthorityArenaPlayerController.h"

#include "Diagnostics/AuthorityArenaNetworkDiagnosticsSubsystem.h"

void AAuthorityArenaPlayerController::ServerRequestRespawn_Implementation()
{
    if (GetPawn() != nullptr)
    {
        ClientRequestRejected(TEXT("Respawn"), TEXT("NotDead"));
        return;
    }
    ClientRequestRejected(TEXT("Respawn"), TEXT("NotImplementedInPACT10"));
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
