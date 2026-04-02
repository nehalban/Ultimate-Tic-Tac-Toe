#pragma once

#include "ultimate_ttt.hpp"

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

// Scores a 3-cell line for "me". Treats 'D' as a hard blocker (line cannot be completed).
inline int score_line(char a, char b, char c, char me, const Weights& w) {
    const char opp = other(me);
    if (a == 'D' || b == 'D' || c == 'D') return 0;

    int me_cnt = (a == me) + (b == me) + (c == me);
    int opp_cnt = (a == opp) + (b == opp) + (c == opp);
    int empty_cnt = (a == '.') + (b == '.') + (c == '.');

    if (me_cnt && opp_cnt) return 0;
    if (me_cnt == 2 && empty_cnt == 1) return w.two_in_row;
    if (me_cnt == 1 && empty_cnt == 2) return w.one_in_row;
    if (opp_cnt == 2 && empty_cnt == 1) return -w.opp_two_in_row;
    if (opp_cnt == 1 && empty_cnt == 2) return -w.opp_one_in_row;
    return 0;
}

inline int score_board(const ttt& b, char me, const Weights& w) {
    int s = 0;
    for (int r = 0; r < 3; r++) {
        s += score_line(b.cell[r][0], b.cell[r][1], b.cell[r][2], me, w);
    }
    for (int c = 0; c < 3; c++) {
        s += score_line(b.cell[0][c], b.cell[1][c], b.cell[2][c], me, w);
    }
    s += score_line(b.cell[0][0], b.cell[1][1], b.cell[2][2], me, w);
    s += score_line(b.cell[0][2], b.cell[1][1], b.cell[2][0], me, w);
    return s;
}

inline int evaluate(const State& st, char me, const Weights& w) {
    // Terminal checks on meta board.
    char meta_w = st.g.meta_winner();
    if (meta_w == me) return w.meta_win;
    if (meta_w == other(me)) return -w.meta_win;
    if (st.g.big_board.is_full()) return 0;

    int score = 0;

    // Meta board heuristics (treat D as blocker).
    score += 20 * score_board(st.g.big_board, me, w);

    // Finished small boards are valuable.
    for (int br = 0; br < 3; br++) {
        for (int bc = 0; bc < 3; bc++) {
            char status = st.g.small_status[br][bc];
            if (status == me) score += w.small_win;
            else if (status == other(me)) score -= w.small_win;
            else if (status == '.') {
                score += score_board(st.g.small_boards[br][bc], me, w);
            }
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

inline int negamax(State st, int depth, int alpha, int beta, char me, const Weights& w) {
    // Check terminal by meta board winner/full (apply_move already maintains big_board).
    char meta_w = st.g.meta_winner();
    if (meta_w == me) return w.meta_win;
    if (meta_w == other(me)) return -w.meta_win;
    if (st.g.big_board.is_full()) return 0;
    if (depth <= 0) return evaluate(st, me, w);

    auto moves = st.g.legal_moves(st.forced_br, st.forced_bc);
    if (moves.empty()) return 0;

    int best = std::numeric_limits<int>::min() / 4;
    for (const auto& m : moves) {
        State child = st;
        ult_ttt::ApplyResult res;
        if (!apply(child, m, res)) continue;

        // Small heuristic: if we "send" opponent to a finished board, they get freedom; penalize.
        int send_pen = 0;
        if (res.next_br == -1 && res.next_bc == -1) send_pen = -w.send_to_finished_penalty;

        int val = -negamax(child, depth - 1, -beta, -alpha, me, w) + send_pen;
        if (val > best) best = val;
        if (val > alpha) alpha = val;
        if (alpha >= beta) break;
    }
    return best;
}

inline ult_ttt::Move best_move(const State& st, int depth, const Weights& w, std::uint32_t seed = 1) {
    auto moves = st.g.legal_moves(st.forced_br, st.forced_bc);
    if (moves.empty()) return ult_ttt::Move{-1, -1, -1, -1};

    std::mt19937 rng(seed);
    std::shuffle(moves.begin(), moves.end(), rng); // tie-break randomness

    const char me = st.to_move;
    int alpha = std::numeric_limits<int>::min() / 4;
    int beta = std::numeric_limits<int>::max() / 4;

    int bestScore = std::numeric_limits<int>::min() / 4;
    ult_ttt::Move best = moves.front();

    for (const auto& m : moves) {
        State child = st;
        ult_ttt::ApplyResult res;
        if (!apply(child, m, res)) continue;

        int move_bonus = pos_bonus(m.r, m.c, w);
        int val = -negamax(child, depth - 1, -beta, -alpha, me, w) + move_bonus;

        if (val > bestScore) {
            bestScore = val;
            best = m;
        }
        if (val > alpha) alpha = val;
    }
    return best;
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

