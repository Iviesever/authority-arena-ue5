# AuthorityArena

> Work in progress on `main`: PACT-00 baseline only. The multiplayer/GAS vertical slice is developed on `feat/authority-arena-0.1`; no release exists yet.

AuthorityArena is an Unreal Engine 5.8 C++ server-authoritative multiplayer lab for a gameplay/networking portfolio. The final P0 scope is a real server plus two clients with native replication, GAS-predicted Dash, server-spawned Projectile Attack, replicated Shield, authoritative Health/Score/Death/Respawn, network emulation, automated negative paths, and packaged Win64 evidence.

Current verified baseline:

- MQB 5.4 builds and runs the shared standard C++ authority core: 32 assertions pass and incremental cache hits are recorded.
- UE 5.8 Development Editor, Development Game, and Shipping Game targets build successfully.
- Headless and visible game startup load the C++ GameMode and emit `AA_EVENT ArenaReady blocks=6` from the programmatic graybox.
- This installed UE distribution does not support a Server target; later multi-process checks use an independent `-server -nullrhi` Game/Editor process and disclose that limitation.

Build the current baseline:

```powershell
pwsh -NoProfile -File .\scripts\Test-Core.ps1
pwsh -NoProfile -File .\scripts\Build.ps1 -Target Editor -Configuration Development
pwsh -NoProfile -File .\scripts\Run-Smoke.ps1
pwsh -NoProfile -File .\scripts\Run-Smoke.ps1 -Interactive
```

See [the product contract](docs/PRODUCT_CONTRACT.md), [architecture](docs/ARCHITECTURE.md), [acceptance matrix](docs/ACCEPTANCE_MATRIX.md), and [build evidence boundary](docs/BUILD_SYSTEM.md). Statements will be upgraded only when the corresponding row has current evidence.
