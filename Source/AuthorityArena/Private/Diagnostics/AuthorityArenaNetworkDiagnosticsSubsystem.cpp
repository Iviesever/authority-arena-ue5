#include "Diagnostics/AuthorityArenaNetworkDiagnosticsSubsystem.h"

#include "AuthorityArena.h"

FString UAuthorityArenaNetworkDiagnosticsSubsystem::DescribeRole(const ENetRole Role)
{
    switch (Role)
    {
    case ROLE_Authority:
        return TEXT("Authority");
    case ROLE_AutonomousProxy:
        return TEXT("AutonomousProxy");
    case ROLE_SimulatedProxy:
        return TEXT("SimulatedProxy");
    case ROLE_None:
    default:
        return TEXT("None");
    }
}

void UAuthorityArenaNetworkDiagnosticsSubsystem::EmitEvent(
    const UObject* Context,
    const FName EventName,
    const FString& Details)
{
    const FString ContextName = GetNameSafe(Context);
    UE_LOG(
        LogAuthorityArena,
        Display,
        TEXT("AA_EVENT event=%s context=%s %s"),
        *EventName.ToString(),
        *ContextName,
        *Details);
}
