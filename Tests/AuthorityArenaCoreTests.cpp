#include "AuthorityRules.h"
#include "NetworkScenario.h"
#include "ReportModel.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace
{
using AuthorityArena::Core::AbilityRequest;
using AuthorityArena::Core::AttackRequest;
using AuthorityArena::Core::AuthorityProbeRequest;
using AuthorityArena::Core::CompareSnapshots;
using AuthorityArena::Core::DecisionCode;
using AuthorityArena::Core::FinalSnapshot;
using AuthorityArena::Core::NetworkScenarioKind;
using AuthorityArena::Core::ParseScenarioName;
using AuthorityArena::Core::PlayerSnapshot;
using AuthorityArena::Core::RespawnRequest;
using AuthorityArena::Core::ValidateAbilityRequest;
using AuthorityArena::Core::ValidateAttackRequest;
using AuthorityArena::Core::ValidateAuthorityProbe;
using AuthorityArena::Core::ValidateRespawnRequest;

int FailureCount = 0;
int AssertionCount = 0;

void Expect(const bool condition, const std::string& message)
{
    ++AssertionCount;
    if (!condition)
    {
        ++FailureCount;
        std::cerr << "FAIL: " << message << '\n';
    }
}

template <typename Actual, typename Expected>
void ExpectEqual(const Actual& actual, const Expected& expected, const std::string& message)
{
    Expect(actual == expected, message);
}

AbilityRequest ValidAbilityRequest()
{
    return AbilityRequest{
        .has_authority = true,
        .is_owner = true,
        .is_alive = true,
        .is_stunned = false,
        .now_seconds = 10.0,
        .cooldown_ready_seconds = 9.0,
        .energy = 100.0,
        .energy_cost = 25.0,
    };
}

AttackRequest ValidAttackRequest()
{
    return AttackRequest{
        .ability = ValidAbilityRequest(),
        .target_valid = true,
        .target_alive = true,
        .last_attack_seconds = 8.0,
        .minimum_interval_seconds = 0.5,
        .squared_distance = 400.0,
        .maximum_squared_distance = 900.0,
    };
}

AuthorityProbeRequest ValidAuthorityProbe()
{
    return AuthorityProbeRequest{
        .has_authority = true,
        .is_owner = true,
        .is_alive = true,
        .target_valid = true,
        .target_alive = true,
        .now_seconds = 10.0,
        .last_request_seconds = 9.0,
        .minimum_interval_seconds = 0.45,
        .squared_distance = 1'440'000.0,
        .maximum_squared_distance = 2'250'000.0,
        .claimed_damage = 34.0,
        .server_damage = 34.0,
        .claimed_health = 100.0,
        .server_health = 100.0,
        .claimed_score = 0,
        .server_score = 0,
        .sequence = 2,
        .last_sequence = 1,
    };
}

void TestAbilityValidation()
{
    ExpectEqual(ValidateAbilityRequest(ValidAbilityRequest()), DecisionCode::Allowed,
                "valid ability request is allowed");

    auto request = ValidAbilityRequest();
    request.has_authority = false;
    ExpectEqual(ValidateAbilityRequest(request), DecisionCode::NotAuthority,
                "non-authority execution is rejected");

    request = ValidAbilityRequest();
    request.is_owner = false;
    ExpectEqual(ValidateAbilityRequest(request), DecisionCode::NotOwner,
                "non-owner request is rejected");

    request = ValidAbilityRequest();
    request.is_alive = false;
    ExpectEqual(ValidateAbilityRequest(request), DecisionCode::Dead,
                "dead actor cannot activate an ability");

    request = ValidAbilityRequest();
    request.is_stunned = true;
    ExpectEqual(ValidateAbilityRequest(request), DecisionCode::Stunned,
                "stunned actor cannot activate an ability");

    request = ValidAbilityRequest();
    request.energy = 24.999;
    ExpectEqual(ValidateAbilityRequest(request), DecisionCode::InsufficientEnergy,
                "energy below the exact cost is rejected");

    request = ValidAbilityRequest();
    request.cooldown_ready_seconds = 10.001;
    ExpectEqual(ValidateAbilityRequest(request), DecisionCode::OnCooldown,
                "request before cooldown readiness is rejected");

    request = ValidAbilityRequest();
    request.energy = request.energy_cost;
    request.cooldown_ready_seconds = request.now_seconds;
    ExpectEqual(ValidateAbilityRequest(request), DecisionCode::Allowed,
                "exact energy and cooldown boundaries are allowed");

    request = ValidAbilityRequest();
    request.energy = std::numeric_limits<double>::quiet_NaN();
    ExpectEqual(ValidateAbilityRequest(request), DecisionCode::InvalidNumeric,
                "NaN input fails closed");
}

void TestAttackValidation()
{
    ExpectEqual(ValidateAttackRequest(ValidAttackRequest()), DecisionCode::Allowed,
                "valid attack is allowed");

    auto request = ValidAttackRequest();
    request.last_attack_seconds = 9.75;
    ExpectEqual(ValidateAttackRequest(request), DecisionCode::RateLimited,
                "attack inside minimum interval is rejected");

    request = ValidAttackRequest();
    request.last_attack_seconds = 9.5;
    ExpectEqual(ValidateAttackRequest(request), DecisionCode::Allowed,
                "attack exactly at rate boundary is allowed");

    request = ValidAttackRequest();
    request.target_valid = false;
    ExpectEqual(ValidateAttackRequest(request), DecisionCode::InvalidTarget,
                "invalid target is rejected");

    request = ValidAttackRequest();
    request.target_alive = false;
    ExpectEqual(ValidateAttackRequest(request), DecisionCode::InvalidTarget,
                "dead target is rejected");

    request = ValidAttackRequest();
    request.squared_distance = 900.001;
    ExpectEqual(ValidateAttackRequest(request), DecisionCode::TargetOutOfRange,
                "out-of-range target is rejected");

    request = ValidAttackRequest();
    request.squared_distance = request.maximum_squared_distance;
    ExpectEqual(ValidateAttackRequest(request), DecisionCode::Allowed,
                "target exactly at range boundary is allowed");
}

void TestRespawnValidation()
{
    ExpectEqual(
        ValidateRespawnRequest(RespawnRequest{true, true, true, false}),
        DecisionCode::Allowed,
        "dead owned player without a pending respawn may request respawn");
    ExpectEqual(
        ValidateRespawnRequest(RespawnRequest{true, true, true, true}),
        DecisionCode::RespawnPending,
        "duplicate respawn request is rejected");
    ExpectEqual(
        ValidateRespawnRequest(RespawnRequest{true, true, false, false}),
        DecisionCode::NotDead,
        "living player cannot request respawn");
}

void TestAuthorityProbeValidation()
{
    ExpectEqual(ValidateAuthorityProbe(ValidAuthorityProbe()), DecisionCode::Allowed,
                "valid authority probe is allowed without trusting client state");

    auto request = ValidAuthorityProbe();
    request.claimed_health = 999.0;
    ExpectEqual(ValidateAuthorityProbe(request), DecisionCode::ForbiddenStateWrite,
                "client cannot replace authoritative health");

    request = ValidAuthorityProbe();
    request.claimed_score = 99;
    ExpectEqual(ValidateAuthorityProbe(request), DecisionCode::ForbiddenStateWrite,
                "client cannot replace authoritative score");

    request = ValidAuthorityProbe();
    request.claimed_damage = 999.0;
    ExpectEqual(ValidateAuthorityProbe(request), DecisionCode::ForgedDamage,
                "client cannot choose final damage");

    request = ValidAuthorityProbe();
    request.sequence = request.last_sequence;
    ExpectEqual(ValidateAuthorityProbe(request), DecisionCode::DuplicateSequence,
                "duplicate request sequence is rejected");

    request = ValidAuthorityProbe();
    request.target_valid = false;
    ExpectEqual(ValidateAuthorityProbe(request), DecisionCode::InvalidTarget,
                "null or wrong target is rejected");

    request = ValidAuthorityProbe();
    request.target_alive = false;
    ExpectEqual(ValidateAuthorityProbe(request), DecisionCode::InvalidTarget,
                "dead target is rejected");

    request = ValidAuthorityProbe();
    request.squared_distance = request.maximum_squared_distance + 1.0;
    ExpectEqual(ValidateAuthorityProbe(request), DecisionCode::TargetOutOfRange,
                "unreachable target is rejected");

    request = ValidAuthorityProbe();
    request.last_request_seconds = 9.8;
    ExpectEqual(ValidateAuthorityProbe(request), DecisionCode::RateLimited,
                "too-fast attack request is rejected");
}

void TestNetworkScenarios()
{
    const auto baseline = ParseScenarioName("baseline");
    Expect(baseline.has_value(), "baseline scenario parses");
    if (baseline)
    {
        ExpectEqual(baseline->kind, NetworkScenarioKind::Baseline, "baseline kind");
        ExpectEqual(baseline->lag_ms, 0, "baseline lag");
        ExpectEqual(baseline->loss_percent, 0, "baseline loss");
    }

    const auto lag60 = ParseScenarioName("lag60");
    Expect(lag60.has_value() && lag60->lag_ms == 60, "lag60 parses exactly");

    const auto lag120 = ParseScenarioName("lag120");
    Expect(lag120.has_value() && lag120->lag_ms == 120, "lag120 parses exactly");

    const auto jitter = ParseScenarioName("jitter");
    Expect(jitter.has_value() && jitter->lag_variance_ms > 0, "jitter has variance");

    const auto loss = ParseScenarioName("loss");
    Expect(loss.has_value() && loss->loss_percent > 0, "loss has packet loss");

    Expect(!ParseScenarioName("unknown").has_value(), "unknown scenario fails closed");
}

FinalSnapshot ServerSnapshot()
{
    return FinalSnapshot{{
        PlayerSnapshot{"Client1", 100.0, 75.0, 1, 0, true},
        PlayerSnapshot{"Client2", 100.0, 100.0, 0, 1, true},
    }};
}

void TestSnapshotConsistency()
{
    const auto server = ServerSnapshot();
    auto reorderedClient = FinalSnapshot{{server.players[1], server.players[0]}};
    auto result = CompareSnapshots(server, reorderedClient);
    Expect(result.consistent, "player order does not affect consistency");
    Expect(result.reason.empty(), "consistent snapshots have no failure reason");

    auto wrongHealth = server;
    wrongHealth.players[0].health = 99.0;
    result = CompareSnapshots(server, wrongHealth);
    Expect(!result.consistent && result.reason.find("health") != std::string::npos,
           "health mismatch is detected and explained");

    auto missingPlayer = server;
    missingPlayer.players.pop_back();
    result = CompareSnapshots(server, missingPlayer);
    Expect(!result.consistent && result.reason.find("count") != std::string::npos,
           "missing player is detected and explained");
}
} // namespace

int main()
{
    TestAbilityValidation();
    TestAttackValidation();
    TestRespawnValidation();
    TestAuthorityProbeValidation();
    TestNetworkScenarios();
    TestSnapshotConsistency();

    if (FailureCount != 0)
    {
        std::cerr << "AuthorityArenaCoreTests: " << FailureCount << " failure(s)\n";
        return 1;
    }

    std::cout << "AuthorityArenaCoreTests: PASS (" << AssertionCount << " assertions)\n";
    return 0;
}
