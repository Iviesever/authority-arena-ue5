# Product Contract

AuthorityArena is an Unreal Engine 5.8 C++ server-authoritative multiplayer lab for a public engineering portfolio. It must demonstrate UE client/server replication, Actor ownership and roles, RepNotify and RPC boundaries, Gameplay Ability System prediction and rejection, real server plus two-client processes, network emulation, automated verification, and packaged Win64 evidence.

The complete binding contract is copied verbatim at [`tasks/20260904-224034-authority-arena-0.1/goal-objective.md`](../tasks/20260904-224034-authority-arena-0.1/goal-objective.md). This summary cannot relax it.

## Required P0 outcome

- A third-person programmatic graybox arena with two distinguishable players.
- Movement, orientation, Projectile Attack, predicted Dash, Shield/Block, Health, Energy, Cooldown, Death, Respawn, and Score.
- C++ authoritative state and real GAS abilities/effects/tags/spec lifecycle—not RPC facsimiles.
- Stable server rejection of forged, invalid, too-fast, unaffordable, dead-state, out-of-range, and duplicate-lifecycle requests.
- A bounded PowerShell 7 runner that owns exactly one server and two client processes, emits structured evidence, covers the required network/failure matrix, and cleans only its own PIDs.
- Development Editor, Development Game, Shipping Game, UE Automation, headless and interactive launch, BuildCookRun, Cook, Stage, Pak, conditional IoStore, Archive, packaged launch, packaged two-client run, and clean-source rebuild.
- Current-source screenshots, diagrams, compact reports, complete portfolio/interview documentation, and explicit AI assistance disclosure.
- Normal PR merge, annotated `v0.1.0` tag, and a GitHub Release with an empty custom-assets list.

## Authority boundary

Clients may predict presentation and eligible GAS actions. They never decide final Health, damage, Score, Death, Respawn, target validity, or cooldown/resource legality. Rejected requests change no authoritative state, do not crash a process, and must converge back to the server state.

## Build boundary

MQB 5.4.0 is tried first and owns repeatable supported targets. The shared standard C++ rules core is deliberately MQB-compatible. UHT/UBT and RunUAT remain the evidence-backed path for UE reflection, targets, Cook, Stage, Pak, IoStore, and Archive when MQB cannot express equivalent semantics.

## Release boundary

Local packaging is mandatory. Packaged binaries and heavy evidence stay in ignored `Artifacts/`. The GitHub Release must have no uploaded custom assets; only GitHub's generated source code archives are permitted.

## Honest fallback

If any P0 gate lacks current, reproducible evidence by the deadline, the only allowed outcome is an unmerged Draft Alpha/WIP with an accurate handoff. A plausible implementation, partial test run, or Debug-only demo is not `v0.1.0` completion.

