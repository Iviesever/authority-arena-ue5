# Server Authority and Fail-Closed Validation

## Trust boundary

The client may submit input intent, a referenced Actor, diagnostic view rotation, sequence numbers, and deliberately hostile values in automation probes. It never writes final Health, Energy, Damage, Score, Deaths, target validity, cooldown legality, or respawn state.

Production combat uses GAS activation plus a server-spawned physical projectile. The client has no damage parameter in that path. `ServerSubmitAuthorityProbe` is a bounded adversarial-test RPC on the owning PlayerController: it exercises the same trust categories with explicit hostile values, records a stable decision, and never applies gameplay mutations—even when the probe is legal.

## Stable decisions

The engine-independent `AuthorityArenaCore::ValidateAuthorityProbe` returns:

- `NotAuthority` / `NotOwner`
- `Dead`
- `InvalidNumeric`
- `DuplicateSequence`
- `ForbiddenStateWrite`
- `ForgedDamage`
- `InvalidTarget`
- `TargetOutOfRange`
- `RateLimited`
- `Allowed`

Validation reads server state first. The order is deliberate: authority/ownership/lifecycle and numeric sanity precede replay, state forgery, damage forgery, target checks, reachability and rate. A newer sequence is consumed even when its payload is rejected, so replaying the same hostile packet becomes `DuplicateSequence` rather than re-evaluating it indefinitely.

## Invariants

| Attempt | Authority response | State invariant |
|---|---|---|
| Claimed Health or Score differs | `ForbiddenStateWrite` | server values unchanged |
| Claimed damage differs from native 34 | `ForgedDamage` | no damage effect produced |
| Same/lower sequence | `DuplicateSequence` | no repeated work or mutation |
| Null/wrong/dead target | `InvalidTarget` | no projectile/damage |
| Target farther than validation range | `TargetOutOfRange` | no projectile/damage |
| Requests inside 0.45 s | `RateLimited` | first legal probe accepted; follow-ups rejected |
| Dash with server Energy 0 | GAS `Failure.Resource` | predicted cost/root motion corrected |
| Ability while `State.Dead` and Health 0 | GAS blocked-tag failure | no Projectile spawned |
| Second respawn while timer pending | `RespawnPending` | exactly one Pawn replacement |

Every rejection emits an `AA_EVENT` with sequence/action and reason. The owning client receives `ClientRequestRejected`; server state is captured afterwards. Process crashes, timeouts, missing events, standalone fallback, or non-zero exits fail the runner.

## Respawn safety

Respawn pending state belongs to PlayerController, not the disposable Character. GameMode keeps timer handles in a map keyed by `TWeakObjectPtr<AAuthorityArenaPlayerController>`. A second request is rejected while the first timer exists. Logout clears the timer and gate before releasing the controller. The timer delegate resolves the weak pointer before touching it, resets PlayerState ASC Health/Energy and `State.Dead`, spawns one Pawn, and records the post-respawn authority snapshot.

## Automation scenarios

- `AuthorityAbuse`: forged Health/Score, forged damage, null target, out-of-range target, and duplicate sequence; authority snapshot must remain Health/Energy 100, Score/Deaths 0.
- `AttackFlood`: after legitimate movement puts the peer in range, exactly one legal probe is accepted and rapid follow-ups are `RateLimited`; no gameplay state changes.
- `DeadAbility`: server establishes actual Health 0 plus `State.Dead`; normal client GAS Attack input is rejected and no Projectile exists.
- `DuplicateRespawn`: server destroys Client2 Pawn, client submits two reliable requests 100 ms apart, server rejects the second as pending and creates exactly one replacement.
- `DashRejected`: client predicts from Energy 100; server changes authoritative Energy to 0 during validation and rejects; final position/resource converge.

These checks cover game-state authority and request validation. They are not claims of production anti-cheat, encryption, secure transport, hostile-internet hardening, or identity authentication.
