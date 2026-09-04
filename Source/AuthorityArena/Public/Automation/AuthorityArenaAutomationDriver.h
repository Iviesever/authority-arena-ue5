#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AuthorityArenaAutomationDriver.generated.h"

UCLASS(ClassGroup = "AuthorityArena", NotBlueprintable)
class AUTHORITYARENA_API UAuthorityArenaAutomationDriver : public UActorComponent
{
    GENERATED_BODY()

public:
    UAuthorityArenaAutomationDriver();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(
        float DeltaTime,
        ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

private:
    void TryEmitRoleSnapshot();
    void TickOwnedClient(float DeltaTime, double ElapsedSeconds);
    void TickCombat();

    double StartTimeSeconds = 0.0;
    float MoveDurationSeconds = 2.0f;
    float ExitAfterSeconds = 0.0f;
    float ViewSampleAccumulator = 0.0f;
    uint32 ViewSequence = 0;
    bool bAutoMove = false;
    bool bCombat = false;
    bool bDashOnly = false;
    bool bMoveStarted = false;
    bool bMoveCompleted = false;
    bool bRoleSnapshotEmitted = false;
    bool bExitRequested = false;
    bool bDashRequested = false;
    bool bSecondDashRequested = false;
    bool bShieldRequested = false;
    bool bAttackOneRequested = false;
    bool bAttackTwoRequested = false;
    bool bAttackThreeRequested = false;
    bool bAttackFourRequested = false;
};
