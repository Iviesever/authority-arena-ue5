#include "UI/AuthorityArenaHUD.h"

#include "Ability/AuthorityArenaAbilitySystemComponent.h"
#include "Ability/AuthorityArenaAttributeSet.h"
#include "Ability/AuthorityArenaGameplayTags.h"
#include "Character/AuthorityArenaCharacter.h"
#include "Diagnostics/AuthorityArenaNetworkDiagnosticsSubsystem.h"
#include "Engine/Canvas.h"
#include "Movement/AuthorityArenaCharacterMovementComponent.h"
#include "Player/AuthorityArenaPlayerState.h"

FString AAuthorityArenaHUD::FormatPercent(const float Current, const float Maximum)
{
    if (!FMath::IsFinite(Current) || !FMath::IsFinite(Maximum) || Maximum <= 0.0f)
    {
        return TEXT("0%");
    }
    return FString::Printf(TEXT("%d%%"), FMath::RoundToInt(FMath::Clamp(Current / Maximum, 0.0f, 1.0f) * 100.0f));
}

void AAuthorityArenaHUD::DrawHUD()
{
    Super::DrawHUD();
    if (Canvas == nullptr)
    {
        return;
    }

    APlayerController* Controller = GetOwningPlayerController();
    AAuthorityArenaCharacter* Character =
        Controller != nullptr ? Cast<AAuthorityArenaCharacter>(Controller->GetPawn()) : nullptr;
    AAuthorityArenaPlayerState* PlayerState =
        Controller != nullptr ? Controller->GetPlayerState<AAuthorityArenaPlayerState>() : nullptr;
    UAuthorityArenaAbilitySystemComponent* AbilitySystem =
        PlayerState != nullptr ? PlayerState->GetAuthorityAbilitySystem() : nullptr;
    UAuthorityArenaAttributeSet* Attributes =
        PlayerState != nullptr ? PlayerState->GetAuthorityAttributeSet() : nullptr;

    constexpr float PanelX = 18.0f;
    constexpr float PanelY = 18.0f;
    constexpr float PanelWidth = 460.0f;
    constexpr float PanelHeight = 390.0f;
    DrawRect(FLinearColor(0.015f, 0.02f, 0.035f, 0.88f), PanelX, PanelY, PanelWidth, PanelHeight);

    float Y = PanelY + 14.0f;
    const FLinearColor Primary(0.30f, 0.85f, 1.0f, 1.0f);
    const FLinearColor Text(0.90f, 0.94f, 1.0f, 1.0f);
    DrawText(TEXT("AUTHORITY ARENA // SERVER-AUTH LAB"), Primary, PanelX + 14.0f, Y, nullptr, 1.12f);
    Y += 30.0f;

    const FString PlayerId = PlayerState != nullptr ? PlayerState->GetConnectionId() : TEXT("Connecting");
    const FString LocalRole = Character != nullptr
        ? UAuthorityArenaNetworkDiagnosticsSubsystem::DescribeRole(Character->GetLocalRole())
        : TEXT("NoPawn");
    const FString RemoteRoleLabel = Character != nullptr
        ? UAuthorityArenaNetworkDiagnosticsSubsystem::DescribeRole(Character->GetRemoteRole())
        : TEXT("NoPawn");
    DrawText(FString::Printf(TEXT("Player: %s   Local: %s   Remote: %s"), *PlayerId, *LocalRole, *RemoteRoleLabel),
        Text, PanelX + 14.0f, Y);
    Y += 24.0f;

    const float Health = Attributes != nullptr ? Attributes->GetHealth() : 0.0f;
    const float MaxHealth = Attributes != nullptr ? Attributes->GetMaxHealth() : 100.0f;
    const float Energy = Attributes != nullptr ? Attributes->GetEnergy() : 0.0f;
    const float MaxEnergy = Attributes != nullptr ? Attributes->GetMaxEnergy() : 100.0f;
    DrawStatBar(TEXT("HEALTH"), Health, MaxHealth, PanelX + 14.0f, Y, FLinearColor(0.95f, 0.18f, 0.22f));
    Y += 35.0f;
    DrawStatBar(TEXT("ENERGY"), Energy, MaxEnergy, PanelX + 14.0f, Y, FLinearColor(0.10f, 0.72f, 1.0f));
    Y += 39.0f;

    const float DashCooldown = AbilitySystem != nullptr
        ? AbilitySystem->GetCooldownRemainingForTag(AuthorityArenaTags::Cooldown_Dash) : 0.0f;
    const float AttackCooldown = AbilitySystem != nullptr
        ? AbilitySystem->GetCooldownRemainingForTag(AuthorityArenaTags::Cooldown_Attack) : 0.0f;
    const float ShieldCooldown = AbilitySystem != nullptr
        ? AbilitySystem->GetCooldownRemainingForTag(AuthorityArenaTags::Cooldown_Shield) : 0.0f;
    const bool bShield = AbilitySystem != nullptr &&
        AbilitySystem->HasMatchingGameplayTag(AuthorityArenaTags::State_Shield_Active);
    const bool bDead = AbilitySystem != nullptr &&
        AbilitySystem->HasMatchingGameplayTag(AuthorityArenaTags::State_Dead);
    DrawText(FString::Printf(TEXT("Cooldowns  Dash %.1fs | Attack %.1fs | Shield %.1fs"),
        DashCooldown, AttackCooldown, ShieldCooldown), Text, PanelX + 14.0f, Y);
    Y += 22.0f;
    DrawText(FString::Printf(TEXT("State  Shield: %s   Dead: %s   Score: %d   Deaths: %d"),
        bShield ? TEXT("ACTIVE") : TEXT("off"),
        bDead ? TEXT("YES") : TEXT("no"),
        PlayerState ? PlayerState->GetScoreValue() : 0,
        PlayerState ? PlayerState->GetDeathCount() : 0),
        bDead ? FLinearColor::Red : Text, PanelX + 14.0f, Y);
    Y += 25.0f;

    int32 ConfiguredLag = 0;
    int32 ConfiguredLoss = 0;
    FParse::Value(FCommandLine::Get(), TEXT("PktLag="), ConfiguredLag);
    FParse::Value(FCommandLine::Get(), TEXT("PktLoss="), ConfiguredLoss);
    const float PingMs = PlayerState != nullptr ? PlayerState->GetPingInMilliseconds() : 0.0f;
    const UAuthorityArenaCharacterMovementComponent* Movement = Character != nullptr
        ? Cast<UAuthorityArenaCharacterMovementComponent>(Character->GetCharacterMovement()) : nullptr;
    DrawText(FString::Printf(TEXT("Network  Ping %.0fms | Config lag %dms | Loss %d%% | Corrections %u"),
        PingMs,
        ConfiguredLag,
        ConfiguredLoss,
        Movement ? Movement->GetClientCorrectionCount() : 0),
        FLinearColor(0.68f, 0.82f, 1.0f), PanelX + 14.0f, Y);
    Y += 28.0f;

    DrawText(TEXT("Recent authoritative / predicted events"), Primary, PanelX + 14.0f, Y);
    Y += 20.0f;
    for (FString Event : UAuthorityArenaNetworkDiagnosticsSubsystem::GetRecentEvents())
    {
        Event.LeftInline(70);
        DrawText(FString::Printf(TEXT("- %s"), *Event), FLinearColor(0.76f, 0.80f, 0.88f), PanelX + 18.0f, Y, nullptr, 0.82f);
        Y += 18.0f;
    }
}

void AAuthorityArenaHUD::DrawStatBar(
    const FString& Label,
    const float Current,
    const float Maximum,
    const float X,
    const float Y,
    const FLinearColor& Color)
{
    constexpr float Width = 420.0f;
    constexpr float Height = 14.0f;
    const float Ratio = Maximum > 0.0f ? FMath::Clamp(Current / Maximum, 0.0f, 1.0f) : 0.0f;
    DrawText(FString::Printf(TEXT("%s  %.0f / %.0f  %s"), *Label, Current, Maximum, *FormatPercent(Current, Maximum)),
        FLinearColor::White, X, Y);
    DrawRect(FLinearColor(0.12f, 0.14f, 0.20f, 1.0f), X, Y + 18.0f, Width, Height);
    DrawRect(Color, X, Y + 18.0f, Width * Ratio, Height);
}
