#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Character/AuthorityArenaCharacter.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/TextRenderComponent.h"
#include "Game/AuthorityArenaGameMode.h"
#include "Movement/AuthorityArenaCharacterMovementComponent.h"
#include "UI/AuthorityArenaHUD.h"
#include "World/AuthorityArenaWorldBuilder.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAuthorityArenaHudContractTest,
    "AuthorityArena.UI.HudContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAuthorityArenaHudContractTest::RunTest(const FString& Parameters)
{
    const AAuthorityArenaGameMode* GameMode = GetDefault<AAuthorityArenaGameMode>();
    TestTrue(TEXT("C++ HUD is the default"), GameMode->HUDClass == AAuthorityArenaHUD::StaticClass());
    TestEqual(TEXT("Full percentage"), AAuthorityArenaHUD::FormatPercent(100.0f, 100.0f), FString(TEXT("100%")));
    TestEqual(TEXT("Percentage rounds"), AAuthorityArenaHUD::FormatPercent(49.6f, 100.0f), FString(TEXT("50%")));
    TestEqual(TEXT("Negative percentage clamps"), AAuthorityArenaHUD::FormatPercent(-5.0f, 100.0f), FString(TEXT("0%")));
    TestEqual(TEXT("Invalid max fails closed"), AAuthorityArenaHUD::FormatPercent(10.0f, 0.0f), FString(TEXT("0%")));

    const AAuthorityArenaCharacter* Character = GetDefault<AAuthorityArenaCharacter>();
    const UAuthorityArenaCharacterMovementComponent* Movement =
        Cast<UAuthorityArenaCharacterMovementComponent>(Character->GetCharacterMovement());
    TestNotNull(TEXT("Character uses correction-counting movement component"), Movement);
    TestEqual(TEXT("Correction count starts at zero"), Movement->GetClientCorrectionCount(), 0u);
    TestNotNull(TEXT("Character has a C++ identity label"),
        Character->FindComponentByClass<UTextRenderComponent>());
    TestNotNull(TEXT("Character has a C++ team-color light"),
        Character->FindComponentByClass<UPointLightComponent>());

    const AAuthorityArenaWorldBuilder* WorldBuilder = GetDefault<AAuthorityArenaWorldBuilder>();
    TestNotNull(TEXT("Graybox has programmatic directional light"),
        WorldBuilder->FindComponentByClass<UDirectionalLightComponent>());
    TestNotNull(TEXT("Graybox has programmatic skylight"),
        WorldBuilder->FindComponentByClass<USkyLightComponent>());
    return true;
}

#endif
