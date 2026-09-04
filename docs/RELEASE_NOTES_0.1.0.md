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

Release-candidate local evidence is bound to source `568b57e5f99dc28d4f70f64a5fee3829cd08e4db`:

- Shipping: 679,588,078 total bytes; 442,058,687 hashed payload bytes; game EXE SHA-256 `43D463E0494DFD66A600532BFE40DF3D2805472D09B6BE08CB7C039D341B7C33`; package fingerprint `0EC9AB311007BB018E99115DF3DFA1438115B85773D3982E5DF845F431604BEF`.
- Development: 1,037,313,879 total bytes; 652,230,871 hashed payload bytes; game EXE SHA-256 `D2C4CCF947CACD342D190809E14EEB65893CB853CBC2669FEE399EA0EBDAE790`; package fingerprint `968E8C11FEE1DE183B8D133F00EBB8A688AE0F13F90D62514E4D9FBF093184E0`.
- Clean-source report: `Artifacts/clean-checkout/568b57e5-20260905-031936/Artifacts/clean-source-report.json`; Shipping smoke run `f5254a4164fd4b9192ace5882e2c0620`; Development packaged Combat run `5472e7fbbbf64bef9e454675b419f982`; owned survivors 0.

## Limits

The Epic-installed engine cannot build a Server target. Shipping Game also blocks command-line URL overrides under the engine's default security macro, so Shipping is built and launched while packaged two-client URL-driven E2E uses Development. There is no matchmaking, production anti-cheat, rollback/rewind, Iris validation, or tuned dormancy/relevancy claim. See `KNOWN_LIMITATIONS.md`.

## Source-only release policy

This release has no custom release assets. No EXE, DLL, PDB, packaged ZIP, SDK/plugin ZIP, manual source archive, Cook/Stage/Pak data, log bundle, screenshot bundle, or trace is uploaded. GitHub's automatically generated source ZIP and tar.gz are the only downloads.

## AI disclosure

The user defined the career goal, topic, deadline, scope, constraints, and acceptance direction. Codex GPT-5.6 Sol performed architecture refinement, code, tests, debugging, packaging, audit, and documentation. The user did not independently hand-write this delivery code; it must not be represented as entirely hand-written. Review `AI_ASSISTANCE.md` and complete at least one Live Change Drill before an interview.
