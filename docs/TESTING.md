# Testing

AuthorityArena separates deterministic rule tests, UE reflection tests, real-process functional tests, failure injection, and local delivery validation. A fast test does not substitute for a real-process check, and a screenshot does not substitute for an assertion.

## Test layers

| Layer | Entrypoint | What it proves |
|---|---|---|
| Contracts | `Verify-Contracts.ps1`, `Test-ProjectStructure.ps1`, `Verify-Documentation.ps1` | Required files, engine association, module/plugin dependencies, source-only policy, and documentation disclosures |
| Portable Core | `Test-Core.ps1` | MQB/MSVC compiles and runs 41 assertions for validation, rate limits, network profiles, and report rules |
| UE Automation | `Run-Automation.ps1` | Three tests: replication/reflection, GAS/effects, and HUD/view helpers |
| Smoke | `Run-Smoke.ps1` | Headless and D3D11 interactive Editor startup, C++ GameMode, and generated arena |
| Multi-process | `RunMultiplayerScenario.ps1` | One server process plus two clients, proxy roles, movement/GAS/combat/lifecycle, JSONL identity, and exact cleanup |
| Network matrix | `Invoke-NetworkMatrix.ps1` | Baseline, 60 ms, 120 ms, jitter, and 2% loss retain the same Combat outcome |
| Failure matrix | `Invoke-FailureMatrix.ps1`, `Test-Watchdog.ps1` | Disconnects, shutdown, connection failure, ability rejection, abuse, flood, dead ability, duplicate respawn, timeout, and cleanup |
| Delivery | `Package-Win64.ps1`, `Verify-PackagedBuild.ps1`, `Run-PackagedSmoke.ps1` | Build/Cook/Stage/Pak/IoStore/Archive, payload hashes, and real packaged startup |
| Clean source | `Verify-CleanSource.ps1` | A fresh clone of an exact commit repeats the build/test/package baseline without copied generated files |

## Core and UE commands

```powershell
pwsh -NoProfile -File .\scripts\Verify-Contracts.ps1
pwsh -NoProfile -File .\scripts\Test-ProjectStructure.ps1
pwsh -NoProfile -File .\scripts\Verify-Documentation.ps1
pwsh -NoProfile -File .\scripts\Test-Core.ps1 -Timings
pwsh -NoProfile -File .\scripts\Build.ps1 -Target Editor -Configuration Development
pwsh -NoProfile -File .\scripts\Build.ps1 -Target Game -Configuration Development
pwsh -NoProfile -File .\scripts\Build.ps1 -Target Game -Configuration Shipping
pwsh -NoProfile -File .\scripts\Run-Automation.ps1
```

The Server target probe is intentionally separate and expected to fail on the tested Epic-installed engine with `Server targets are not currently supported from this engine distribution.` It is evidence of a toolchain boundary, not a failing product test.

## Real-process commands

```powershell
pwsh -NoProfile -File .\scripts\RunMultiplayerScenario.ps1 `
  -Scenario Combat -NetworkProfile Baseline -TimeoutSeconds 90
pwsh -NoProfile -File .\scripts\Invoke-NetworkMatrix.ps1
pwsh -NoProfile -File .\scripts\Invoke-FailureMatrix.ps1
pwsh -NoProfile -File .\scripts\tests\RunMultiplayerScenario.Tests.ps1
pwsh -NoProfile -File .\scripts\Test-Watchdog.ps1
```

Every normal report requires three distinct exit-zero processes and three valid event streams. JSONL rows must carry schema 1, exact runId/processRole, and strictly increasing sequence. Functional fields decide pass/fail. Wall-clock duration, packet settings, ping, loss, and correction counts are observations, never deterministic benchmark claims.

Packaged runners use `gameExecutable`, not the root bootstrap. A historical test that owned only the bootstrap left its child Shipping game alive; the exact old PID/path was identified and removed, then direct-game smoke/Combat runs proved zero survivors. This regression is locked by `Test-ProjectStructure.ps1`. Watchdog evidence also binds source SHA, dirty state, UTC and all three role/PID identities.

## Packaged checks

Shipping is used for the release-like archive and real headless/interactive startup. UE Shipping Game builds intentionally clear command-line map/URL overrides unless the engine is rebuilt with `UE_ALLOW_MAP_OVERRIDE_IN_SHIPPING=1`; the project does not weaken that engine security default. The packaged two-client test therefore uses a Development archive. Its explicit automation-only flag removes the ListenServer host Pawn so only Client1 and Client2 participate in combat.

```powershell
pwsh -NoProfile -File .\scripts\Package-Win64.ps1 -Configuration Shipping
pwsh -NoProfile -File .\scripts\Package-Win64.ps1 -Configuration Development

# Assign the manifest paths printed by the two package commands.
$shippingManifest = 'D:\local\Artifacts\package\shipping\package-manifest.json'
$developmentManifest = 'D:\local\Artifacts\package\development\package-manifest.json'

pwsh -NoProfile -File .\scripts\Run-PackagedSmoke.ps1 `
  -ManifestPath $shippingManifest
pwsh -NoProfile -File .\scripts\Run-PackagedSmoke.ps1 `
  -ManifestPath $shippingManifest -Interactive
pwsh -NoProfile -File .\scripts\RunMultiplayerScenario.ps1 -Build Packaged `
  -PackageManifest $developmentManifest -Scenario Combat
```

The two manifest variables are caller-supplied local paths printed by `Package-Win64.ps1`; they are not release assets. The manifest records source SHA, engine version/changelist, configuration, every UAT stage, bootstrap/game EXE hashes, a sorted `.exe/.dll/.pak/.utoc/.ucas` inventory, payload bytes, and a reproducible package fingerprint.

## Evidence retention

Full logs and packages stay below ignored `Artifacts/`; reports committed under `docs/examples/` are path/PID-redacted summaries. A failed run is not rewritten as passing. The append-only progress log retains meaningful RED diagnostics and the later GREEN run.
