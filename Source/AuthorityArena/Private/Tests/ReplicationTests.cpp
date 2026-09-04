#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Automation/AuthorityArenaAutomationDriver.h"
#include "Character/AuthorityArenaCharacter.h"
#include "Diagnostics/AuthorityArenaNetworkDiagnosticsSubsystem.h"
#include "Game/AuthorityArenaGameState.h"
#include "Player/AuthorityArenaPlayerController.h"
#include "Player/AuthorityArenaPlayerState.h"
#include "UObject/UnrealType.h"

namespace
{
bool IsRepNotifyProperty(const UClass* Class, const FName PropertyName)
{
    const FProperty* Property = FindFProperty<FProperty>(Class, PropertyName);
    return Property != nullptr &&
        Property->HasAnyPropertyFlags(CPF_Net) &&
        Property->RepNotifyFunc != NAME_None;
}

bool HasRpcFlags(const UClass* Class, const FName FunctionName, const EFunctionFlags RequiredFlags)
{
    const UFunction* Function = Class->FindFunctionByName(FunctionName);
    return Function != nullptr && Function->HasAllFunctionFlags(RequiredFlags);
}
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAuthorityArenaReplicationContractTest,
    "AuthorityArena.Network.ReplicationContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAuthorityArenaReplicationContractTest::RunTest(const FString& Parameters)
{
    const AAuthorityArenaGameState* GameState = GetDefault<AAuthorityArenaGameState>();
    TestEqual(TEXT("Default match phase"), GameState->GetMatchPhase(), FName(TEXT("Waiting")));
    TestEqual(TEXT("Default match time"), GameState->GetRemainingSeconds(), 180);
    TestTrue(TEXT("GameState replicates"), GameState->GetIsReplicated());
    TestTrue(TEXT("MatchPhase is RepNotify"), IsRepNotifyProperty(
        AAuthorityArenaGameState::StaticClass(), TEXT("MatchPhase")));
    TestTrue(TEXT("RemainingSeconds is RepNotify"), IsRepNotifyProperty(
        AAuthorityArenaGameState::StaticClass(), TEXT("RemainingSeconds")));
    TestTrue(TEXT("ScenarioRunId is RepNotify"), IsRepNotifyProperty(
        AAuthorityArenaGameState::StaticClass(), TEXT("ScenarioRunId")));

    const AAuthorityArenaPlayerState* PlayerState = GetDefault<AAuthorityArenaPlayerState>();
    TestEqual(TEXT("Default connection id"), PlayerState->GetConnectionId(), FString());
    TestEqual(TEXT("Default score"), PlayerState->GetScoreValue(), 0);
    TestEqual(TEXT("Default death count"), PlayerState->GetDeathCount(), 0);
    TestTrue(TEXT("ConnectionId is RepNotify"), IsRepNotifyProperty(
        AAuthorityArenaPlayerState::StaticClass(), TEXT("ConnectionId")));
    TestTrue(TEXT("ScoreValue is RepNotify"), IsRepNotifyProperty(
        AAuthorityArenaPlayerState::StaticClass(), TEXT("ScoreValue")));
    TestTrue(TEXT("DeathCount is RepNotify"), IsRepNotifyProperty(
        AAuthorityArenaPlayerState::StaticClass(), TEXT("DeathCount")));

    TestTrue(TEXT("Respawn RPC is reliable server RPC"), HasRpcFlags(
        AAuthorityArenaPlayerController::StaticClass(),
        TEXT("ServerRequestRespawn"),
        FUNC_Net | FUNC_NetServer | FUNC_NetReliable));
    TestTrue(TEXT("View sample RPC is unreliable server RPC"), HasRpcFlags(
        AAuthorityArenaPlayerController::StaticClass(),
        TEXT("ServerReportViewSample"),
        FUNC_Net | FUNC_NetServer));
    const UFunction* ViewSample = AAuthorityArenaPlayerController::StaticClass()->FindFunctionByName(
        TEXT("ServerReportViewSample"));
    TestTrue(TEXT("View sample is explicitly not reliable"),
        ViewSample != nullptr && !ViewSample->HasAnyFunctionFlags(FUNC_NetReliable));
    TestTrue(TEXT("Rejection RPC is reliable client RPC"), HasRpcFlags(
        AAuthorityArenaPlayerController::StaticClass(),
        TEXT("ClientRequestRejected"),
        FUNC_Net | FUNC_NetClient | FUNC_NetReliable));

    const AAuthorityArenaCharacter* Character = GetDefault<AAuthorityArenaCharacter>();
    TestTrue(TEXT("Character actor replicates"), Character->GetIsReplicated());
    TestTrue(TEXT("Character movement replicates"), Character->IsReplicatingMovement());
    TestNotNull(
        TEXT("Character owns a C++ automation driver"),
        Character->FindComponentByClass<UAuthorityArenaAutomationDriver>());

    TestEqual(TEXT("Authority role label"),
        UAuthorityArenaNetworkDiagnosticsSubsystem::DescribeRole(ROLE_Authority),
        FString(TEXT("Authority")));
    TestEqual(TEXT("Autonomous role label"),
        UAuthorityArenaNetworkDiagnosticsSubsystem::DescribeRole(ROLE_AutonomousProxy),
        FString(TEXT("AutonomousProxy")));
    TestEqual(TEXT("Simulated role label"),
        UAuthorityArenaNetworkDiagnosticsSubsystem::DescribeRole(ROLE_SimulatedProxy),
        FString(TEXT("SimulatedProxy")));

    return true;
}

#endif
