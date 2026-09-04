## AuthorityArena 0.1 vertical slice

This Draft PR tracks PACT-10 through PACT-70 after the verified PACT-00 baseline on `main`.

Current state at PR creation:

- UE 5.8 C++ project and programmatic graybox load in headless and visible processes.
- Development Editor, Development Game, and Shipping Game targets build.
- MQB owns the shared standard C++ rules tests; UBT/RunUAT boundaries are documented.
- The Epic-installed engine cannot build a Server target, so the contract-approved independent `-server -nullrhi` process is used without claiming a Dedicated Server binary.

Still required before Ready/merge:

- PACT-10 native replication with one server and two clients.
- PACT-20 native GAS Dash/Projectile/Shield.
- PACT-30 authority and rejection matrix.
- PACT-40 real multi-process network/failure automation.
- PACT-50 visual/diagnostic evidence.
- PACT-60 Automation, packaging, packaged multiplayer, and clean-source rebuild.
- PACT-70 complete portfolio docs and independent no-Blocker/High audit.

No binary or packaged artifact will be attached to the Release. If any P0 gate remains unproven, this PR stays Draft and the project remains Alpha/WIP.
