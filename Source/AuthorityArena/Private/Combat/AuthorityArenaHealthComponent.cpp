#include "Combat/AuthorityArenaHealthComponent.h"

#include "Character/AuthorityArenaCharacter.h"
#include "Engine/World.h"
#include "Game/AuthorityArenaGameMode.h"

UAuthorityArenaHealthComponent::UAuthorityArenaHealthComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(false);
}

void UAuthorityArenaHealthComponent::HandleHealthDepleted(AActor* DamageInstigator)
{
    AAuthorityArenaCharacter* Character = Cast<AAuthorityArenaCharacter>(GetOwner());
    if (bDeathHandled || Character == nullptr || !Character->HasAuthority() || GetWorld() == nullptr)
    {
        return;
    }
    bDeathHandled = true;
    if (AAuthorityArenaGameMode* GameMode = GetWorld()->GetAuthGameMode<AAuthorityArenaGameMode>())
    {
        GameMode->HandleCharacterDeath(Character, DamageInstigator);
    }
}
