#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "AuthorityArenaGameInstance.generated.h"

class UNetDriver;

UCLASS()
class AUTHORITYARENA_API UAuthorityArenaGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    virtual void Init() override;
    virtual void Shutdown() override;

private:
    void OnNetworkFailure(
        UWorld* World,
        UNetDriver* NetDriver,
        ENetworkFailure::Type FailureType,
        const FString& ErrorString);
};
