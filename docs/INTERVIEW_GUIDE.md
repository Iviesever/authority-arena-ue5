# Interview Guide

Use this guide to explain decisions in your own words. Do not memorize slogans; trace each answer to code and a runnable scenario.

## Network roles

The server owns `Authority` actors. An owning client sees its Pawn as `AutonomousProxy`; other clients see it as `SimulatedProxy`. `RoleSnapshot` events prove these roles in different OS processes. A role describes replication behavior, not business permission by itself.

## Actor ownership

The PlayerController owns its client connection and is the correct target for client-to-server RPCs and owner-only rejection responses. PlayerState replicates to everyone and survives respawn. GameMode is server-only. Ownership is why arbitrary clients cannot call a Server RPC on another player's controller.

## RPC calling conditions

A Server RPC requires a replicated actor owned by the caller and a live connection. Reliable does not make an invalid ownership path valid. AuthorityArena validates every request again on the server before mutation. `ServerRequestRespawn` is reliable because it is a discrete lifecycle request; `ServerReportViewSample` is unreliable because newer samples supersede old ones.

## RepNotify

RepNotify observes server-written state changes on clients. GameState uses it for phase/run/time fields; PlayerState and AttributeSet use it for identity, score/deaths, Health, and Energy. Server setters call the same observation method locally so diagnostics are comparable, but RepNotify is not the authority decision itself.

## PlayerState versus Character

PlayerState owns identity, Score, Deaths, ASC, attributes, ability specs, cost/cooldown state, and tags that must survive Pawn replacement. Character is the current avatar: transform, collision, camera, Character Movement, visual state, and Pawn-scoped components. On respawn the ASC persists and binds to the new Character avatar without duplicate specs.

## GAS prediction

Dash and Shield use `LocalPredicted`. The client starts responsive presentation and commits a predicted spec using a Prediction Key; the server independently validates tags/resources/cooldown and confirms or rejects. Attack may start predicted presentation, but projectile creation and damage are authority-only. Prediction improves feel; it never transfers final truth.

## Server rejection

Rejection is fail-closed and happens before mutation. Examples include `Failure.Resource`, cooldown, `State.Dead`, forbidden state write, forged damage, invalid/out-of-range target, duplicate sequence, rate limit, and respawn pending. The DashRejected run is useful because it proves visible client prediction followed by authoritative correction.

## Character Movement

The project relies on `UCharacterMovementComponent` saved moves, client prediction, server correction, and replicated transform/rotation. It does not invent a second movement protocol. The custom movement subclass only counts corrections for observability.

## Reliable and Unreliable

Reliable RPCs are ordered and retransmitted but can still overflow or block if abused; reserve them for infrequent state transitions. Unreliable RPCs suit superseding observations and presentation. AuthorityArena uses reliable respawn/authority probes and client rejection notifications, unreliable view samples and match/impact presentation, and replicated properties for durable truth.

## Dormancy and Relevancy basics

Relevancy decides whether an actor should replicate to a connection; dormancy avoids repeated property work for stable actors. They affect bandwidth and wake-up correctness, not authority. This P0 keeps combat actors active and does not claim tuned dormancy/relevancy results. A future test would measure bytes, force wake-up on change, and verify late/re-entering clients receive current state.

## Rollback versus UE Replication

UE replication/prediction converges replicated state from a server truth. Rollback simulation rewinds deterministic state and resimulates inputs, often for peer fighting games or server reconciliation. AuthorityArena uses UE CharacterMovement/GAS prediction and no custom rollback or rewind. See `ROLLBACK_VS_UE_REPLICATION.md`.

## Multi-process testing

The test is real because the server and two clients have distinct PIDs, separate worlds and event files, a shared runId, actual UDP connection roles, and independent exits. Editor-Cmd provides runtime DedicatedServer behavior. Packaged Development uses a ListenServer process plus two remote clients and explicitly suppresses the automation host Pawn.

## Process cleanup

The runner stores PID, start time, and absolute executable path. Before stopping a process it rechecks all identity data; mismatch fails closed. Cleanup runs in `finally`, in reverse creation order, and never kills by executable name. Timeout testing additionally searches for the exact runId and requires zero survivors.

## Common failure diagnosis

- No log: confirm UE's absolute switch is `-abslog`, not a generic `-log` assumption.
- Server never ready: inspect actual `IpNetDriver listening on port` and `ServerReady`, then verify port and map URL.
- Shipping clients become Standalone: UE Shipping clears command-line URL overrides by default; use Development for URL-driven packaged E2E.
- Ability activates but outcome is absent: separate predicted, confirmed, projectile-spawn, overlap, effect, and replicated-state events.
- Client correction is flaky: schedule from replicated server time, avoid local startup clocks, and keep functional assertions independent of wall time.
- Respawn duplicates: inspect the controller pending latch and weak timer; count destroy/respawn events exactly.
- Package Stage loses Zen: retain the UAT failure, allow `-StartZenServerForStage` recovery, and rerun in a new output directory.
- Visual run stalls on startup: read RHI/driver logs; on the tested machine explicit D3D11 avoids the UE 5.8 D3D12 driver deny-list.

## A concise project pitch

“AuthorityArena is a source-first UE 5.8 networking lab. The interesting part is not the graybox combat itself; it is the executable boundary between predicted client intent and server-owned outcomes, plus a runner that proves that boundary under real processes, packet emulation, failures, packaging, and clean-source rebuild.”
