#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "AuthorityArenaPlayerController.generated.h"

UCLASS()
class AUTHORITYARENA_API AAuthorityArenaPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    UFUNCTION(Server, Reliable)
    void ServerRequestRespawn();

    UFUNCTION(Server, Unreliable)
    void ServerReportViewSample(FRotator ViewRotation, uint32 Sequence);

    UFUNCTION(Client, Reliable)
    void ClientRequestRejected(FName Action, FName Reason);

private:
    uint32 LastAcceptedViewSequence = 0;
};
