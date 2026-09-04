#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "AuthorityArenaGameState.generated.h"

UCLASS()
class AUTHORITYARENA_API AAuthorityArenaGameState : public AGameStateBase
{
    GENERATED_BODY()

public:
    AAuthorityArenaGameState();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    FName GetMatchPhase() const { return MatchPhase; }
    int32 GetRemainingSeconds() const { return RemainingSeconds; }
    const FString& GetScenarioRunId() const { return ScenarioRunId; }

    bool SetMatchPhaseAuthority(FName NewPhase);
    bool SetRemainingSecondsAuthority(int32 NewRemainingSeconds);
    bool SetScenarioRunIdAuthority(const FString& NewRunId);

    UFUNCTION(NetMulticast, Unreliable)
    void MulticastMatchPulse(FName Pulse);

private:
    UFUNCTION()
    void OnRep_MatchPhase();

    UFUNCTION()
    void OnRep_RemainingSeconds();

    UFUNCTION()
    void OnRep_ScenarioRunId();

    UPROPERTY(ReplicatedUsing = OnRep_MatchPhase)
    FName MatchPhase = TEXT("Waiting");

    UPROPERTY(ReplicatedUsing = OnRep_RemainingSeconds)
    int32 RemainingSeconds = 180;

    UPROPERTY(ReplicatedUsing = OnRep_ScenarioRunId)
    FString ScenarioRunId;
};
