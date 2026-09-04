# Real Multi-process Testing

## Entry points

```powershell
pwsh -NoProfile -File .\scripts\RunMultiplayerScenario.ps1 `
  -Scenario Combat -NetworkProfile Lag60

pwsh -NoProfile -File .\scripts\Invoke-NetworkMatrix.ps1
pwsh -NoProfile -File .\scripts\Invoke-FailureMatrix.ps1
pwsh -NoProfile -File .\scripts\tests\RunMultiplayerScenario.Tests.ps1

$developmentManifest = 'D:\local\Artifacts\package\development\package-manifest.json'
pwsh -NoProfile -File .\scripts\RunMultiplayerScenario.ps1 -Build Packaged `
  -PackageManifest $developmentManifest -Scenario Combat
```

Every run discovers UE 5.8, reserves a temporary UDP port, creates a 32-character RunId and unique ignored artifact directory, then launches exactly one server and two independent client processes. Because the installed Epic distribution cannot build `TargetType.Server`, the server is a separate `UnrealEditor-Cmd -server -nullrhi` process; runtime net mode is dedicated, but no Dedicated Server target binary is claimed.

For packaged E2E, the manifest must identify a Development build. UE Shipping clears command-line map/URL overrides under its default security macro, so the runner rejects a Shipping manifest before launch. Development Game produces a ListenServer host; `-AuthoritySuppressHostPawn` removes only the automation host Pawn, leaving the server process plus Client1 and Client2 as the tested combat actors. Package configuration and payload hashes are recorded in the report.

## Process ownership and cleanup

Each owned process record contains role, PID, start time and resolved executable path. Cleanup re-reads all three identity properties before calling `Stop-Process`; any mismatch fails closed instead of broadening the target. Normal scenarios exit by C++ timers. Network failures exit through `UAuthorityArenaGameInstance`'s C++ subscription to `GEngine->OnNetworkFailure`. `finally` performs reverse-order cleanup only for identities still owned by the run.

No command kills by image name. Other projects' UE/UBT/Automation processes are not stopped or restarted.

## Readiness and clocks

The runner waits for the actual UE 5.8 `IpNetDriver listening on port` text and the project `ServerReady` event before launching clients. Clients must observe their AutonomousProxy and their peer's SimulatedProxy where the scenario expects two connections. Gameplay actions are scheduled from replicated `ScenarioStartServerTime` using `AGameStateBase::GetServerWorldTimeSeconds`, not from independent process startup clocks.

All waits have one absolute deadline. A missing marker, early unexpected exit, invalid report, non-zero exit or deadline failure makes the command non-zero and preserves logs.

## Structured event streams

Each process receives a separate `-AuthorityEventLog=<path>` and `-AuthorityProcessRole=<role>`. `UAuthorityArenaNetworkDiagnosticsSubsystem` appends condensed JSONL under a process-local lock:

```json
{"schemaVersion":1,"runId":"...","processRole":"Server","pid":1234,"sequence":1,"utc":"...Z","event":"ServerReady","context":"AuthorityArenaGameMode_0","details":"run=..."}
```

The runner parses every non-empty line, checks schema 1, exact RunId/role and strictly increasing sequence, and records the three event counts in `report.json`. Text logs remain the detailed UE diagnostic source; JSONL is the stable machine-readable event contract.

Shipping does not enable general logs in this installed shared-engine build. Packaged readiness and assertions therefore consume JSONL directly; object numeric suffixes are normalized only in the synthetic assertion text, while original JSONL stays unchanged.

For Combat, authority records two `FinalAuthorityState` rows at a stable server-time checkpoint. Both clients record both replicated Pawns as `ClientFinalState` two seconds before their controlled exit. The report's `finalConsistency` contains four comparisons and rejects position delta over 5 uu, Health/Energy delta over 0.15, or any Score/Deaths/Shield/Dead mismatch.

## Network profiles

UE 5.8 source confirms command-line packet simulation settings `-PktLag`, `-PktLagVariance` and `-PktLoss`. The runner applies them to all three processes and requires every log to echo each non-zero value.

| Profile | Lag | Variance | Loss |
|---|---:|---:|---:|
| Baseline | 0 ms | 0 ms | 0% |
| Lag60 | 60 ms | 0 ms | 0% |
| Lag120 | 120 ms | 0 ms | 0% |
| Jitter | 90 ms | 30 ms | 0% |
| Loss | 80 ms | 15 ms | 2% |

All profiles run the same Combat assertions: two connections, roles, Dash prediction/confirmation, Shield prediction/confirmation, four server projectiles, 34→17 shielded damage, later full damage, Death/Score/Respawn, clean client exits and no standalone fallback. Ability spacing is deliberately wider than the 0.45 s cooldown so network timing does not turn the test into a local-cooldown race.

Configured latency/loss and wall-clock duration are observations. The report sets `deterministicTimingClaim=false`; it does not claim deterministic real-time scheduling or benchmark performance.

## Failure scenarios

- `ClientDisconnect`: Client2 exits after the two-Pawn authority snapshot and before Client1/server; the remaining run closes cleanly.
- `ServerShutdown`: server exits first; both clients must emit `NetworkFailure`, exit through the C++ handler and avoid standalone fallback.
- `SecondClientConnectFail`: Client2 targets a separately reserved unused port; it emits `NetworkFailure`, while Client1 remains connected to the real server.
- `DashRejected`: predicted client action, actual server Energy 0, structured resource rejection and correction.
- `AuthorityAbuse`: forbidden state write, forged damage, invalid/out-of-range target and duplicate sequence, plus predicted real GAS Attack rejected by authority target geometry with no projectile.
- `AttackFlood`: four native GAS ServerTryActivateAbility requests/Prediction Keys; one AttackConfirmed/projectile/damage and three server `Cooldown.Attack` rejections, alongside one accepted Core probe and rate-limited follow-ups.
- `DeadAbility`: Health 0 + `State.Dead`, normal GAS attack rejected, no projectile.
- `DuplicateRespawn`: two reliable requests while one weak timer is pending; second rejected, exactly one replacement.
- `Watchdog`: no process has an exit timer. The child runner must fail non-zero at the deadline; `Test-Watchdog.ps1` then proves all three logs remain and no process carrying the RunId exists.

## Report verification

`Verify-ScenarioReport.ps1` checks schema/result, RunId/source SHA, optional clean-worktree requirement, exactly three exit-0 process records and all three non-empty JSONL summaries. Expected-fault reports use `PASS_EXPECTED_FAULT`; watchdog is separately `PASS_EXPECTED_FAILURE` because the runner itself must return non-zero. Its report includes source SHA, dirty flag, verification UTC, three role/PID identities and leak count.

Full logs and traces remain under ignored `Artifacts/`. Only small, path/PID-redacted evidence examples are committed.
