#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AuthorityArenaGameMode.generated.h"

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

private:
    void FinishAutomationServerRun();

    int32 NextSpawnIndex = 0;
    FTimerHandle AutomationExitTimer;
};
