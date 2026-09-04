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

AUTHORITYARENACORE_API DecisionCode ValidateAbilityRequest(const AbilityRequest& request) noexcept;
AUTHORITYARENACORE_API DecisionCode ValidateAttackRequest(const AttackRequest& request) noexcept;
AUTHORITYARENACORE_API DecisionCode ValidateRespawnRequest(const RespawnRequest& request) noexcept;
AUTHORITYARENACORE_API const char* ToString(DecisionCode code) noexcept;
} // namespace AuthorityArena::Core
