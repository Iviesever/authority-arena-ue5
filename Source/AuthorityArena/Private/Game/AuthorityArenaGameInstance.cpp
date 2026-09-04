#include "Game/AuthorityArenaGameInstance.h"

#include "Diagnostics/AuthorityArenaNetworkDiagnosticsSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

void UAuthorityArenaGameInstance::Init()
{
    Super::Init();
    if (GEngine != nullptr)
    {
        GEngine->OnNetworkFailure().AddUObject(
            this, &UAuthorityArenaGameInstance::OnNetworkFailure);
    }
}

void UAuthorityArenaGameInstance::Shutdown()
{
    if (GEngine != nullptr)
    {
        GEngine->OnNetworkFailure().RemoveAll(this);
    }
    Super::Shutdown();
}

void UAuthorityArenaGameInstance::OnNetworkFailure(
    UWorld* World,
    UNetDriver* NetDriver,
    const ENetworkFailure::Type FailureType,
    const FString& ErrorString)
{
    if (World != nullptr && World->GetGameInstance() != this)
    {
        return;
    }
    UAuthorityArenaNetworkDiagnosticsSubsystem::EmitEvent(
        this,
        TEXT("NetworkFailure"),
        FString::Printf(
            TEXT("type=%d error=%s"),
            static_cast<int32>(FailureType),
            *ErrorString.Replace(TEXT(" "), TEXT("_"))));

    if (FParse::Param(FCommandLine::Get(), TEXT("AuthorityExitOnNetworkFailure")))
    {
        FGenericPlatformMisc::RequestExit(false);
    }
}
