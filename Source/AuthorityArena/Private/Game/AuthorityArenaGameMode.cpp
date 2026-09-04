#include "Game/AuthorityArenaGameMode.h"

#include "AuthorityArena.h"
#include "Character/AuthorityArenaCharacter.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "World/AuthorityArenaWorldBuilder.h"

AAuthorityArenaGameMode::AAuthorityArenaGameMode()
{
    DefaultPawnClass = AAuthorityArenaCharacter::StaticClass();
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
    }
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
