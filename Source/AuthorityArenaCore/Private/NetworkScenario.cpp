#include "NetworkScenario.h"

namespace AuthorityArena::Core
{
std::optional<NetworkScenario> ParseScenarioName(const std::string_view name) noexcept
{
    if (name == "baseline")
    {
        return NetworkScenario{NetworkScenarioKind::Baseline, 0, 0, 0};
    }
    if (name == "lag60")
    {
        return NetworkScenario{NetworkScenarioKind::Lag60, 60, 0, 0};
    }
    if (name == "lag120")
    {
        return NetworkScenario{NetworkScenarioKind::Lag120, 120, 0, 0};
    }
    if (name == "jitter")
    {
        return NetworkScenario{NetworkScenarioKind::Jitter, 90, 30, 0};
    }
    if (name == "loss")
    {
        return NetworkScenario{NetworkScenarioKind::Loss, 80, 15, 2};
    }
    return std::nullopt;
}
} // namespace AuthorityArena::Core
