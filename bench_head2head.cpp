// Head-to-head: Negamax (by depth) vs MCTS (by iterations), plus per-config
// time-per-move. Each pairing plays `games` full games, alternating who moves
// first. Reports results from MCTS's perspective. Build:
//   g++ -O3 -std=c++17 bench_head2head.cpp -o h2h && ./h2h
#include "ultimate_ttt.hpp"
#include "negamax_bot.hpp"
#include "mcts_bot.hpp"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <random>

using namespace std::chrono;

enum Kind { NEGA, MCTS };
struct Bot { Kind k; int p; }; // p = depth (NEGA) or iterations (MCTS)

static ult_ttt::Move pick(Bot b, const ult_ttt& g, char p, int fbr, int fbc, std::mt19937& rng) {
    if (b.k == NEGA) {
        negamax::State st; st.g = g; st.to_move = p; st.forced_br = fbr; st.forced_bc = fbc;
        negamax::Weights w;
        return negamax::best_move(st, b.p, w, rng());
    }
    mcts::Config mc; mc.iterations = b.p; mc.seed = rng();
    return mcts::best_move(g, p, fbr, fbc, mc);
}

static char play(Bot X, Bot O, std::uint32_t seed) {
    ult_ttt g; char p = 'X'; int fbr = -1, fbc = -1;
    std::mt19937 rng(seed);
    for (int ply = 0; ply < 81; ply++) {
        auto f = g.normalize_forced(fbr, fbc); fbr = f.first; fbc = f.second;
        ult_ttt::Move m = pick(p == 'X' ? X : O, g, p, fbr, fbc, rng);
        auto res = g.apply_move(m, p, fbr, fbc);
        if (!res.ok) return '?';
        if (res.game_over) return res.winner == 'X' ? 'X' : (res.winner == 'O' ? 'O' : 'D');
        fbr = res.next_br; fbc = res.next_bc; p = (p == 'X') ? 'O' : 'X';
    }
    return 'D';
}

// win-rate for `mcts` bot vs `nega` bot, alternating colors
static void h2h(Bot nega, Bot mcts_bot, int games) {
    int mw = 0, d = 0, nw = 0;
    for (int i = 0; i < games; i++) {
        std::uint32_t s = 777u + static_cast<std::uint32_t>(i) * 2654435761u;
        char r; char mcts_mark;
        if (i % 2 == 0) { r = play(mcts_bot, nega, s); mcts_mark = 'X'; }
        else            { r = play(nega, mcts_bot, s); mcts_mark = 'O'; }
        if (r == 'D') d++;
        else if (r == mcts_mark) mw++;
        else nw++;
    }
    double mwr = 100.0 * (mw + 0.5 * d) / games;
    std::printf("Nega d%-2d vs MCTS %-6d  games=%3d  MCTS: W=%3d D=%3d L=%3d  MCTS winrate=%5.1f%%\n",
                nega.p, mcts_bot.p, games, mw, d, nw, mwr);
}

// average ms/move for a config, sampled along one self-played line
static double ms_per_move(Bot b) {
    ult_ttt g; char p = 'X'; int fbr = -1, fbc = -1;
    std::mt19937 rng(4242);
    long long total_us = 0; int moves = 0;
    for (int ply = 0; ply < 24; ply++) {
        auto f = g.normalize_forced(fbr, fbc); fbr = f.first; fbc = f.second;
        auto t0 = high_resolution_clock::now();
        ult_ttt::Move m = pick(b, g, p, fbr, fbc, rng);
        auto t1 = high_resolution_clock::now();
        total_us += duration_cast<microseconds>(t1 - t0).count(); moves++;
        auto res = g.apply_move(m, p, fbr, fbc);
        if (res.game_over) break;
        fbr = res.next_br; fbc = res.next_bc; p = (p == 'X') ? 'O' : 'X';
    }
    return total_us / 1000.0 / moves;
}

int main() {
    std::printf("== Time per move (ms, single thread) ==\n");
    for (int d : {2, 4, 6}) std::printf("Negamax d%-2d : %.2f ms/move\n", d, ms_per_move({NEGA, d}));
    for (int n : {1000, 5000, 20000}) std::printf("MCTS %-6d : %.2f ms/move\n", n, ms_per_move({MCTS, n}));

    std::printf("\n== Head-to-head (win-rate from MCTS perspective) ==\n");
    const int G = 40;
    for (int d : {2, 4, 6})
        for (int n : {1000, 5000, 20000})
            h2h({NEGA, d}, {MCTS, n}, G);
    return 0;
}
