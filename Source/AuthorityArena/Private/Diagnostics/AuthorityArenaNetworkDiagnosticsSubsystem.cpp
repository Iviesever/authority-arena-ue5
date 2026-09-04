#include "Diagnostics/AuthorityArenaNetworkDiagnosticsSubsystem.h"

#include "AuthorityArena.h"
#include "Dom/JsonObject.h"
#include "HAL/PlatformProcess.h"
#include "Misc/FileHelper.h"
#include "Misc/ScopeLock.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
FCriticalSection EventStreamMutex;
FCriticalSection RecentEventMutex;
uint64 EventSequence = 0;
bool bEventStreamInitialized = false;
FString EventStreamPath;
FString EventRunId;
FString EventProcessRole;
TArray<FString> RecentEvents;

void InitializeEventStream()
{
    if (bEventStreamInitialized)
    {
        return;
    }
    bEventStreamInitialized = true;
    FParse::Value(FCommandLine::Get(), TEXT("AuthorityEventLog="), EventStreamPath);
    FParse::Value(FCommandLine::Get(), TEXT("AuthorityRunId="), EventRunId);
    FParse::Value(FCommandLine::Get(), TEXT("AuthorityProcessRole="), EventProcessRole);
}

void AppendStructuredEvent(
    const FString& ContextName,
    const FName EventName,
    const FString& Details)
{
    FScopeLock Lock(&EventStreamMutex);
    InitializeEventStream();
    if (EventStreamPath.IsEmpty())
    {
        return;
    }

    TSharedRef<FJsonObject> Event = MakeShared<FJsonObject>();
    Event->SetNumberField(TEXT("schemaVersion"), 1);
    Event->SetStringField(TEXT("runId"), EventRunId);
    Event->SetStringField(TEXT("processRole"), EventProcessRole);
    Event->SetNumberField(TEXT("pid"), FPlatformProcess::GetCurrentProcessId());
    Event->SetNumberField(TEXT("sequence"), ++EventSequence);
    Event->SetStringField(TEXT("utc"), FDateTime::UtcNow().ToIso8601());
    Event->SetStringField(TEXT("event"), EventName.ToString());
    Event->SetStringField(TEXT("context"), ContextName);
    Event->SetStringField(TEXT("details"), Details);

    FString Line;
    const TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
        TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Line);
    if (!FJsonSerializer::Serialize(Event, Writer))
    {
        return;
    }
    Line.Append(LINE_TERMINATOR);
    FFileHelper::SaveStringToFile(
        Line,
        *EventStreamPath,
        FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,
        &IFileManager::Get(),
        FILEWRITE_Append);
}
} // namespace

FString UAuthorityArenaNetworkDiagnosticsSubsystem::DescribeRole(const ENetRole Role)
{
    switch (Role)
    {
    case ROLE_Authority:
        return TEXT("Authority");
    case ROLE_AutonomousProxy:
        return TEXT("AutonomousProxy");
    case ROLE_SimulatedProxy:
        return TEXT("SimulatedProxy");
    case ROLE_None:
    default:
        return TEXT("None");
    }
}

void UAuthorityArenaNetworkDiagnosticsSubsystem::EmitEvent(
    const UObject* Context,
    const FName EventName,
    const FString& Details)
{
    const FString ContextName = GetNameSafe(Context);
    UE_LOG(
        LogAuthorityArena,
        Display,
        TEXT("AA_EVENT event=%s context=%s %s"),
        *EventName.ToString(),
        *ContextName,
        *Details);
    {
        FScopeLock Lock(&RecentEventMutex);
        RecentEvents.Add(FString::Printf(TEXT("%s %s"), *EventName.ToString(), *Details));
        constexpr int32 MaximumRecentEvents = 6;
        if (RecentEvents.Num() > MaximumRecentEvents)
        {
            RecentEvents.RemoveAt(0, RecentEvents.Num() - MaximumRecentEvents, EAllowShrinking::No);
        }
    }
    AppendStructuredEvent(ContextName, EventName, Details);
}

TArray<FString> UAuthorityArenaNetworkDiagnosticsSubsystem::GetRecentEvents()
{
    FScopeLock Lock(&RecentEventMutex);
    return RecentEvents;
}
