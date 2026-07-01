#pragma once

// Monte Carlo Tree Search bot for Ultimate Tic-Tac-Toe.
//
// Uses the same compact bb::BitState representation as the negamax engine, which
// makes the copy-heavy MCTS rollouts cheap. Unlike a depth-limited minimax, MCTS
// is anytime (more iterations -> stronger) and needs no evaluation weights: it
// estimates move quality from random playouts to the end of the game.
//
// Algorithm: standard UCT.
//   selection    - descend the tree by the UCB1 rule until a node with an
//                  unexpanded move or a terminal node is reached
//   expansion    - add one child for a random untried move
//   simulation   - play uniformly random legal moves to a terminal position
//   backpropagation - update visit counts and win totals up the path
//
// A node's `wins` are stored from the perspective of the player who moved INTO
// that node (i.e. the player to move at its parent), so a parent selecting among
// its children maximizes its own win rate directly.

#include "bitboard.hpp"
#include "ultimate_ttt.hpp"

#include <cmath>
#include <cstdint>
#include <random>
#include <vector>

namespace mcts {

struct Config {
    int iterations = 20000;       // number of MCTS simulations per move
    double c = 1.4142135623730951; // UCB1 exploration constant (~sqrt(2))
    std::uint32_t seed = 12345;
};

// Winner of a position: 'X', 'O', 'D' (draw/full), or '.' if not terminal.
inline char terminal_winner(const bb::BitState& st) {
    if (bb::is_win(st.meta_x)) return 'X';
    if (bb::is_win(st.meta_o)) return 'O';
    if (st.all_resolved()) return 'D';
    return '.';
}

struct Node {
    bb::BitState st;
    int parent = -1;
    std::uint8_t move_code = 0xFF; // board*9 + cell of the move from parent (0xFF at root)
    int visits = 0;
    double wins = 0.0;             // from the perspective of the mover into this node
    bool terminal = false;
    char winner = '.';
    std::vector<std::uint8_t> untried; // move codes not yet expanded
    std::vector<int> children;
};

// Play uniformly random legal moves from `st` until the game ends; return winner.
inline char rollout(bb::BitState st, std::mt19937& rng) {
    for (;;) {
        char w = terminal_winner(st);
        if (w != '.') return w;
        bb::BMove moves[81];
        int n = bb::gather_moves(st, moves);
        if (n == 0) return 'D'; // safety: no legal move but not flagged terminal
        int idx = static_cast<int>(rng() % static_cast<std::uint32_t>(n));
        bb::apply(st, moves[idx].board, moves[idx].cell);
    }
}

// Fill a fresh node's terminal status and untried-move list from its state.
inline void init_node(Node& nd) {
    char w = terminal_winner(nd.st);
    if (w != '.') { nd.terminal = true; nd.winner = w; return; }
    bb::BMove moves[81];
    int n = bb::gather_moves(nd.st, moves);
    nd.untried.reserve(static_cast<std::size_t>(n));
    for (int i = 0; i < n; i++)
        nd.untried.push_back(static_cast<std::uint8_t>(moves[i].board * 9 + moves[i].cell));
}

inline ult_ttt::Move best_move(const ult_ttt& g, char to_move, int forced_br, int forced_bc,
                               const Config& cfg) {
    const int iterations = cfg.iterations > 0 ? cfg.iterations : 1; // always expand >=1 child

    std::vector<Node> pool;
    pool.reserve(static_cast<std::size_t>(iterations) + 2);

    Node root;
    root.st = bb::from_ult(g, to_move, forced_br, forced_bc);
    init_node(root);
    pool.push_back(root);

    if (pool[0].terminal || pool[0].untried.empty())
        return ult_ttt::Move{-1, -1, -1, -1}; // nothing to choose

    std::mt19937 rng(cfg.seed);

    for (int it = 0; it < iterations; it++) {
        // --- Selection: descend fully-expanded, non-terminal nodes by UCB1. ---
        int node = 0;
        for (;;) {
            if (pool[node].terminal || !pool[node].untried.empty()) break;
            if (pool[node].children.empty()) break;
            const double logN = std::log(static_cast<double>(pool[node].visits));
            double best_u = -1e300;
            int best_child = pool[node].children[0];
            for (int c : pool[node].children) {
                const Node& ch = pool[c];
                const double exploit = ch.wins / ch.visits;
                const double explore = cfg.c * std::sqrt(logN / ch.visits);
                const double u = exploit + explore;
                if (u > best_u) { best_u = u; best_child = c; }
            }
            node = best_child;
        }

        // --- Expansion: add one random untried move as a child. ---
        if (!pool[node].terminal && !pool[node].untried.empty()) {
            std::uint32_t k = rng() % static_cast<std::uint32_t>(pool[node].untried.size());
            std::uint8_t code = pool[node].untried[k];
            pool[node].untried[k] = pool[node].untried.back();
            pool[node].untried.pop_back();

            Node child;
            child.st = pool[node].st;
            bb::apply(child.st, code / 9, code % 9);
            child.parent = node;
            child.move_code = code;
            init_node(child);

            pool.push_back(child);             // may reallocate; only index into pool afterward
            int cidx = static_cast<int>(pool.size()) - 1;
            pool[node].children.push_back(cidx);
            node = cidx;
        }

        // --- Simulation. ---
        char winner = pool[node].terminal ? pool[node].winner : rollout(pool[node].st, rng);

        // --- Backpropagation. ---
        for (int cur = node; cur != -1; cur = pool[cur].parent) {
            Node& nd = pool[cur];
            nd.visits++;
            char mover_into = (nd.st.to_move == 'X') ? 'O' : 'X';
            nd.wins += (winner == mover_into) ? 1.0 : (winner == 'D' ? 0.5 : 0.0);
        }
    }

    // Pick the most-visited child (robust choice).
    int best_child = -1, most = -1;
    for (int c : pool[0].children) {
        if (pool[c].visits > most) { most = pool[c].visits; best_child = c; }
    }
    if (best_child == -1) return ult_ttt::Move{-1, -1, -1, -1};

    std::uint8_t code = pool[best_child].move_code;
    int board = code / 9, cell = code % 9;
    return ult_ttt::Move{board / 3, board % 3, cell / 3, cell % 3};
}

} // namespace mcts
