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
    virtual void RestartPlayer(AController* NewPlayer) override;

    FTransform ChooseSpawnTransform(int32 PlayerIndex) const;

private:
    int32 NextSpawnIndex = 0;
};
