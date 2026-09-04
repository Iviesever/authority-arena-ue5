#pragma once

#include <string>
#include <vector>

#ifndef AUTHORITYARENACORE_API
#define AUTHORITYARENACORE_API
#endif

namespace AuthorityArena::Core
{
struct PlayerSnapshot
{
    std::string connection_id;
    double health = 0.0;
    double energy = 0.0;
    int score = 0;
    int deaths = 0;
    bool alive = false;
};

struct FinalSnapshot
{
    std::vector<PlayerSnapshot> players;
};

struct ConsistencyResult
{
    bool consistent = false;
    std::string reason;
};

AUTHORITYARENACORE_API ConsistencyResult CompareSnapshots(
    const FinalSnapshot& authority,
    const FinalSnapshot& observed,
    double attribute_tolerance = 0.001) noexcept;
} // namespace AuthorityArena::Core
