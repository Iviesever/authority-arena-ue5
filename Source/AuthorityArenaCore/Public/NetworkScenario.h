#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

#ifndef AUTHORITYARENACORE_API
#define AUTHORITYARENACORE_API
#endif

namespace AuthorityArena::Core
{
enum class NetworkScenarioKind : std::uint8_t
{
    Baseline,
    Lag60,
    Lag120,
    Jitter,
    Loss,
};

struct NetworkScenario
{
    NetworkScenarioKind kind = NetworkScenarioKind::Baseline;
    int lag_ms = 0;
    int lag_variance_ms = 0;
    int loss_percent = 0;
};

AUTHORITYARENACORE_API std::optional<NetworkScenario> ParseScenarioName(std::string_view name) noexcept;
} // namespace AuthorityArena::Core
