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

    UFUNCTION(Server, Reliable)
    void ServerSubmitAuthorityProbe(
        AActor* ClaimedTarget,
        float ClaimedDamage,
        float ClaimedHealth,
        int32 ClaimedScore,
        uint32 Sequence);

    UFUNCTION(Client, Reliable)
    void ClientRequestRejected(FName Action, FName Reason);

private:
    void TickRespawnAutomation();
    void TickAuthorityProbeAutomation();
    void TickClientExitAutomation();
    void TickClientFinalStateAutomation();
    void EmitClientFinalStates() const;

    uint32 LastAcceptedViewSequence = 0;
    uint32 LastAuthorityProbeSequence = 0;
    double LastAuthorityProbeSeconds = 0.0;
    double AutomationStartSeconds = 0.0;
    float AutomationRespawnRequestSeconds = 0.0f;
    float AutomationClientExitSeconds = 0.0f;
    bool bAutomationRespawnRequested = false;
    bool bAutomationSecondRespawnRequested = false;
    bool bDuplicateRespawnAutomation = false;
    bool bAutomationClientExitRequested = false;
    bool bAutomationFinalStatesEmitted = false;
    bool bAuthorityAbuse = false;
    bool bAuthorityFlood = false;
    bool bProbeOneSent = false;
    bool bProbeTwoSent = false;
    bool bProbeThreeSent = false;
    bool bProbeFourSent = false;
    bool bProbeFiveSent = false;
    bool bRespawnPending = false;
};
