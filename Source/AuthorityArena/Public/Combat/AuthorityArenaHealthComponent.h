#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AuthorityArenaHealthComponent.generated.h"

UCLASS(ClassGroup = "AuthorityArena", NotBlueprintable)
class AUTHORITYARENA_API UAuthorityArenaHealthComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAuthorityArenaHealthComponent();

    void HandleHealthDepleted(AActor* DamageInstigator);
    bool IsDeathHandled() const { return bDeathHandled; }

private:
    bool bDeathHandled = false;
};
