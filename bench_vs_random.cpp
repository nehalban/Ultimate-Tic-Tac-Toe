// Benchmark: Negamax (by depth) and MCTS (by iterations) vs a uniform-random bot.
// Each configuration plays `games` full games, alternating who moves first (so the
// bot plays X in half and O in half). Prints W/D/L and win-rate (win + 0.5*draw)
// from the engine's perspective. Build:
//   g++ -O3 -std=c++17 bench_vs_random.cpp -o bench && ./bench
#include "ultimate_ttt.hpp"
#include "negamax_bot.hpp"
#include "mcts_bot.hpp"
#include "random_bot.hpp"

#include <cstdint>
#include <cstdio>
#include <random>

enum Kind { RANDOM, NEGA, MCTS };
struct Bot { Kind k; int param; }; // param = depth (NEGA) or iterations (MCTS)

static ult_ttt::Move pick(Bot b, const ult_ttt& g, char p, int fbr, int fbc, std::mt19937& rng) {
    if (b.k == RANDOM) return random_bot::best_move(g, p, fbr, fbc, rng());
    if (b.k == NEGA) {
        negamax::State st; st.g = g; st.to_move = p; st.forced_br = fbr; st.forced_bc = fbc;
        negamax::Weights w;
        return negamax::best_move(st, b.param, w, rng());
    }
    mcts::Config mc; mc.iterations = b.param; mc.seed = rng();
    return mcts::best_move(g, p, fbr, fbc, mc);
}

// Play one full game. Returns 'X', 'O', or 'D'.
static char play(Bot X, Bot O, std::uint32_t seed) {
    ult_ttt g; char p = 'X'; int fbr = -1, fbc = -1;
    std::mt19937 rng(seed);
    for (int ply = 0; ply < 81; ply++) {
        auto f = g.normalize_forced(fbr, fbc); fbr = f.first; fbc = f.second;
        ult_ttt::Move m = pick(p == 'X' ? X : O, g, p, fbr, fbc, rng);
        auto res = g.apply_move(m, p, fbr, fbc);
        if (!res.ok) return '?';                    // must never happen
        if (res.game_over) return res.winner == 'X' ? 'X' : (res.winner == 'O' ? 'O' : 'D');
        fbr = res.next_br; fbc = res.next_bc; p = (p == 'X') ? 'O' : 'X';
    }
    return 'D';
}

static void eval(const char* name, Bot bot, int games) {
    int w = 0, d = 0, l = 0, bad = 0;
    Bot rnd{RANDOM, 0};
    for (int i = 0; i < games; i++) {
        std::uint32_t s = 12345u + static_cast<std::uint32_t>(i) * 2654435761u;
        char r = (i % 2 == 0) ? play(bot, rnd, s) : play(rnd, bot, s);
        char botmark = (i % 2 == 0) ? 'X' : 'O';
        if (r == '?') bad++;
        else if (r == 'D') d++;
        else if (r == botmark) w++;
        else l++;
    }
    double wr = 100.0 * (w + 0.5 * d) / games;
    std::printf("%-14s games=%4d  W=%4d  D=%3d  L=%3d  winrate=%5.1f%%%s\n",
                name, games, w, d, l, wr, bad ? "  [ILLEGAL!]" : "");
}

int main() {
    std::printf("== Negamax vs Random ==\n");
    eval("Negamax d1", {NEGA, 1}, 300);
    eval("Negamax d2", {NEGA, 2}, 300);
    eval("Negamax d3", {NEGA, 3}, 300);
    eval("Negamax d4", {NEGA, 4}, 200);
    eval("Negamax d5", {NEGA, 5}, 200);
    eval("Negamax d6", {NEGA, 6}, 100);
    std::printf("== MCTS vs Random ==\n");
    eval("MCTS 100",   {MCTS, 100},   300);
    eval("MCTS 500",   {MCTS, 500},   300);
    eval("MCTS 1000",  {MCTS, 1000},  200);
    eval("MCTS 5000",  {MCTS, 5000},  150);
    eval("MCTS 20000", {MCTS, 20000}, 80);
    return 0;
}
