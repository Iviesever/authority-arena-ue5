# Code Walkthrough

## Start at the boundaries

1. `AuthorityArena.uproject` enables GameplayAbilities and declares the Core and UE modules.
2. `AuthorityArenaCore` is standard C++: `AuthorityRules` validates untrusted requests, `NetworkScenario` defines emulation profiles, and `ReportModel` checks snapshot/report contracts. `Test-Core.ps1` gives this layer to MQB first.
3. `AuthorityArena` is the UE layer. Its Build.cs depends on Core, Engine, GameplayAbilities/Tags/Tasks, EnhancedInput, Json, Slate, and UMG.

## Follow one connection

`AAuthorityArenaGameMode::InitNewPlayer` parses the untrusted URL label and assigns a server-owned connection identity. `PostLogin` records the connection, establishes a replicated server-time scenario start, and emits a presentation pulse. `RestartPlayer` selects deterministic transforms. `Logout` clears weak respawn timers and emits the disconnect event.

`AAuthorityArenaPlayerState` owns identity, score, deaths, the ASC, and AttributeSet because those survive Pawn replacement. `AAuthorityArenaCharacter` owns the current avatar, camera, collision, CharacterMovement, combat/health components, and automation driver. `PossessedBy` and `OnRep_PlayerState` both bind `InitAbilityActorInfo(PlayerState, Character)`.

## Follow Dash

The local automation/input layer calls `TryActivateAbility(Ability.Dash)`. `UGA_Dash` is `LocalPredicted`, commits native cost/cooldown effects, and starts a root-motion ability task. Client and server share a Prediction Key. In the rejection scenario, only the server arms a one-shot zero-Energy gate; GAS rejects with `Failure.Resource` and native movement/attribute replication corrects the client.

## Follow an Attack

`UGA_ProjectileAttack` can display predicted input locally, but only `HasAuthority` calls `UAuthorityArenaCombatComponent::SpawnProjectileAuthority`. The replicated projectile carries server-chosen transform, direction, speed, lifetime, and 34 raw damage. Only server overlap applies `UAuthorityArenaGE_ProjectileDamage` to `IncomingDamage`. Attribute execution reads the Shield tag, computes 17 or 34 applied damage, clamps Health, and invokes the idempotent death path. Score/deaths live on PlayerState; GameMode destroys and replaces the victim Pawn after one weak, cancellable timer.

## Follow a hostile RPC

`AAuthorityArenaPlayerController::ServerSubmitAuthorityProbe` never trusts claimed Health, Score, damage, target, distance, or sequence. It reconstructs authority state, calls the portable `EvaluateAuthorityProbe`, emits an accepted/rejected event, and returns a stable reason to the owning client. Mutation is absent from this diagnostic probe. Real scenarios cover forbidden state writes, forged damage, duplicate sequence, invalid/dead/out-of-range targets, and rate limiting.

## Follow the evidence

All significant paths call `UAuthorityArenaNetworkDiagnosticsSubsystem::EmitEvent`. Each process writes its own JSONL with schemaVersion, runId, processRole, PID, sequence, UTC, event, context, and details. `RunMultiplayerScenario.ps1` owns the process identity, readiness, one deadline, assertion text, report, and reverse-order cleanup. It never uses image-name kills.

`AAuthorityArenaHUD::DrawHUD` reads replicated attributes/tags/score/deaths and local network observations. It never writes authoritative state. `AAuthorityArenaWorldBuilder` uses C++ default subobjects and Engine BasicShapes, lights, labels, and collision, so no manual map authoring or custom content asset is required.

## Delivery path

`Package-Win64.ps1` requires a clean worktree, runs Core tests, invokes RunUAT BuildCookRun, validates Cook/Stage/Pak/IoStore/Archive, inventories runtime payloads, and emits a source-bound manifest. `Verify-PackagedBuild.ps1` rejects a missing or changed file, size, hash, stage, source SHA, or overall fingerprint. Packages and complete logs remain ignored.
