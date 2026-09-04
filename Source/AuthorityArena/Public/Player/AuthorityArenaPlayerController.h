#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "AuthorityArenaPlayerController.generated.h"

UCLASS()
class AUTHORITYARENA_API AAuthorityArenaPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    AAuthorityArenaPlayerController();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    bool IsRespawnPending() const { return bRespawnPending; }
    bool TryMarkRespawnPendingAuthority();
    void ClearRespawnPendingAuthority();

    UFUNCTION(Server, Reliable)
    void ServerRequestRespawn();

    UFUNCTION(Server, Unreliable)
    void ServerReportViewSample(FRotator ViewRotation, uint32 Sequence);

    UFUNCTION(Client, Reliable)
    void ClientRequestRejected(FName Action, FName Reason);

private:
    uint32 LastAcceptedViewSequence = 0;
    double AutomationStartSeconds = 0.0;
    float AutomationRespawnRequestSeconds = 0.0f;
    bool bAutomationRespawnRequested = false;
    bool bRespawnPending = false;
};
