# AuthorityArena v0.1.0

AuthorityArena 0.1.0 is a source-first UE 5.8 C++ multiplayer portfolio lab. It packages a compact third-person graybox around server authority, native GAS prediction/rejection, real process orchestration, adverse-network validation, and auditable local delivery.

## Included

- One server process plus two independent client processes with Authority, AutonomousProxy, and SimulatedProxy evidence.
- PlayerState-owned Mixed ASC/AttributeSet with native Dash, Projectile Attack, and Shield abilities/effects/tags.
- Server-only projectile spawn/damage, Health, Death, Respawn, Score, resource/cooldown/target validation, and stable rejection reasons.
- Baseline, lag, jitter, loss, disconnect, shutdown, connect-failure, abuse, flood, dead-state, duplicate-respawn, and watchdog scenarios.
- C++ HUD, programmatic arena, real two-client screenshot, architecture/replication/GAS diagrams, and structured JSONL reports.
- MQB Core tests, UE Automation, Editor/Game Development and Game Shipping builds, BuildCookRun/Cook/Stage/Pak/IoStore/Archive, payload fingerprints, packaged startup, packaged two-client Development scenario, and clean-source verification.

## Delivery evidence

Every local package manifest records its exact source SHA, engine version/changelist, configuration, stage results, local ignored path, total/payload bytes, bootstrap and game executable SHA-256, individual payload hashes, and aggregate package fingerprint. The acceptance matrix and progress log identify the final verified manifests and process reports. Local paths are evidence for the build host and are not downloadable release assets.

Release-candidate local evidence is bound to source `eb03f031131d709b74a91a7f0c17216433e06b92`:

- Shipping: 679,627,218 total bytes; 442,065,855 hashed payload bytes; game EXE SHA-256 `3BCB342B3286DB9481B07713BC54F41D2C9FD6115BF111C8B43DCD57E694714F`; package fingerprint `1C383D2B835CA22E3E6E4C25D8078940123C74A26A895C0674447713C0DF0889`.
- Development: 1,037,353,813 total bytes; 652,238,039 hashed payload bytes; game EXE SHA-256 `D3DF311ED9BF6159F641EFCFE96E004D633F730F9EACE782C206D9659CDC8240`; package fingerprint `5AC90E49C422D1A35102EC9305A38F2DFE31726FA7E243D1AA5FB9BE97E40B48`.
- Clean-source report: `Artifacts/clean-checkout/eb03f031-20260905-044828/Artifacts/clean-source-report.json`; Editor Combat run `6b38083cf8814d689259840a0fef1dfa`; Development packaged Combat run `537df6c50af34aba88a12ecf5f1ce8c7`; interactive Shipping run `190f4e2f7ba44c02bb89be8f620b4793`; owned survivors 0.

## Limits

The Epic-installed engine cannot build a Server target. Shipping Game also blocks command-line URL overrides under the engine's default security macro, so Shipping is built and launched while packaged two-client URL-driven E2E uses Development. There is no matchmaking, production anti-cheat, rollback/rewind, Iris validation, or tuned dormancy/relevancy claim. See `KNOWN_LIMITATIONS.md`.

## Source-only release policy

This release has no custom release assets. No EXE, DLL, PDB, packaged ZIP, SDK/plugin ZIP, manual source archive, Cook/Stage/Pak data, log bundle, screenshot bundle, or trace is uploaded. GitHub's automatically generated source ZIP and tar.gz are the only downloads.

## AI disclosure

The user defined the career goal, topic, deadline, scope, constraints, and acceptance direction. Codex GPT-5.6 Sol performed architecture refinement, code, tests, debugging, packaging, audit, and documentation. The user did not independently hand-write this delivery code; it must not be represented as entirely hand-written. Review `AI_ASSISTANCE.md` and complete at least one Live Change Drill before an interview.
