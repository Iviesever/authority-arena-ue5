#pragma once

#include <cstdint>

#ifndef AUTHORITYARENACORE_API
#define AUTHORITYARENACORE_API
#endif

namespace AuthorityArena::Core
{
enum class DecisionCode : std::uint8_t
{
    Allowed,
    NotAuthority,
    NotOwner,
    Dead,
    NotDead,
    Stunned,
    InsufficientEnergy,
    OnCooldown,
    RateLimited,
    InvalidTarget,
    TargetOutOfRange,
    RespawnPending,
    InvalidNumeric,
    ForbiddenStateWrite,
    ForgedDamage,
    DuplicateSequence,
};

struct AbilityRequest
{
    bool has_authority = false;
    bool is_owner = false;
    bool is_alive = false;
    bool is_stunned = false;
    double now_seconds = 0.0;
    double cooldown_ready_seconds = 0.0;
    double energy = 0.0;
    double energy_cost = 0.0;
};

struct AttackRequest
{
    AbilityRequest ability;
    bool target_valid = false;
    bool target_alive = false;
    double last_attack_seconds = 0.0;
    double minimum_interval_seconds = 0.0;
    double squared_distance = 0.0;
    double maximum_squared_distance = 0.0;
};

struct RespawnRequest
{
    bool has_authority = false;
    bool is_owner = false;
    bool is_dead = false;
    bool respawn_pending = false;
};

struct AuthorityProbeRequest
{
    bool has_authority = false;
    bool is_owner = false;
    bool is_alive = false;
    bool target_valid = false;
    bool target_alive = false;
    double now_seconds = 0.0;
    double last_request_seconds = 0.0;
    double minimum_interval_seconds = 0.0;
    double squared_distance = 0.0;
    double maximum_squared_distance = 0.0;
    double claimed_damage = 0.0;
    double server_damage = 0.0;
    double claimed_health = 0.0;
    double server_health = 0.0;
    int claimed_score = 0;
    int server_score = 0;
    std::uint32_t sequence = 0;
    std::uint32_t last_sequence = 0;
};

AUTHORITYARENACORE_API DecisionCode ValidateAbilityRequest(const AbilityRequest& request) noexcept;
AUTHORITYARENACORE_API DecisionCode ValidateAttackRequest(const AttackRequest& request) noexcept;
AUTHORITYARENACORE_API DecisionCode ValidateRespawnRequest(const RespawnRequest& request) noexcept;
AUTHORITYARENACORE_API DecisionCode ValidateAuthorityProbe(const AuthorityProbeRequest& request) noexcept;
AUTHORITYARENACORE_API const char* ToString(DecisionCode code) noexcept;
} // namespace AuthorityArena::Core
