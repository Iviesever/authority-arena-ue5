# Rollback Versus UE Replication

AuthorityArena uses UE client/server replication, CharacterMovement prediction, and GAS Prediction Keys. It does not implement a custom rollback simulation or server rewind.

| Dimension | UE replication/prediction in AuthorityArena | Rollback simulation |
|---|---|---|
| Goal | Keep clients responsive while converging durable state to server truth | Hide latency by rewinding to a past state and resimulating an input history |
| Ownership | Server owns gameplay truth; owning client predicts eligible intent | Often one authority or deterministic peers own an input timeline and confirmed frame |
| Prediction | CharacterMovement saved moves and GAS LocalPredicted abilities | Entire deterministic game-state frames are predicted |
| Correction | Server property/movement updates and Prediction Key confirm/reject | Restore a prior snapshot, apply corrected input, then resimulate forward |
| Bandwidth | Replicated property deltas, actor channels, RPCs, movement packets | Primarily input frames plus checksums/snapshots; exact design varies |
| Determinism | General UE gameplay is not assumed bitwise deterministic | Simulation must be highly deterministic across resimulation participants |
| Collision/history | Current server collision for projectile overlap | Historical collision/state can be restored for rewind validation |
| Failure surface | Ownership, relevancy, stale replication, RPC validation, correction | Divergent simulation, insufficient history, expensive resimulation, visual pops |
| Use case | Authoritative action games with rich UE Actors/GAS and scalable replication | Tight deterministic games or explicit rewind/lag-compensation systems |

## Why this project stays with UE replication

The learning target is UE networking: roles, ownership, RPC conditions, RepNotify, CharacterMovement, GAS prediction/rejection, server-owned damage, and process orchestration. Building a second rollback engine would obscure those boundaries and introduce a determinism claim the project cannot substantiate.

The Dash rejection test already demonstrates a correction loop without custom rollback: the client predicts root motion/cost, the server rejects from actual Energy, the Prediction Key resolves, and replicated movement/attributes converge. Projectile collision is deliberately current-authority collision; no historical hitbox claim is made.

## What a future rewind experiment would require

A scoped P1 could record timestamped server transforms, validate client shot time within a bounded window, rewind only collision proxies, run the query, restore state, and compare false-positive/latency behavior. It would need adversarial timestamps, memory/bandwidth measurements, restore-safety tests, and explicit separation from full deterministic rollback.
