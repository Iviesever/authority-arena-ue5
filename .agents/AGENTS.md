# Repository Workflow

## Mandatory sequence

Every PACT is executed in this order:

```text
RED -> minimal implementation -> build/test -> real run -> evidence -> progress -> commit -> push
```

- One Agent only may write this repository. Additional Agents are read-only auditors.
- Do not write production behavior before observing the corresponding test or acceptance check fail for the expected reason.
- Never mark `PASS` from intent, compilation alone, stale output, or a narrower proxy check.
- Keep `docs/ACCEPTANCE_MATRIX.md`, the active `progress.md`, and `handoff.md` truthful after every checkpoint.
- Do not overwrite unknown files, rewrite failure history, force-push, or terminate unrelated processes.
- A process may be terminated only when its PID, start time, executable path, and current run ID identify it as owned by the current scenario.
- Core authority logic is C++; Blueprints may not contain authoritative gameplay behavior.
- MQB owns every target it can correctly and repeatably build. UBT/RunUAT fallbacks require exact failure evidence and documented boundaries.
- `Artifacts/`, packaged programs, executables, libraries, PDBs, Cook/Stage/Pak data, logs, and traces remain local and ignored.
- GitHub Release must contain no custom assets. Only GitHub-generated source archives are allowed.
- Do not publish `v0.1.0` unless every P0 row has current authoritative evidence, the clean-source/package checks pass, and an independent read-only audit has no Blocker or High finding.
- If those gates are not met, preserve a Draft PR and accurate Alpha/WIP handoff.

## Evidence rules

- Record the exact command, exit code, UTC/local timestamp, source SHA, and evidence path.
- Keep failures; append corrections rather than rewriting history.
- Screenshots and compact committed reports must come from the current source SHA.
- Network timing, scheduling, latency, packet loss, and correction counts are observations, not deterministic performance claims.
- Never expose credentials, GitHub tokens, unrelated command lines, or machine-sensitive paths in committed examples.

