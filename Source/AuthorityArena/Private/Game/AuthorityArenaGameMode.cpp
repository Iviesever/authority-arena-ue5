#include "Game/AuthorityArenaGameMode.h"

#include "AuthorityArena.h"
#include "Ability/AuthorityArenaAbilitySystemComponent.h"
#include "Ability/AuthorityArenaAttributeSet.h"
#include "Ability/AuthorityArenaGameplayTags.h"
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
#include "UI/AuthorityArenaHUD.h"

AAuthorityArenaGameMode::AAuthorityArenaGameMode()
{
    DefaultPawnClass = AAuthorityArenaCharacter::StaticClass();
    GameStateClass = AAuthorityArenaGameState::StaticClass();
    PlayerControllerClass = AAuthorityArenaPlayerController::StaticClass();
    PlayerStateClass = AAuthorityArenaPlayerState::StaticClass();
    HUDClass = AAuthorityArenaHUD::StaticClass();
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
                ArenaGameState->SetRoundNumberAuthority(1);
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

    FString RunId;
    if (GetNumPlayers() >= 2 &&
        FParse::Value(FCommandLine::Get(), TEXT("AuthorityRunId="), RunId) &&
        !GetWorldTimerManager().IsTimerActive(AutomationSnapshotTimer))
    {
        GetWorldTimerManager().SetTimer(
            AutomationSnapshotTimer,
            this,
            &AAuthorityArenaGameMode::CaptureAutomationSnapshot,
            4.0f,
            false);
    }
    if (GetNumPlayers() >= 2)
    {
        if (AAuthorityArenaGameState* ArenaGameState = GetGameState<AAuthorityArenaGameState>())
        {
            if (ArenaGameState->GetScenarioStartServerTime() <= 0.0f)
            {
                ArenaGameState->SetScenarioStartServerTimeAuthority(
                    ArenaGameState->GetServerWorldTimeSeconds() + 1.0f);
            }
            ArenaGameState->MulticastMatchPulse(TEXT("PlayersReady"));
        }
        if (FParse::Param(FCommandLine::Get(), TEXT("AuthorityRejectDash")))
        {
            for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
            {
                AAuthorityArenaPlayerController* Controller =
                    Cast<AAuthorityArenaPlayerController>(It->Get());
                AAuthorityArenaPlayerState* PlayerState =
                    IsValid(Controller) ? Controller->GetPlayerState<AAuthorityArenaPlayerState>() : nullptr;
                if (PlayerState != nullptr && PlayerState->GetConnectionId() == TEXT("Client1"))
                {
                    PlayerState->GetAuthorityAbilitySystem()->ArmNextDashRejectionAuthority();
                    UAuthorityArenaNetworkDiagnosticsSubsystem::EmitEvent(
                        this,
                        TEXT("DashRejectionArmed"),
                        TEXT("player=Client1 reason=Failure.Resource"));
                    break;
                }
            }
        }
        if (FParse::Param(FCommandLine::Get(), TEXT("AuthorityMarkDead")))
        {
            for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
            {
                AAuthorityArenaPlayerController* Controller =
                    Cast<AAuthorityArenaPlayerController>(It->Get());
                AAuthorityArenaPlayerState* PlayerState =
                    IsValid(Controller) ? Controller->GetPlayerState<AAuthorityArenaPlayerState>() : nullptr;
                if (PlayerState != nullptr && PlayerState->GetConnectionId() == TEXT("Client1"))
                {
                    UAuthorityArenaAbilitySystemComponent* AbilitySystem =
                        PlayerState->GetAuthorityAbilitySystem();
                    AbilitySystem->SetNumericAttributeBase(
                        UAuthorityArenaAttributeSet::GetHealthAttribute(), 0.0f);
                    AbilitySystem->AddLooseGameplayTag(AuthorityArenaTags::State_Dead);
                    UAuthorityArenaNetworkDiagnosticsSubsystem::EmitEvent(
                        this, TEXT("DeadGateArmed"), TEXT("player=Client1 health=0 tag=State.Dead"));
                    break;
                }
            }
        }
        ScheduleAutomationLifecycle();
    }
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
    if (AAuthorityArenaPlayerController* ArenaController = Cast<AAuthorityArenaPlayerController>(Exiting))
    {
        const TWeakObjectPtr<AAuthorityArenaPlayerController> WeakController(ArenaController);
        if (FTimerHandle* Timer = RespawnTimers.Find(WeakController))
        {
            GetWorldTimerManager().ClearTimer(*Timer);
            RespawnTimers.Remove(WeakController);
        }
        ArenaController->ClearRespawnPendingAuthority();
    }
    Super::Logout(Exiting);
}

void AAuthorityArenaGameMode::RestartPlayer(AController* NewPlayer)
{
    if (NewPlayer == nullptr || NewPlayer->IsPendingKillPending())
    {
        return;
    }

    int32 SpawnIndex = NextSpawnIndex++;
    if (const AAuthorityArenaPlayerState* PlayerState =
            NewPlayer->GetPlayerState<AAuthorityArenaPlayerState>())
    {
        if (PlayerState->GetConnectionId() == TEXT("Client1"))
        {
            SpawnIndex = 0;
        }
        else if (PlayerState->GetConnectionId() == TEXT("Client2"))
        {
            SpawnIndex = 1;
        }
    }
    const FTransform SpawnTransform = ChooseSpawnTransform(SpawnIndex);
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

void AAuthorityArenaGameMode::RequestRespawn(AAuthorityArenaPlayerController* Controller)
{
    if (!IsValid(Controller) || Controller->GetPawn() != nullptr)
    {
        if (IsValid(Controller))
        {
            UAuthorityArenaNetworkDiagnosticsSubsystem::EmitEvent(
                this, TEXT("RespawnRejected"), TEXT("reason=NotDead"));
            Controller->ClientRequestRejected(TEXT("Respawn"), TEXT("NotDead"));
        }
        return;
    }
    if (!Controller->TryMarkRespawnPendingAuthority())
    {
        UAuthorityArenaNetworkDiagnosticsSubsystem::EmitEvent(
            this, TEXT("RespawnRejected"), TEXT("reason=RespawnPending"));
        Controller->ClientRequestRejected(TEXT("Respawn"), TEXT("RespawnPending"));
        return;
    }

    const TWeakObjectPtr<AAuthorityArenaPlayerController> WeakController(Controller);
    FTimerHandle& Timer = RespawnTimers.FindOrAdd(WeakController);
    GetWorldTimerManager().SetTimer(
        Timer,
        FTimerDelegate::CreateWeakLambda(this, [this, WeakController]()
        {
            AAuthorityArenaPlayerController* StrongController = WeakController.Get();
            RespawnTimers.Remove(WeakController);
            if (!IsValid(StrongController))
            {
                return;
            }
            if (AAuthorityArenaPlayerState* MutablePlayerState =
                    StrongController->GetPlayerState<AAuthorityArenaPlayerState>())
            {
                if (UAuthorityArenaAbilitySystemComponent* AbilitySystem =
                        MutablePlayerState->GetAuthorityAbilitySystem())
                {
                    AbilitySystem->RemoveLooseGameplayTag(AuthorityArenaTags::State_Dead);
                    AbilitySystem->SetNumericAttributeBase(
                        UAuthorityArenaAttributeSet::GetHealthAttribute(),
                        MutablePlayerState->GetAuthorityAttributeSet()->GetMaxHealth());
                    AbilitySystem->SetNumericAttributeBase(
                        UAuthorityArenaAttributeSet::GetEnergyAttribute(),
                        MutablePlayerState->GetAuthorityAttributeSet()->GetMaxEnergy());
                }
            }
            StrongController->ClearRespawnPendingAuthority();
            RestartPlayer(StrongController);
            const AAuthorityArenaPlayerState* PlayerState =
                StrongController->GetPlayerState<AAuthorityArenaPlayerState>();
            UAuthorityArenaNetworkDiagnosticsSubsystem::EmitEvent(
                this,
                TEXT("PawnRespawned"),
                FString::Printf(
                    TEXT("player=%s pawn=%s"),
                    PlayerState ? *PlayerState->GetConnectionId() : TEXT("Unknown"),
                    *GetNameSafe(StrongController->GetPawn())));
            CaptureAutomationSnapshot();
        }),
        1.0f,
        false);
}

void AAuthorityArenaGameMode::HandleCharacterDeath(
    AAuthorityArenaCharacter* Character,
    AActor* DamageInstigator)
{
    if (!IsValid(Character) || !Character->HasAuthority())
    {
        return;
    }
    AAuthorityArenaPlayerController* VictimController =
        Cast<AAuthorityArenaPlayerController>(Character->GetController());
    AAuthorityArenaPlayerState* VictimState =
        IsValid(VictimController) ? VictimController->GetPlayerState<AAuthorityArenaPlayerState>() : nullptr;
    if (VictimState != nullptr)
    {
        VictimState->RecordDeathAuthority();
        if (UAuthorityArenaAbilitySystemComponent* AbilitySystem = VictimState->GetAuthorityAbilitySystem())
        {
            AbilitySystem->AddLooseGameplayTag(AuthorityArenaTags::State_Dead);
            AbilitySystem->CancelAllAbilities();
        }
    }

    const AAuthorityArenaCharacter* InstigatorCharacter = Cast<AAuthorityArenaCharacter>(DamageInstigator);
    AAuthorityArenaPlayerState* InstigatorState =
        InstigatorCharacter != nullptr
            ? InstigatorCharacter->GetPlayerState<AAuthorityArenaPlayerState>()
            : nullptr;
    if (InstigatorState != nullptr && InstigatorState != VictimState)
    {
        InstigatorState->AddScoreAuthority();
    }

    UAuthorityArenaNetworkDiagnosticsSubsystem::EmitEvent(
        this,
        TEXT("Death"),
        FString::Printf(
            TEXT("victim=%s instigator=%s"),
            VictimState ? *VictimState->GetConnectionId() : TEXT("Unknown"),
            InstigatorState ? *InstigatorState->GetConnectionId() : TEXT("World")));

    if (VictimController != nullptr)
    {
        VictimController->UnPossess();
    }
    Character->Destroy();
    if (VictimController != nullptr)
    {
        RequestRespawn(VictimController);
    }
}

void AAuthorityArenaGameMode::ScheduleAutomationLifecycle()
{
    if (bAutomationLifecycleScheduled ||
        !FParse::Param(FCommandLine::Get(), TEXT("AuthorityLifecycle")))
    {
        return;
    }
    bAutomationLifecycleScheduled = true;
    GetWorldTimerManager().SetTimer(
        AutomationLifecycleTimer,
        this,
        &AAuthorityArenaGameMode::DestroyAutomationPawn,
        1.5f,
        false);
}

void AAuthorityArenaGameMode::DestroyAutomationPawn()
{
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        AAuthorityArenaPlayerController* Controller = Cast<AAuthorityArenaPlayerController>(It->Get());
        AAuthorityArenaPlayerState* PlayerState =
            IsValid(Controller) ? Controller->GetPlayerState<AAuthorityArenaPlayerState>() : nullptr;
        if (!IsValid(PlayerState) || PlayerState->GetConnectionId() != TEXT("Client2"))
        {
            continue;
        }

        APawn* Pawn = Controller->GetPawn();
        if (!IsValid(Pawn))
        {
            return;
        }
        const FString PawnName = Pawn->GetName();
        Controller->UnPossess();
        Pawn->Destroy();
        UAuthorityArenaNetworkDiagnosticsSubsystem::EmitEvent(
            this,
            TEXT("PawnDestroyed"),
            FString::Printf(TEXT("player=Client2 pawn=%s"), *PawnName));
        return;
    }
}

void AAuthorityArenaGameMode::CaptureAutomationSnapshot()
{
    int32 SnapshotCount = 0;
    for (TActorIterator<AAuthorityArenaCharacter> It(GetWorld()); It; ++It)
    {
        const AAuthorityArenaCharacter* Character = *It;
        const AAuthorityArenaPlayerState* PlayerState =
            Character->GetPlayerState<AAuthorityArenaPlayerState>();
        const UAuthorityArenaAttributeSet* Attributes =
            PlayerState != nullptr ? PlayerState->GetAuthorityAttributeSet() : nullptr;
        const UAuthorityArenaAbilitySystemComponent* AbilitySystem =
            PlayerState != nullptr ? PlayerState->GetAuthorityAbilitySystem() : nullptr;
        const FVector Location = Character->GetActorLocation();
        UAuthorityArenaNetworkDiagnosticsSubsystem::EmitEvent(
            this,
            TEXT("AuthorityPosition"),
            FString::Printf(
                TEXT("player=%s x=%.2f y=%.2f z=%.2f role=%s health=%.2f energy=%.2f score=%d deaths=%d shield=%s dead=%s"),
                PlayerState ? *PlayerState->GetConnectionId() : TEXT("Unknown"),
                Location.X,
                Location.Y,
                Location.Z,
                *UAuthorityArenaNetworkDiagnosticsSubsystem::DescribeRole(Character->GetLocalRole()),
                Attributes ? Attributes->GetHealth() : -1.0f,
                Attributes ? Attributes->GetEnergy() : -1.0f,
                PlayerState ? PlayerState->GetScoreValue() : -1,
                PlayerState ? PlayerState->GetDeathCount() : -1,
                AbilitySystem && AbilitySystem->HasMatchingGameplayTag(AuthorityArenaTags::State_Shield_Active)
                    ? TEXT("true") : TEXT("false"),
                AbilitySystem && AbilitySystem->HasMatchingGameplayTag(AuthorityArenaTags::State_Dead)
                    ? TEXT("true") : TEXT("false")));
        ++SnapshotCount;
    }
    bAutomationSnapshotCaptured = SnapshotCount >= 2;
    UAuthorityArenaNetworkDiagnosticsSubsystem::EmitEvent(
        this,
        TEXT("AuthoritySnapshotComplete"),
        FString::Printf(TEXT("count=%d"), SnapshotCount));
}

void AAuthorityArenaGameMode::FinishAutomationServerRun()
{
    if (!bAutomationSnapshotCaptured)
    {
        CaptureAutomationSnapshot();
    }
    if (AAuthorityArenaGameState* ArenaGameState = GetGameState<AAuthorityArenaGameState>())
    {
        ArenaGameState->SetMatchPhaseAuthority(TEXT("Complete"));
    }
    UAuthorityArenaNetworkDiagnosticsSubsystem::EmitEvent(this, TEXT("ServerScenarioComplete"));
    FGenericPlatformMisc::RequestExit(false);
}
