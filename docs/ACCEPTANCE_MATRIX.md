# Acceptance Matrix

Statuses are `NOT RUN`, `RED`, `PASS`, `FAIL`, or `N/A` with a reason. `PASS` requires a current command, source SHA, and authoritative evidence path.

| Area | Requirement | Status | Evidence |
|---|---|---:|---|
| PRE-01 | GitHub identity is exactly `Iviesever` | PASS | 2026-09-04 read-only `gh auth status` and `gh api user` audit |
| PRE-02 | Target repository state recovered without overwrite | PASS | API 404 plus owner repository-list cross-check; repository did not exist |
| PRE-03 | Local target conflict/status recovered | PASS | Exact paths absent before creation; `D:\program` not a Git repository |
| PRE-04 | UE 5.8, VS/MSVC, Windows SDK, pwsh, MQB, Git discovered | PASS | UE 5.8.0 CL 55116800; MSVC 14.51; SDK 26100; pwsh 7.6; MQB 5.4; Git 2.51 |
| PRE-05 | Related process inventory captured without interference | PASS | No matching UE/UBT/UAT/Shader/Cook/Package/test process at audit time |
| PACT-00.01 | Public GitHub repository | PASS | `gh repo view`: `Iviesever/authority-arena-ue5`, `PUBLIC`, initially empty |
| PACT-00.02 | Minimal UE 5.8 C++ project opens in Editor | PASS | Headless and visible smoke loaded `/Engine/Maps/Entry` with `AuthorityArenaGameMode` |
| PACT-00.03 | Development Editor build | PASS | UBT `AuthorityArenaEditor Win64 Development`, final exit 0 |
| PACT-00.04 | Development Game build | PASS | UBT `AuthorityArena Win64 Development`, current-source final exit 0 |
| PACT-00.05 | Shipping Game build | PASS | UBT `AuthorityArena Win64 Shipping`, current-source final exit 0 |
| PACT-00.06 | Programmatic graybox requires no manual Editor clicks | PASS | Both smoke logs contain `AA_EVENT ArenaReady blocks=6`; C++ default subcomponents use Engine BasicShapes |
| PACT-00.07 | Build/test scripts and governance contracts | PASS | RED exit 1 when verifier absent; GREEN `PASS contracts ... files=12` on 2026-09-04 |
| PACT-00.08 | Real `main` baseline pushed | PASS | Source commit `249ed563e6c84bbca24d191d6c049eb53ab94163`; local and `origin/main` matched |
| PACT-00.09 | Feature branch from real `origin/main` | PASS | `feat/authority-arena-0.1` created from `origin/main` at `e35df954166ff558b824ce41386c26a68f684d24` and pushed |
| PACT-00.10 | Draft PR and factual progress | PASS | Draft PR #1, base `main`, head `feat/authority-arena-0.1`, verified `isDraft=true`, `CLEAN` |
| PACT-10.01 | One server and two independent clients | PASS | Clean SHA `15e800b`; both reports show 3 distinct exit-0 processes and 2 connections |
| PACT-10.02 | CharacterMovement replicated across real processes | PASS | Connection report authority X values moved >100 uu from configured spawns |
| PACT-10.03 | Authority/Autonomous/Simulated roles observed | PASS | Server/client logs plus compact connection report |
| PACT-10.04 | PlayerState identity/score/deaths ownership | PASS | UE reflection Automation plus real `Client1`/`Client2` PlayerState identity logs |
| PACT-10.05 | GameState match phase/time/round state | PASS | RepNotify reflection covers phase/time/round/run ID; server/client match events observed |
| PACT-10.06 | Replicated properties and RepNotify | PASS | `AuthorityArena.Network.ReplicationContract`, succeeded 1, failed 0 |
| PACT-10.07 | Reliable Server RPC, bounded unreliable RPC, Client RPC, presentation Multicast | PASS | UHT flag reflection + Lifecycle `MatchPulse` on all three processes |
| PACT-10.08 | Destroy/respawn/disconnect leaves no dangling reference | PASS | Lifecycle report: one destroy/one respawn/count 2/two disconnects/zero process leaks |
| PACT-20.01 | Predicted GAS Dash with cost/cooldown/confirmation/rejection | PASS | Clean Combat + DashRejected reports at SHA `7676892`; client prediction/server confirm and resource rejection/correction |
| PACT-20.02 | Server-spawned projectile and GameplayEffect damage | PASS | Combat: four server deferred spawns/impacts; 34-damage GameplayEffect; no client damage parameter |
| PACT-20.03 | GAS Tag/Effect Shield with energy and damage reduction | PASS | Native duration tag/effect; clean Combat first hit raw 34/applied 17, later 34 |
| PACT-20.04 | ASC/AttributeSet/Ability/Effect/Tags/Cue boundary/Prediction Key/Spec lifecycle | PASS | GAS Automation 1/1 + clean multi-process evidence; ASC on PlayerState Mixed, native effects/tags/specs, impact multicast boundary |
| PACT-30.01 | Client cannot set Health or Score | PASS | Clean AuthorityAbuse: `ForbiddenStateWrite`; post-probe Health/Energy 100, Score/Deaths 0 |
| PACT-30.02 | Client cannot forge damage or exceed attack rate | PASS | Clean AuthorityAbuse `ForgedDamage`; AttackFlood 1 accepted + 3 `RateLimited` |
| PACT-30.03 | Energy/dead/target/respawn/RPC validation fails closed | PASS | Clean DashRejected, DeadAbility, AuthorityAbuse and DuplicateRespawn reports at `148a1b1` |
| PACT-30.04 | Rejection is structured, non-mutating, non-crashing and convergent | PASS | Stable reason events, unchanged snapshots, predicted correction, all owned processes exit 0/leak 0 |
| PACT-40.01 | PowerShell discovers UE, unique port, ready/connect/action/final workflow | PASS | Clean matrices at `aef2a5e`; unique RunId/port, real listener/ready markers and bounded exits |
| PACT-40.02 | JSON from server and both clients verifies full combat lifecycle | PASS | Three validated JSONL streams per profile plus `report.json`; event counts in `network-matrix.json` |
| PACT-40.03 | Cleanup owns exact processes and preserves unrelated processes | PASS | PID/start/path identity; watchdog leak 0; unrelated `cookscope-ue5` process observed and untouched |
| PACT-40.04 | Baseline, 60 ms, 120 ms, jitter, loss | PASS | Five clean full-Combat reports, strict verifier, `docs/examples/network-matrix.json` |
| PACT-40.05 | Client disconnect, server shutdown, client connect failure | PASS | Clean ClientDisconnect and two `PASS_EXPECTED_FAULT` reports with C++ NetworkFailure exits |
| PACT-40.06 | Rejection, attack flood, invalid target, timeout/watchdog | PASS | Clean failure matrix 9/9; watchdog runner exit 1, logs kept, owned leak 0 |
| PACT-40.07 | Functional assertions separated from timing observations | PASS | Functional booleans separate from `observations`; every report sets `deterministicTimingClaim=false` |
| PACT-50.01 | Distinct players and visible Health/Energy/Cooldown/roles/network data | NOT RUN | — |
| PACT-50.02 | Visible Dash states, projectile, shield, death/respawn/score/events | NOT RUN | — |
| PACT-50.03 | Real two-client screenshot from current SHA | NOT RUN | — |
| PACT-50.04 | Architecture, replication and GAS diagrams | NOT RUN | — |
| PACT-50.05 | Structured network report; optional trace remains ignored | NOT RUN | — |
| PACT-60.01 | Core, Attribute/Effect, Ability, RepNotify, validation, lifecycle, config tests | NOT RUN | — |
| PACT-60.02 | Negative, repeated, timeout and process-cleanup tests | NOT RUN | — |
| PACT-60.03 | UE Automation, headless and interactive verification | NOT RUN | — |
| PACT-60.04 | BuildCookRun/Cook/Stage/Pak/conditional IoStore/Archive | NOT RUN | — |
| PACT-60.05 | Packaged executable and packaged two-client scenario | NOT RUN | — |
| PACT-60.06 | Clean-source full rebuild and verification | NOT RUN | — |
| PACT-60.07 | Artifact SHA-256 and source SHA binding | NOT RUN | — |
| PACT-70.01 | README and optionality-resolved README_ZH | NOT RUN | — |
| PACT-70.02 | All required architecture/network/GAS/authority/test/build docs | NOT RUN | — |
| PACT-70.03 | AI disclosure, walkthrough, interview guide and three live drills | NOT RUN | — |
| PACT-70.04 | Rollback versus UE replication comparison | NOT RUN | — |
| PACT-70.05 | Independent audit has no Blocker/High | NOT RUN | — |
| PACT-70.06 | PR ready and merged with normal merge commit | NOT RUN | — |
| PACT-70.07 | Annotated `v0.1.0` tag | NOT RUN | — |
| PACT-70.08 | Non-draft source-only Release with empty custom assets | NOT RUN | — |

## Current release decision

`NOT ELIGIBLE`: P0 implementation and verification have not run. No tag or Release may be created.
