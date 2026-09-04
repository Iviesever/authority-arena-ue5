#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AuthorityArenaCharacterMovementComponent.generated.h"

struct FMovementBaseInterfaceData;

UCLASS()
class AUTHORITYARENA_API UAuthorityArenaCharacterMovementComponent : public UCharacterMovementComponent
{
    GENERATED_BODY()

public:
    uint32 GetClientCorrectionCount() const { return ClientCorrectionCount; }

    virtual void OnClientCorrectionReceived(
        FNetworkPredictionData_Client_Character& ClientData,
        float TimeStamp,
        FVector NewLocation,
        FVector NewVelocity,
        FMovementBaseInterfaceData* NewMovementBaseInterfaceData,
        FName NewBaseBoneName,
        bool bHasBase,
        bool bBaseRelativePosition,
        uint8 ServerMovementMode,
        FVector ServerGravityDirection) override;

private:
    uint32 ClientCorrectionCount = 0;
};
