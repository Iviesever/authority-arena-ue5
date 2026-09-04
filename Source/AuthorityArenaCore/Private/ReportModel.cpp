#include "ReportModel.h"

#include <cmath>

namespace AuthorityArena::Core
{
namespace
{
const PlayerSnapshot* FindPlayer(const FinalSnapshot& snapshot, const std::string& connectionId) noexcept
{
    for (const PlayerSnapshot& player : snapshot.players)
    {
        if (player.connection_id == connectionId)
        {
            return &player;
        }
    }
    return nullptr;
}
} // namespace

ConsistencyResult CompareSnapshots(
    const FinalSnapshot& authority,
    const FinalSnapshot& observed,
    const double attributeTolerance) noexcept
{
    if (!std::isfinite(attributeTolerance) || attributeTolerance < 0.0)
    {
        return {false, "invalid attribute tolerance"};
    }
    if (authority.players.size() != observed.players.size())
    {
        return {false, "player count mismatch"};
    }

    for (const PlayerSnapshot& expected : authority.players)
    {
        const PlayerSnapshot* actual = FindPlayer(observed, expected.connection_id);
        if (actual == nullptr)
        {
            return {false, "missing player " + expected.connection_id};
        }
        if (std::abs(expected.health - actual->health) > attributeTolerance)
        {
            return {false, "health mismatch for " + expected.connection_id};
        }
        if (std::abs(expected.energy - actual->energy) > attributeTolerance)
        {
            return {false, "energy mismatch for " + expected.connection_id};
        }
        if (expected.score != actual->score)
        {
            return {false, "score mismatch for " + expected.connection_id};
        }
        if (expected.deaths != actual->deaths)
        {
            return {false, "death count mismatch for " + expected.connection_id};
        }
        if (expected.alive != actual->alive)
        {
            return {false, "alive state mismatch for " + expected.connection_id};
        }
    }
    return {true, {}};
}
} // namespace AuthorityArena::Core
