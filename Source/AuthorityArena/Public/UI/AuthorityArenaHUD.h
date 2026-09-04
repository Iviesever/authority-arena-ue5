#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "AuthorityArenaHUD.generated.h"

UCLASS(NotBlueprintable)
class AUTHORITYARENA_API AAuthorityArenaHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;

    static FString FormatPercent(float Current, float Maximum);

private:
    void DrawStatBar(
        const FString& Label,
        float Current,
        float Maximum,
        float X,
        float Y,
        const FLinearColor& Color);
};
