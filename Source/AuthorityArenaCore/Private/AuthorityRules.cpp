#include "AuthorityRules.h"

#include <cmath>

namespace AuthorityArena::Core
{
namespace
{
bool IsFiniteAndNonNegative(const double value) noexcept
{
    return std::isfinite(value) && value >= 0.0;
}
} // namespace

DecisionCode ValidateAbilityRequest(const AbilityRequest& request) noexcept
{
    if (!request.has_authority)
    {
        return DecisionCode::NotAuthority;
    }
    if (!request.is_owner)
    {
        return DecisionCode::NotOwner;
    }
    if (!request.is_alive)
    {
        return DecisionCode::Dead;
    }
    if (request.is_stunned)
    {
        return DecisionCode::Stunned;
    }
    if (!IsFiniteAndNonNegative(request.now_seconds) ||
        !IsFiniteAndNonNegative(request.cooldown_ready_seconds) ||
        !IsFiniteAndNonNegative(request.energy) ||
        !IsFiniteAndNonNegative(request.energy_cost))
    {
        return DecisionCode::InvalidNumeric;
    }
    if (request.now_seconds < request.cooldown_ready_seconds)
    {
        return DecisionCode::OnCooldown;
    }
    if (request.energy < request.energy_cost)
    {
        return DecisionCode::InsufficientEnergy;
    }
    return DecisionCode::Allowed;
}

DecisionCode ValidateAttackRequest(const AttackRequest& request) noexcept
{
    const DecisionCode abilityDecision = ValidateAbilityRequest(request.ability);
    if (abilityDecision != DecisionCode::Allowed)
    {
        return abilityDecision;
    }
    if (!request.target_valid || !request.target_alive)
    {
        return DecisionCode::InvalidTarget;
    }
    if (!IsFiniteAndNonNegative(request.last_attack_seconds) ||
        !IsFiniteAndNonNegative(request.minimum_interval_seconds) ||
        !IsFiniteAndNonNegative(request.squared_distance) ||
        !IsFiniteAndNonNegative(request.maximum_squared_distance))
    {
        return DecisionCode::InvalidNumeric;
    }
    if (request.ability.now_seconds - request.last_attack_seconds < request.minimum_interval_seconds)
    {
        return DecisionCode::RateLimited;
    }
    if (request.squared_distance > request.maximum_squared_distance)
    {
        return DecisionCode::TargetOutOfRange;
    }
    return DecisionCode::Allowed;
}

DecisionCode ValidateRespawnRequest(const RespawnRequest& request) noexcept
{
    if (!request.has_authority)
    {
        return DecisionCode::NotAuthority;
    }
    if (!request.is_owner)
    {
        return DecisionCode::NotOwner;
    }
    if (!request.is_dead)
    {
        return DecisionCode::NotDead;
    }
    if (request.respawn_pending)
    {
        return DecisionCode::RespawnPending;
    }
    return DecisionCode::Allowed;
}

const char* ToString(const DecisionCode code) noexcept
{
    switch (code)
    {
    case DecisionCode::Allowed: return "Allowed";
    case DecisionCode::NotAuthority: return "NotAuthority";
    case DecisionCode::NotOwner: return "NotOwner";
    case DecisionCode::Dead: return "Dead";
    case DecisionCode::NotDead: return "NotDead";
    case DecisionCode::Stunned: return "Stunned";
    case DecisionCode::InsufficientEnergy: return "InsufficientEnergy";
    case DecisionCode::OnCooldown: return "OnCooldown";
    case DecisionCode::RateLimited: return "RateLimited";
    case DecisionCode::InvalidTarget: return "InvalidTarget";
    case DecisionCode::TargetOutOfRange: return "TargetOutOfRange";
    case DecisionCode::RespawnPending: return "RespawnPending";
    case DecisionCode::InvalidNumeric: return "InvalidNumeric";
    }
    return "InvalidNumeric";
}
} // namespace AuthorityArena::Core
