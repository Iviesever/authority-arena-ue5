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
- Audit-fix evidence: real GAS invalid-target and four-request Flood paths, four final server/client state comparisons in every Combat profile, and source-bound watchdog identities.

Final local delivery source: `eb03f031131d709b74a91a7f0c17216433e06b92`. Shipping package fingerprint: `1C383D2B835CA22E3E6E4C25D8078940123C74A26A895C0674447713C0DF0889` (679,627,218 total bytes). Development fingerprint: `5AC90E49C422D1A35102EC9305A38F2DFE31726FA7E243D1AA5FB9BE97E40B48` (1,037,353,813 total bytes). Both were produced from the exact detached clean-source commit; packaged Combat has matching report/package source SHA and final consistency 4/4.

Still required before Ready/merge:

- Final source-contract CI is green. The second independent read-only audit returned Blocker 0 / High 0; its one screenshot-wording Medium was corrected to the exact capture SHA.
- Mark Ready, merge normally, create the annotated tag, and publish a source-only Release whose custom `assets` array is empty.

No binary or packaged artifact will be attached to the Release. If any P0 gate remains unproven, this PR stays Draft and the project remains Alpha/WIP.
