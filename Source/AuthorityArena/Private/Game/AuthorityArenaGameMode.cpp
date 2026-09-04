#include "Game/AuthorityArenaGameMode.h"

#include "AuthorityArena.h"
#include "Character/AuthorityArenaCharacter.h"
#include "Diagnostics/AuthorityArenaNetworkDiagnosticsSubsystem.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"
#include "Player/AuthorityArenaPlayerController.h"
#include "Player/AuthorityArenaPlayerState.h"
#include "Game/AuthorityArenaGameState.h"
#include "World/AuthorityArenaWorldBuilder.h"
#include "TimerManager.h"

AAuthorityArenaGameMode::AAuthorityArenaGameMode()
{
    DefaultPawnClass = AAuthorityArenaCharacter::StaticClass();
    GameStateClass = AAuthorityArenaGameState::StaticClass();
    PlayerControllerClass = AAuthorityArenaPlayerController::StaticClass();
    PlayerStateClass = AAuthorityArenaPlayerState::StaticClass();
}

void AAuthorityArenaGameMode::BeginPlay()
{
    Super::BeginPlay();

    if (HasAuthority() && GetWorld() != nullptr)
    {
        AAuthorityArenaWorldBuilder* Arena = GetWorld()->SpawnActor<AAuthorityArenaWorldBuilder>(
            AAuthorityArenaWorldBuilder::StaticClass(),
            FTransform::Identity);
        if (IsValid(Arena))
        {
            UE_LOG(LogAuthorityArena, Display, TEXT("AA_EVENT ArenaReady blocks=6"));
        }
        else
        {
            UE_LOG(LogAuthorityArena, Error, TEXT("AA_EVENT ArenaSpawnFailed"));
        }

        FString RunId;
        if (FParse::Value(FCommandLine::Get(), TEXT("AuthorityRunId="), RunId))
        {
            if (AAuthorityArenaGameState* ArenaGameState = GetGameState<AAuthorityArenaGameState>())
            {
                ArenaGameState->SetScenarioRunIdAuthority(RunId);
                ArenaGameState->SetMatchPhaseAuthority(TEXT("Running"));
            }
            UAuthorityArenaNetworkDiagnosticsSubsystem::EmitEvent(
                this,
                TEXT("ServerReady"),
                FString::Printf(TEXT("run=%s"), *RunId));
        }

        float ExitAfterSeconds = 0.0f;
        if (FParse::Value(FCommandLine::Get(), TEXT("AuthorityExitAfter="), ExitAfterSeconds) &&
            ExitAfterSeconds > 0.0f)
        {
            GetWorldTimerManager().SetTimer(
                AutomationExitTimer,
                this,
                &AAuthorityArenaGameMode::FinishAutomationServerRun,
                ExitAfterSeconds,
                false);
        }
    }
}

FString AAuthorityArenaGameMode::InitNewPlayer(
    APlayerController* NewPlayerController,
    const FUniqueNetIdRepl& UniqueId,
    const FString& Options,
    const FString& Portal)
{
    const FString Error = Super::InitNewPlayer(NewPlayerController, UniqueId, Options, Portal);
    if (!Error.IsEmpty() || !IsValid(NewPlayerController))
    {
        return Error;
    }

    FString ConnectionId = UGameplayStatics::ParseOption(Options, TEXT("PlayerId"));
    if (ConnectionId.IsEmpty())
    {
        ConnectionId = FString::Printf(TEXT("Player%d"), GetNumPlayers() + 1);
    }
    if (AAuthorityArenaPlayerState* ArenaPlayerState = NewPlayerController->GetPlayerState<AAuthorityArenaPlayerState>())
    {
        ArenaPlayerState->SetConnectionIdentityAuthority(ConnectionId, ConnectionId);
    }
    return Error;
}

void AAuthorityArenaGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);
    const AAuthorityArenaPlayerState* ArenaPlayerState =
        IsValid(NewPlayer) ? NewPlayer->GetPlayerState<AAuthorityArenaPlayerState>() : nullptr;
    UAuthorityArenaNetworkDiagnosticsSubsystem::EmitEvent(
        this,
        TEXT("PlayerConnected"),
        FString::Printf(
            TEXT("player=%s count=%d"),
            ArenaPlayerState ? *ArenaPlayerState->GetConnectionId() : TEXT("Unknown"),
            GetNumPlayers()));
}

void AAuthorityArenaGameMode::Logout(AController* Exiting)
{
    const AAuthorityArenaPlayerState* ArenaPlayerState =
        IsValid(Exiting) ? Exiting->GetPlayerState<AAuthorityArenaPlayerState>() : nullptr;
    UAuthorityArenaNetworkDiagnosticsSubsystem::EmitEvent(
        this,
        TEXT("PlayerDisconnected"),
        FString::Printf(
            TEXT("player=%s"),
            ArenaPlayerState ? *ArenaPlayerState->GetConnectionId() : TEXT("Unknown")));
    Super::Logout(Exiting);
}

void AAuthorityArenaGameMode::RestartPlayer(AController* NewPlayer)
{
    if (NewPlayer == nullptr || NewPlayer->IsPendingKillPending())
    {
        return;
    }

    const FTransform SpawnTransform = ChooseSpawnTransform(NextSpawnIndex++);
    RestartPlayerAtTransform(NewPlayer, SpawnTransform);
}

FTransform AAuthorityArenaGameMode::ChooseSpawnTransform(const int32 PlayerIndex) const
{
    static const FVector SpawnLocations[] =
    {
        FVector(-600.0, 0.0, 120.0),
        FVector(600.0, 0.0, 120.0),
        FVector(0.0, -600.0, 120.0),
        FVector(0.0, 600.0, 120.0),
    };
    static const FRotator SpawnRotations[] =
    {
        FRotator(0.0, 0.0, 0.0),
        FRotator(0.0, 180.0, 0.0),
        FRotator(0.0, 90.0, 0.0),
        FRotator(0.0, -90.0, 0.0),
    };

    const int32 Index = FMath::Abs(PlayerIndex) % UE_ARRAY_COUNT(SpawnLocations);
    return FTransform(SpawnRotations[Index], SpawnLocations[Index]);
}

void AAuthorityArenaGameMode::FinishAutomationServerRun()
{
    for (TActorIterator<AAuthorityArenaCharacter> It(GetWorld()); It; ++It)
    {
        const AAuthorityArenaCharacter* Character = *It;
        const AAuthorityArenaPlayerState* PlayerState =
            Character->GetPlayerState<AAuthorityArenaPlayerState>();
        const FVector Location = Character->GetActorLocation();
        UAuthorityArenaNetworkDiagnosticsSubsystem::EmitEvent(
            this,
            TEXT("AuthorityPosition"),
            FString::Printf(
                TEXT("player=%s x=%.2f y=%.2f z=%.2f role=%s"),
                PlayerState ? *PlayerState->GetConnectionId() : TEXT("Unknown"),
                Location.X,
                Location.Y,
                Location.Z,
                *UAuthorityArenaNetworkDiagnosticsSubsystem::DescribeRole(Character->GetLocalRole())));
    }
    if (AAuthorityArenaGameState* ArenaGameState = GetGameState<AAuthorityArenaGameState>())
    {
        ArenaGameState->SetMatchPhaseAuthority(TEXT("Complete"));
    }
    UAuthorityArenaNetworkDiagnosticsSubsystem::EmitEvent(this, TEXT("ServerScenarioComplete"));
    FGenericPlatformMisc::RequestExit(false);
}
