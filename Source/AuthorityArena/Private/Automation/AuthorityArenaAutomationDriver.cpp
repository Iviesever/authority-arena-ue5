#include "Automation/AuthorityArenaAutomationDriver.h"

#include "Character/AuthorityArenaCharacter.h"
#include "Diagnostics/AuthorityArenaNetworkDiagnosticsSubsystem.h"
#include "Engine/World.h"
#include "Player/AuthorityArenaPlayerController.h"
#include "Player/AuthorityArenaPlayerState.h"

namespace
{
const TCHAR* DescribeNetMode(const ENetMode NetMode)
{
    switch (NetMode)
    {
    case NM_Standalone: return TEXT("Standalone");
    case NM_DedicatedServer: return TEXT("DedicatedServer");
    case NM_ListenServer: return TEXT("ListenServer");
    case NM_Client: return TEXT("Client");
    default: return TEXT("Unknown");
    }
}
} // namespace

UAuthorityArenaAutomationDriver::UAuthorityArenaAutomationDriver()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
    SetIsReplicatedByDefault(false);
}

void UAuthorityArenaAutomationDriver::BeginPlay()
{
    Super::BeginPlay();
    StartTimeSeconds = GetWorld() != nullptr ? GetWorld()->GetTimeSeconds() : 0.0;
    bAutoMove = FParse::Param(FCommandLine::Get(), TEXT("AuthorityAutoMove"));
    FParse::Value(FCommandLine::Get(), TEXT("AuthorityMoveDuration="), MoveDurationSeconds);
    FParse::Value(FCommandLine::Get(), TEXT("AuthorityExitAfter="), ExitAfterSeconds);
    MoveDurationSeconds = FMath::Clamp(MoveDurationSeconds, 0.25f, 10.0f);
    ExitAfterSeconds = FMath::Clamp(ExitAfterSeconds, 0.0f, 60.0f);
}

void UAuthorityArenaAutomationDriver::TickComponent(
    const float DeltaTime,
    const ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    TryEmitRoleSnapshot();

    const AAuthorityArenaCharacter* Character = Cast<AAuthorityArenaCharacter>(GetOwner());
    if (Character == nullptr || GetWorld() == nullptr || !Character->IsLocallyControlled())
    {
        return;
    }

    const double ElapsedSeconds = GetWorld()->GetTimeSeconds() - StartTimeSeconds;
    TickOwnedClient(DeltaTime, ElapsedSeconds);
}

void UAuthorityArenaAutomationDriver::TryEmitRoleSnapshot()
{
    if (bRoleSnapshotEmitted)
    {
        return;
    }

    const AAuthorityArenaCharacter* Character = Cast<AAuthorityArenaCharacter>(GetOwner());
    const AAuthorityArenaPlayerState* PlayerState =
        Character != nullptr ? Character->GetPlayerState<AAuthorityArenaPlayerState>() : nullptr;
    if (Character == nullptr || PlayerState == nullptr || PlayerState->GetConnectionId().IsEmpty())
    {
        return;
    }

    bRoleSnapshotEmitted = true;
    UAuthorityArenaNetworkDiagnosticsSubsystem::EmitEvent(
        this,
        TEXT("RoleSnapshot"),
        FString::Printf(
            TEXT("player=%s local_role=%s remote_role=%s net_mode=%s"),
            *PlayerState->GetConnectionId(),
            *UAuthorityArenaNetworkDiagnosticsSubsystem::DescribeRole(Character->GetLocalRole()),
            *UAuthorityArenaNetworkDiagnosticsSubsystem::DescribeRole(Character->GetRemoteRole()),
            DescribeNetMode(Character->GetNetMode())));
}

void UAuthorityArenaAutomationDriver::TickOwnedClient(
    const float DeltaTime,
    const double ElapsedSeconds)
{
    AAuthorityArenaCharacter* Character = Cast<AAuthorityArenaCharacter>(GetOwner());
    const AAuthorityArenaPlayerState* PlayerState =
        Character != nullptr ? Character->GetPlayerState<AAuthorityArenaPlayerState>() : nullptr;
    if (Character == nullptr || PlayerState == nullptr)
    {
        return;
    }

    if (bAutoMove && ElapsedSeconds < MoveDurationSeconds)
    {
        if (!bMoveStarted)
        {
            bMoveStarted = true;
            UAuthorityArenaNetworkDiagnosticsSubsystem::EmitEvent(
                this,
                TEXT("AutoMoveStart"),
                FString::Printf(TEXT("player=%s"), *PlayerState->GetConnectionId()));
        }
        Character->AddMovementInput(Character->GetActorForwardVector(), 1.0f);

        ViewSampleAccumulator += DeltaTime;
        if (ViewSampleAccumulator >= 0.25f)
        {
            ViewSampleAccumulator = 0.0f;
            if (AAuthorityArenaPlayerController* Controller =
                    Cast<AAuthorityArenaPlayerController>(Character->GetController()))
            {
                Controller->ServerReportViewSample(Character->GetControlRotation(), ++ViewSequence);
            }
        }
    }
    else if (bAutoMove && !bMoveCompleted)
    {
        bMoveCompleted = true;
        const FVector Location = Character->GetActorLocation();
        UAuthorityArenaNetworkDiagnosticsSubsystem::EmitEvent(
            this,
            TEXT("AutoMoveComplete"),
            FString::Printf(
                TEXT("player=%s x=%.2f y=%.2f z=%.2f"),
                *PlayerState->GetConnectionId(),
                Location.X,
                Location.Y,
                Location.Z));
    }

    if (!bExitRequested &&
        ExitAfterSeconds > 0.0f &&
        ElapsedSeconds >= ExitAfterSeconds &&
        Character->GetNetMode() == NM_Client)
    {
        bExitRequested = true;
        UAuthorityArenaNetworkDiagnosticsSubsystem::EmitEvent(
            this,
            TEXT("ClientScenarioComplete"),
            FString::Printf(TEXT("player=%s"), *PlayerState->GetConnectionId()));
        FGenericPlatformMisc::RequestExit(false);
    }
}
