# Build System

## Verified toolchain

| Tool | Verified version/path | Status |
|---|---|---|
| Unreal Engine | 5.8.0, CL 55116800, `D:\program\UnrealEngine\Epic Games\UE_5.8` | discovered; project build not yet run |
| Visual Studio | Community 2026 18.7.3 | installed |
| MSVC | 14.51.36231 (`cl.exe` 19.51.36248) | installed |
| Windows SDK | 10.0.26100.0 | installed |
| PowerShell | 7.6.0 | verified |
| MQB | 5.4.0, `C:\Users\Iviesever\bin\mqb.exe` | Core build/test verified |
| Git | 2.51.2.windows.1 | verified |

`mqb --version` is not a supported MQB 5.4 option and exits 2. The authoritative local version string is the first line of `mqb --help`.

## MQB-owned target

`scripts/Test-Core.ps1` invokes MQB over the exact source set for `AuthorityArenaCoreTests`. The shared Core sources contain no Unreal headers or types and are also consumed by UBT through the `AuthorityArenaCore` module.

Verified command shape:

```powershell
mqb run Tests/AuthorityArenaCoreTests.cpp `
  Source/AuthorityArenaCore/Private/AuthorityRules.cpp `
  Source/AuthorityArenaCore/Private/NetworkScenario.cpp `
  Source/AuthorityArenaCore/Private/ReportModel.cpp `
  /ISource/AuthorityArenaCore/Public --std 20 /WX
```

MQB 5.4 owns a default `/W3`. Adding raw `/W4` creates MSVC command-line warning D9025 because it overrides the owned default. The evidence path retains MQB's `/W3` and adds `/WX`, producing a clean build where every emitted compiler warning fails the command.

On 2026-09-04 the first test compile failed because the production headers did not exist. After the minimal implementation, 32 assertions passed. A consecutive identical run reported four compile-cache hits, one link-cache hit, and zero misses.

## UE build boundary

Verified on 2026-09-04. A single bounded probe was run from the project root:

```powershell
mqb build Source/AuthorityArena/Private/Game/AuthorityArenaGameMode.cpp `
  --no-discover -ISource/AuthorityArena/Public --std 20
```

MQB exited `1` at the first UE dependency: `CoreMinimal.h` was not found. MQB's public interface does not ingest `.uproject` target rules, UHT generated headers, UBT module macros, Engine include graphs, or UE link dependencies. Hand-authoring UBT's internal response environment would not establish equivalent target ownership, so the bounded probe stopped there.

Verified boundary:

- MQB: shared standard C++ rules tests.
- UBT: Unreal reflection/UHT, Editor/Game/Server targets, generated code, Engine module dependency and target linking.
- RunUAT: BuildCookRun, Cook, Stage, Pak, optional IoStore, and Archive.

This is not a claim that MQB replaces UBT. `scripts/Build.ps1` makes MQB the first local entry for the Core tests, then invokes UBT for an Unreal target.

## Verified Unreal targets

The following current-source commands succeeded with UE 5.8.0, MSVC 14.44.35207 selected by UBT, and Windows SDK 10.0.26100.0:

```powershell
pwsh -NoProfile -File .\scripts\Build.ps1 -Target Editor -Configuration Development
pwsh -NoProfile -File .\scripts\Build.ps1 -Target Game -Configuration Development
pwsh -NoProfile -File .\scripts\Build.ps1 -Target Game -Configuration Shipping
```

The first Editor compile exposed an incorrect `AController*` call to `PlayerCanRestart(APlayerController*)`; after the single API-aligned fix, the identical target succeeded. Final Game builds after the runtime `ArenaReady` marker also succeeded.

The bounded Server target probe:

```powershell
pwsh -NoProfile -File .\scripts\Build.ps1 -Target Server -Configuration Development
```

exited non-zero with UE's exact message:

```text
Server targets are not currently supported from this engine distribution.
```

This Epic-installed distribution therefore uses a separate Game/Editor process with `-server -nullrhi` for server-authoritative multi-process verification. It must not be described as a Dedicated Server target build.

## Runtime smoke boundary

Headless Editor-Cmd and visible Editor game processes both loaded `/Engine/Maps/Entry`, selected `AuthorityArenaGameMode`, emitted `AA_EVENT ArenaReady blocks=6`, and were cleaned up by verified owned PID. The installed NVIDIA 551.61 driver is deny-listed by UE 5.8 for D3D12 and blocked visible startup before engine initialization. The visible smoke therefore uses explicit `-d3d11`; the project default remains D3D12 until packaging validation decides whether that default is viable.
