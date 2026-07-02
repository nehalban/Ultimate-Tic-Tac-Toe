#pragma once

// Trivial baseline opponent: plays a uniformly random legal move. Useful as a
// sanity check and as a benchmark yardstick for the real engines.

#include "ultimate_ttt.hpp"

#include <cstdint>
#include <random>

namespace random_bot {

inline ult_ttt::Move best_move(const ult_ttt& g, char to_move, int forced_br, int forced_bc,
                               std::uint32_t seed) {
    (void)to_move;
    auto moves = g.legal_moves(forced_br, forced_bc); // already normalizes the forced board
    if (moves.empty()) return ult_ttt::Move{-1, -1, -1, -1};
    std::mt19937 rng(seed);
    return moves[rng() % moves.size()];
}

} // namespace random_bot
