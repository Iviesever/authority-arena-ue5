## AuthorityArena 0.1 vertical slice

This Draft PR tracks PACT-10 through PACT-70 after the verified PACT-00 baseline on `main`.

Implemented and source-reviewed in this Draft:

- UE 5.8 C++ project and programmatic graybox load in headless and visible processes.
- Development Editor, Development Game, and Shipping Game targets build.
- MQB owns the shared standard C++ rules tests; UBT/RunUAT boundaries are documented.
- The Epic-installed engine cannot build a Server target, so the contract-approved independent `-server -nullrhi` process is used without claiming a Dedicated Server binary.
- Native PlayerState/GameState replication, CharacterMovement, GAS Dash/Projectile/Shield, authority rejection, death/respawn/score, C++ HUD, and structured diagnostics.
- A five-profile network matrix, nine failure scenarios, watchdog cleanup, three-repeat baseline, real two-client screenshot, and technical diagrams.
- Shipping and Development BuildCookRun archives with Cook/Stage/Pak/IoStore/Archive, payload hashes, real packaged startup, and Development packaged two-client Combat.

Still required before Ready/merge:

- Commit and push the complete portfolio/AI/interview documentation.
- Run the exact committed source through clean-clone MQB, UBT, Automation, Editor Combat, Shipping package smoke, and Development packaged two-client Combat.
- Run a current-SHA full local matrix, obtain green GitHub source-contract CI, and complete a fresh independent read-only audit with no Blocker/High.
- Only then mark Ready, merge normally, create the annotated tag, and publish a source-only Release whose custom `assets` array is empty.

No binary or packaged artifact will be attached to the Release. If any P0 gate remains unproven, this PR stays Draft and the project remains Alpha/WIP.
