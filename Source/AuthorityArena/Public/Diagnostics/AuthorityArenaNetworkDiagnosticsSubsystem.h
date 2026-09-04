#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AuthorityArenaNetworkDiagnosticsSubsystem.generated.h"

UCLASS()
class AUTHORITYARENA_API UAuthorityArenaNetworkDiagnosticsSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    static FString DescribeRole(ENetRole Role);
    static void EmitEvent(const UObject* Context, FName EventName, const FString& Details = FString());
};
