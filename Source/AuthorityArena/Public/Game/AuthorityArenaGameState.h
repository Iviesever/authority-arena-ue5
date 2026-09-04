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
    int32 GetRoundNumber() const { return RoundNumber; }
    float GetScenarioStartServerTime() const { return ScenarioStartServerTime; }
    const FString& GetScenarioRunId() const { return ScenarioRunId; }

    bool SetMatchPhaseAuthority(FName NewPhase);
    bool SetRemainingSecondsAuthority(int32 NewRemainingSeconds);
    bool SetRoundNumberAuthority(int32 NewRoundNumber);
    bool SetScenarioStartServerTimeAuthority(float NewStartTime);
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

    UFUNCTION()
    void OnRep_RoundNumber();

    UFUNCTION()
    void OnRep_ScenarioStartServerTime();

    UPROPERTY(ReplicatedUsing = OnRep_MatchPhase)
    FName MatchPhase = TEXT("Waiting");

    UPROPERTY(ReplicatedUsing = OnRep_RemainingSeconds)
    int32 RemainingSeconds = 180;

    UPROPERTY(ReplicatedUsing = OnRep_ScenarioRunId)
    FString ScenarioRunId;

    UPROPERTY(ReplicatedUsing = OnRep_RoundNumber)
    int32 RoundNumber = 1;

    UPROPERTY(ReplicatedUsing = OnRep_ScenarioStartServerTime)
    float ScenarioStartServerTime = 0.0f;
};
