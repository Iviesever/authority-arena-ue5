#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AuthorityArenaGameMode.generated.h"

class AAuthorityArenaPlayerController;

UCLASS()
class AUTHORITYARENA_API AAuthorityArenaGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AAuthorityArenaGameMode();

    virtual void BeginPlay() override;
    virtual FString InitNewPlayer(
        APlayerController* NewPlayerController,
        const FUniqueNetIdRepl& UniqueId,
        const FString& Options,
        const FString& Portal = TEXT("")) override;
    virtual void PostLogin(APlayerController* NewPlayer) override;
    virtual void Logout(AController* Exiting) override;
    virtual void RestartPlayer(AController* NewPlayer) override;

    FTransform ChooseSpawnTransform(int32 PlayerIndex) const;
    void RequestRespawn(AAuthorityArenaPlayerController* Controller);

private:
    void CaptureAutomationSnapshot();
    void ScheduleAutomationLifecycle();
    void DestroyAutomationPawn();
    void FinishAutomationServerRun();

    int32 NextSpawnIndex = 0;
    bool bAutomationSnapshotCaptured = false;
    bool bAutomationLifecycleScheduled = false;
    FTimerHandle AutomationSnapshotTimer;
    FTimerHandle AutomationLifecycleTimer;
    FTimerHandle AutomationExitTimer;
    TMap<TWeakObjectPtr<AAuthorityArenaPlayerController>, FTimerHandle> RespawnTimers;
};
