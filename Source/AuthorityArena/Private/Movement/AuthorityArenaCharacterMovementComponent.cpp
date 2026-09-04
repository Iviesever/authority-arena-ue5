#include "Movement/AuthorityArenaCharacterMovementComponent.h"

#include "Diagnostics/AuthorityArenaNetworkDiagnosticsSubsystem.h"

void UAuthorityArenaCharacterMovementComponent::OnClientCorrectionReceived(
    FNetworkPredictionData_Client_Character& ClientData,
    const float TimeStamp,
    const FVector NewLocation,
    const FVector NewVelocity,
    FMovementBaseInterfaceData* NewMovementBaseInterfaceData,
    const FName NewBaseBoneName,
    const bool bHasBase,
    const bool bBaseRelativePosition,
    const uint8 ServerMovementMode,
    const FVector ServerGravityDirection)
{
    Super::OnClientCorrectionReceived(
        ClientData,
        TimeStamp,
        NewLocation,
        NewVelocity,
        NewMovementBaseInterfaceData,
        NewBaseBoneName,
        bHasBase,
        bBaseRelativePosition,
        ServerMovementMode,
        ServerGravityDirection);
    ++ClientCorrectionCount;
    UAuthorityArenaNetworkDiagnosticsSubsystem::EmitEvent(
        this,
        TEXT("MovementCorrection"),
        FString::Printf(TEXT("count=%u timestamp=%.3f"), ClientCorrectionCount, TimeStamp));
}
