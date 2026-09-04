# Live Change Drills

Each drill is intentionally small enough for an interview session. Create a short-lived branch, write the failing test first, make the minimum implementation, run the listed checks, and explain the authority/replication impact. Complete at least one before presenting this repository.

## Drill 1 — Energy regeneration

Goal: add server-authoritative Energy regeneration after a configurable delay without letting clients write Energy.

1. Add Core tests for finite/non-negative rate and clamping to MaxEnergy.
2. Add a native GameplayEffect or authority timer that changes Energy through ASC APIs.
3. Define how recent cost use resets the delay; keep the timestamp server-owned.
4. Add an Automation assertion for clamping and a real-process event that shows the owner receives the RepNotify update.

Verify:

```powershell
pwsh -NoProfile -File .\scripts\Test-Core.ps1
pwsh -NoProfile -File .\scripts\Build.ps1 -Target Editor -Configuration Development
pwsh -NoProfile -File .\scripts\Run-Automation.ps1
pwsh -NoProfile -File .\scripts\RunMultiplayerScenario.ps1 -Scenario Combat
```

Explain why regeneration belongs to authoritative GAS state, how it survives respawn, and why the HUD only reads the replicated value.

## Drill 2 — Dash rejection condition

Goal: add a server rejection condition such as “Dash is blocked for 0.75 seconds after taking damage.”

1. Write a Core decision test and a UE ability test that initially fail.
2. Represent the block with a server-granted gameplay tag rather than a client boolean.
3. Add the tag to Dash activation requirements and emit one stable rejection reason.
4. Add a real scenario where the client predicts before receiving the block, then converges after rejection.

Verify Core, `AuthorityArena.GAS`, the new scenario, and the existing `DashRejected` scenario. Explain Prediction Key rollback, authority timing, and why the condition is not trusted from the client.

## Drill 3 — RepNotify UI state

Goal: add a replicated `RoundOutcome` state displayed in the C++ HUD.

1. Add a reflection/contract test for a `ReplicatedUsing` property and its default.
2. Add an authority-only setter on GameState and an `OnRep_RoundOutcome` observation.
3. Draw the value in `AAuthorityArenaHUD` without any HUD-to-server mutation.
4. Add a two-process assertion that both clients observe the same value and a HUD helper test for formatting.

Verify:

```powershell
pwsh -NoProfile -File .\scripts\Run-Automation.ps1
pwsh -NoProfile -File .\scripts\RunMultiplayerScenario.ps1 `
  -Scenario ConnectionMovement -NetworkProfile Lag60
```

Explain why GameState is the owner, how RepNotify differs from multicast, and how a late-joining client obtains the durable value.
