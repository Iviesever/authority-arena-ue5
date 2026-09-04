# Known Limitations

- The tested Epic-installed UE 5.8 distribution cannot build `TargetType.Server`. Editor evidence uses a distinct `UnrealEditor-Cmd -server -nullrhi` process with `NM_DedicatedServer`, but no Dedicated Server binary is claimed.
- UE Shipping Game clears command-line map and connection URL overrides when `UE_ALLOW_MAP_OVERRIDE_IN_SHIPPING=0`. Shipping archives are built and launched, while packaged server-plus-two-client automation uses Development. The repository does not override the engine-wide Shipping security policy.
- A Development Game process becomes a ListenServer and creates a host Pawn. The packaged E2E runner passes `-AuthoritySuppressHostPawn`; GameMode removes only that automation host Pawn so the two remote clients retain the same combat geometry. This is test scaffolding, not production session management.
- Full packet-emulation profiles run on the Editor-Cmd topology, where the log confirms applied `PktLag`, `PktLagVariance`, and `PktLoss`. Packaged validation is Baseline only and fails closed for other profiles.
- The project has no matchmaking, authentication, persistence backend, seamless travel, host migration, NAT traversal, encryption layer, production anti-cheat, or live service operations.
- The authority probes demonstrate validation boundaries and stable rejection reasons; they are not a claim of cheat-proof networking.
- Projectile hit validation is current server collision, not lag-compensated rewind. No custom rollback protocol is implemented.
- Iris, Network Prediction plugin integration, dormancy tuning, and relevancy/bandwidth experiments are intentionally outside P0.
- The committed screenshot is real viewport evidence bound to capture source `3611c5ff740706bc38680a2cb3c5b43bc94856d4`, not the later delivery HEAD and not a benchmark. Null-RHI scenario durations and UI ping/correction counters are observational.
- Network Insights trace generation is optional and no trace is committed. Large traces and all packages remain local and ignored.
- The local NVIDIA 551.61 driver is deny-listed by UE 5.8 for D3D12 startup. Interactive verification explicitly uses D3D11; this does not claim D3D12 compatibility on that machine.
- GitHub-hosted CI validates source/document contracts only because the hosted runner has neither the licensed UE 5.8 installation nor MQB 5.4. Full MQB, UBT, Automation, multi-process, and packaging gates are local and source-bound.
