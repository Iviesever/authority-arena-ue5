# Build System

## Verified toolchain

| Tool | Verified version/path | Status |
|---|---|---|
| Unreal Engine | 5.8.0, CL 55116800, `D:\program\UnrealEngine\Epic Games\UE_5.8` | Editor/Game build, Automation, Cook and package verified |
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

On 2026-09-04 the first test compile failed because the production headers did not exist. After the minimal implementation, 32 assertions passed. PACT-30 expanded the same suite to 41 assertions for hostile authority probes. Consecutive identical runs report compile/link cache hits; timings remain observational.

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

Headless Editor-Cmd and visible Editor game processes both loaded `/Engine/Maps/Entry`, selected `AuthorityArenaGameMode`, emitted `AA_EVENT ArenaReady blocks=6`, and were cleaned up by verified owned PID. The installed NVIDIA 551.61 driver is deny-listed by UE 5.8 for D3D12 and blocked visible startup before engine initialization. Visible Editor and packaged smoke therefore use explicit `-d3d11`; no D3D12 compatibility claim is made for that host.

## Win64 package boundary

`Package-Win64.ps1` refuses a dirty worktree, runs MQB Core first, and invokes RunUAT with Build/Cook/Stage/Pak/IoStore/Archive. `Verify-PackagedBuild.ps1` rechecks stage booleans, the bootstrap and configuration-specific game EXE, every `.exe/.dll/.pak/.utoc/.ucas` byte count and SHA-256, payload byte total, and an aggregate sorted package fingerprint.

Runtime scripts launch the manifest's configuration-specific `gameExecutable` directly. Holding the root bootstrap PID is insufficient because the bootstrap can create a child game process and exit; direct launch makes the process whose readiness/exit is asserted the same PID/start-time/path identity that cleanup owns.

Both Shipping and Development archives are local-only. Shipping proves the release configuration and real headless/interactive startup. UE 5.8 `GameInstance.cpp` defines `UE_ALLOW_MAP_OVERRIDE_IN_SHIPPING=0`, so a Shipping Game build clears command-line map/connection URLs. Packaged server-plus-two-client automation deliberately uses Development and reports that configuration in JSON; it does not weaken the engine-wide Shipping policy.

RunUAT can start Zen with the Cook process as owner and later sponsor it for Stage. One retained run lost Zen between Cook and Stage and failed; a new-directory rerun exercised `-StartZenServerForStage` and succeeded. Failed archive directories remain ignored evidence and are never overwritten.
