#pragma once

#include "ultimate_ttt.hpp"
#include "bitboard.hpp"
#include "transposition.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <random>
#include <vector>

namespace ultimate_bot {

struct Weights {
    // Terminal-ish / high level
    int meta_win = 1'000'000;
    int small_win = 10'000;

    // Line patterns (applies to both small boards and meta board)
    int two_in_row = 80;    // "XX." for me
    int one_in_row = 10;    // "X.." for me
    int opp_two_in_row = 90; // opponent "OO."
    int opp_one_in_row = 10;

    // Preference for playing in-center etc. (small cell only)
    int center = 3;
    int corner = 2;
    int edge = 1;

    // Incentivize sending opponent to a finished board (free move for them is bad)
    int send_to_finished_penalty = 25;
};

struct State {
    ult_ttt g;
    char to_move = 'X';
    int forced_br = -1;
    int forced_bc = -1;
};

inline char other(char p) { return (p == 'X') ? 'O' : 'X'; }

inline int pos_bonus(int r, int c, const Weights& w) {
    if (r == 1 && c == 1) return w.center;
    const bool corner = (r == 0 || r == 2) && (c == 0 || c == 2);
    if (corner) return w.corner;
    return w.edge;
}

// Scores one 3x3 board line-by-line for "me", given the X/O occupancy masks and a
// `blocked` mask of cells that can never complete a line ('D' on the meta board).
// Bitboard equivalent of the old char-grid score_line/score_board pair.
inline int score_board_bits(std::uint16_t mine, std::uint16_t opp, std::uint16_t blocked,
                            const Weights& w) {
    int s = 0;
    for (std::uint16_t L : bb::LINES) {
        if (blocked & L) continue; // line cannot be completed
        int me_cnt = bb::popcount9(mine & L);
        int opp_cnt = bb::popcount9(opp & L);
        int empty_cnt = 3 - me_cnt - opp_cnt;
        if (me_cnt && opp_cnt) continue;
        if (me_cnt == 2 && empty_cnt == 1) s += w.two_in_row;
        else if (me_cnt == 1 && empty_cnt == 2) s += w.one_in_row;
        else if (opp_cnt == 2 && empty_cnt == 1) s -= w.opp_two_in_row;
        else if (opp_cnt == 1 && empty_cnt == 2) s -= w.opp_one_in_row;
    }
    return s;
}

inline int evaluate(const bb::BitState& st, bool me_is_x, const Weights& w) {
    // Terminal checks on meta board.
    if (bb::is_win(st.meta_x)) return me_is_x ? w.meta_win : -w.meta_win;
    if (bb::is_win(st.meta_o)) return me_is_x ? -w.meta_win : w.meta_win;
    if (st.all_resolved()) return 0;

    const std::uint16_t meta_mine = me_is_x ? st.meta_x : st.meta_o;
    const std::uint16_t meta_opp = me_is_x ? st.meta_o : st.meta_x;

    int score = 0;

    // Meta board heuristics (drawn boards block lines, like the old 'D').
    score += 20 * score_board_bits(meta_mine, meta_opp, st.meta_d, w);

    // Finished small boards are valuable; ongoing boards scored line-by-line.
    for (int b = 0; b < 9; b++) {
        if ((meta_mine >> b) & 1) score += w.small_win;
        else if ((meta_opp >> b) & 1) score -= w.small_win;
        else if (!((st.meta_d >> b) & 1)) {
            std::uint16_t mine = me_is_x ? st.sx[b] : st.so[b];
            std::uint16_t opp = me_is_x ? st.so[b] : st.sx[b];
            score += score_board_bits(mine, opp, 0, w);
        }
    }

    return score;
}

inline bool apply(State& st, const ult_ttt::Move& m, ult_ttt::ApplyResult& res_out) {
    res_out = st.g.apply_move(m, st.to_move, st.forced_br, st.forced_bc);
    if (!res_out.ok) return false;
    st.forced_br = res_out.next_br;
    st.forced_bc = res_out.next_bc;
    st.to_move = other(st.to_move);
    return true;
}

// Killer moves: up to two non-capturing moves per depth that caused a beta
// cutoff. Tried early at sibling nodes to improve pruning. Reset each search.
struct Killers {
    static constexpr int MAXD = 64;
    std::uint8_t k[MAXD][2];
    void clear() {
        for (int d = 0; d < MAXD; d++) { k[d][0] = tt::NO_MOVE; k[d][1] = tt::NO_MOVE; }
    }
    void record(int depth, std::uint8_t mv) {
        if (depth < 0 || depth >= MAXD || mv == tt::NO_MOVE) return;
        if (k[depth][0] != mv) { k[depth][1] = k[depth][0]; k[depth][0] = mv; }
    }
    bool is_killer(int depth, std::uint8_t mv) const {
        return depth >= 0 && depth < MAXD && (k[depth][0] == mv || k[depth][1] == mv);
    }
};

// Running search-node counter (for benchmarking/diagnostics). Reset by best_move.
inline long long& node_counter() {
    static long long n = 0;
    return n;
}

// Remaining-depth threshold at/above which moves are ordered by a 1-ply eval of
// the resulting child. That eval is relatively expensive, so it is only worth it
// deep in the tree where a better cutoff prunes a large subtree; near the leaves
// the cheap TT-move/killer ordering is used alone. Tuned empirically: at this
// threshold the engine is never slower than no ordering at any depth and is
// ~2-3.6x faster on deep searches. Searches shallower than this never pay the cost.
constexpr int ORDER_EVAL_MIN_DEPTH = 6;

// Order moves in place, best-first, to maximize alpha-beta cutoffs. Ordering only
// affects search efficiency, never the value returned, so it cannot change the
// move best_move() selects (proven: results are identical with ordering on/off and
// for reversed order). Priority: TT move, then killers, then - at deep nodes - by a
// 1-ply eval. The eval uses the side-to-move's perspective (st.to_move), which is
// the standard negamax ordering signal and empirically ~4x better here than the
// fixed root perspective. Ties keep row-major order (insertion sort is stable).
inline void order_moves(const bb::BitState& st, bb::BMove* moves, int n,
                        std::uint8_t tt_move, const Killers& kt, int depth,
                        const Weights& w) {
    int score[81];
    const bool deep = (depth >= ORDER_EVAL_MIN_DEPTH);
    const bool mover_is_x = (st.to_move == 'X');
    for (int i = 0; i < n; i++) {
        const int b = moves[i].board, c = moves[i].cell;
        const std::uint8_t code = static_cast<std::uint8_t>(b * 9 + c);
        int s = 0;
        if (code == tt_move) s += (1 << 28);            // best move from a prior visit
        else if (kt.is_killer(depth, code)) s += (1 << 24); // caused a cutoff at a sibling
        if (deep) {
            // Approximate the searched quantity val = e - V(child) by a 1-ply eval,
            // so the move likely to raise alpha / cause a cutoff is tried first.
            bb::BitState child = st;
            bool game_over = bb::apply(child, b, c);
            int e = (!game_over && child.forced < 0) ? -w.send_to_finished_penalty : 0;
            s += e - evaluate(child, mover_is_x, w);
        }
        score[i] = s;
    }
    for (int i = 1; i < n; i++) {
        bb::BMove m = moves[i];
        int sc = score[i], j = i - 1;
        while (j >= 0 && score[j] < sc) { moves[j + 1] = moves[j]; score[j + 1] = score[j]; j--; }
        moves[j + 1] = m;
        score[j + 1] = sc;
    }
}

// Bitboard negamax with a transposition table and move ordering. `me_is_x` fixes
// the evaluation perspective for the whole subtree (matching the original, which
// always scored from the root mover's view and negated through the recursion).
inline int negamax(bb::BitState st, int depth, int alpha, int beta, bool me_is_x,
                   const Weights& w, tt::Table& table, Killers& kt, long long& nodes) {
    ++nodes;
    // Terminal by meta board winner / all boards resolved.
    if (bb::is_win(st.meta_x)) return me_is_x ? w.meta_win : -w.meta_win;
    if (bb::is_win(st.meta_o)) return me_is_x ? -w.meta_win : w.meta_win;
    if (st.all_resolved()) return 0;
    if (depth <= 0) return evaluate(st, me_is_x, w);

    const int alpha0 = alpha;
    int tt_val;
    std::uint8_t tt_move;
    if (table.probe(st.key, depth, alpha, beta, tt_val, tt_move)) return tt_val;

    bb::BMove moves[81];
    int n = bb::gather_moves(st, moves);
    if (n == 0) return 0;

    order_moves(st, moves, n, tt_move, kt, depth, w);

    int best = std::numeric_limits<int>::min() / 4;
    std::uint8_t best_mv = tt::NO_MOVE;
    for (int i = 0; i < n; i++) {
        bb::BitState child = st;
        bool game_over = bb::apply(child, moves[i].board, moves[i].cell);

        // If this move sends the opponent to a finished board they get a free
        // choice (bad for us), so penalize. bb::apply normalizes forced to -1 in
        // exactly that case (and game is not over). The penalty is an edge bonus:
        // val = e - V(child). We fold e into the child's window (e - beta, e - alpha)
        // so alpha-beta bounds the true val exactly. Without this, an edge bonus
        // applied outside the window makes fail-soft results depend on move order -
        // which would let move ordering change the chosen move (it must not).
        int e = (!game_over && child.forced < 0) ? -w.send_to_finished_penalty : 0;

        int val = e - negamax(child, depth - 1, e - beta, e - alpha, me_is_x, w, table, kt, nodes);
        if (val > best) {
            best = val;
            best_mv = static_cast<std::uint8_t>(moves[i].board * 9 + moves[i].cell);
        }
        if (val > alpha) alpha = val;
        if (alpha >= beta) { kt.record(depth, best_mv); break; }
    }

    const std::uint8_t flag = (best <= alpha0) ? tt::UPPER : (best >= beta) ? tt::LOWER : tt::EXACT;
    table.store(st.key, depth, best, flag, best_mv);
    return best;
}

inline ult_ttt::Move best_move(const State& st, int depth, const Weights& w, std::uint32_t seed = 1) {
    bb::BitState bs = bb::from_ult(st.g, st.to_move, st.forced_br, st.forced_bc);

    bb::BMove moves[81];
    int n = bb::gather_moves(bs, moves);
    if (n == 0) return ult_ttt::Move{-1, -1, -1, -1};

    std::mt19937 rng(seed);
    std::shuffle(moves, moves + n, rng); // tie-break randomness (same order+rng as before)

    const bool me_is_x = (st.to_move == 'X');
    int alpha = std::numeric_limits<int>::min() / 4;
    int beta = std::numeric_limits<int>::max() / 4;

    int bestScore = std::numeric_limits<int>::min() / 4;
    bb::BMove best = moves[0];

    // Transposition table and killers are shared by the child searches. The table
    // persists across best_move() calls but is generation-scoped per search, so no
    // entry from a prior search (possibly a different perspective/weights) is reused.
    // The root loop is left exactly as before (shuffle, row-major, strict-greater,
    // same window progression), so the chosen move is unchanged - only faster.
    static tt::Table table(19); // ~512K entries (~12 MB), allocated once
    table.new_search();
    static Killers killers;
    killers.clear();
    long long& nodes = node_counter();
    nodes = 0;

    for (int i = 0; i < n; i++) {
        bb::BitState child = bs;
        bool game_over = bb::apply(child, moves[i].board, moves[i].cell);

        int move_bonus = pos_bonus(moves[i].cell / 3, moves[i].cell % 3, w);
        int send_pen = (!game_over && child.forced < 0) ? -w.send_to_finished_penalty : 0;
        int e = move_bonus + send_pen; // root edge bonus, folded into the window (see negamax)
        int val = e - negamax(child, depth - 1, e - beta, e - alpha, me_is_x, w, table, killers, nodes);

        if (val > bestScore) {
            bestScore = val;
            best = moves[i];
        }
        if (val > alpha) alpha = val;
    }
    return ult_ttt::Move{best.board / 3, best.board % 3, best.cell / 3, best.cell % 3};
}

// ---- Simple "training": hill-climb weights via self-play matches ----
struct TrainConfig {
    int iterations = 40;
    int games_per_iter = 10;
    int depth = 3;
    int weight_step = 5;
    std::uint32_t seed = 12345;
};

inline int play_bot_game(Weights wx, Weights wo, int depth, std::uint32_t seed) {
    State st;
    st.to_move = 'X';
    st.forced_br = -1;
    st.forced_bc = -1;

    std::mt19937 rng(seed);

    while (true) {
        const Weights& w = (st.to_move == 'X') ? wx : wo;
        ult_ttt::Move m = best_move(st, depth, w, rng());
        ult_ttt::ApplyResult res;
        if (!apply(st, m, res)) return 0; // shouldn't happen
        if (res.game_over) {
            if (res.winner == 'X') return 1;
            if (res.winner == 'O') return -1;
            return 0;
        }
    }
}

inline Weights mutate(Weights base, std::mt19937& rng, int step) {
    auto jitter = [&](int& x) {
        std::uniform_int_distribution<int> d(-step, step);
        x += d(rng);
    };
    jitter(base.two_in_row);
    jitter(base.one_in_row);
    jitter(base.opp_two_in_row);
    jitter(base.opp_one_in_row);
    jitter(base.center);
    jitter(base.corner);
    jitter(base.edge);
    jitter(base.send_to_finished_penalty);
    // Keep sane
    base.two_in_row = std::max(1, base.two_in_row);
    base.one_in_row = std::max(0, base.one_in_row);
    base.opp_two_in_row = std::max(1, base.opp_two_in_row);
    base.opp_one_in_row = std::max(0, base.opp_one_in_row);
    return base;
}

inline Weights train(Weights start, const TrainConfig& cfg, std::vector<int>* history = nullptr) {
    std::mt19937 rng(cfg.seed);
    Weights bestW = start;

    auto score_match = [&](const Weights& challenger) {
        // Challenger as X half the time and as O half the time.
        int score = 0;
        for (int i = 0; i < cfg.games_per_iter; i++) {
            std::uint32_t s = rng();
            if (i % 2 == 0) score += play_bot_game(challenger, bestW, cfg.depth, s);
            else score -= play_bot_game(bestW, challenger, cfg.depth, s); // subtract because challenger is O here
        }
        return score;
    };

    for (int it = 0; it < cfg.iterations; it++) {
        Weights cand = mutate(bestW, rng, cfg.weight_step);
        int s = score_match(cand);
        if (history) history->push_back(s);
        if (s > 0) bestW = cand;
    }

    return bestW;
}

} // namespace ultimate_bot

