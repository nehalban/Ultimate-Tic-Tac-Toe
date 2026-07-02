#include "ultimate_ttt.hpp"
#include "negamax_bot.hpp"
#include "mcts_bot.hpp"
#include "random_bot.hpp"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <vector>

int main() {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);
    std::cout.tie(nullptr);

    std::cout << "Pick mode:\n"
              << "  1) play  (human vs human)\n"
              << "  2) bot   (human vs AI - choose negamax or MCTS, you are X)\n"
              << "  3) self  (negamax vs negamax)\n"
              << "  4) train (negamax self-play weight tuning)\n"
              << "Enter 1-4: ";
    int mode = 1;
    if (!(std::cin >> mode)) return 0;

    if (mode == 1) {
        ult_ttt game;
        game.play();
        return 0;
    }

    if (mode == 2) {
        std::cout << "Choose opponent AI:\n"
                  << "  1) Negamax (alpha-beta)\n"
                  << "  2) MCTS\n"
                  << "  3) Random (uniform legal move)\n"
                  << "Enter 1-3: ";
        int algo = 1;
        if (!(std::cin >> algo)) return 0;

        int depth = 3;
        mcts::Config mc;
        if (algo == 2) {
            std::cout << "MCTS iterations (e.g. 20000): ";
            if (!(std::cin >> mc.iterations)) return 0;
        } else if (algo == 3) {
            // Random needs no configuration.
        } else {
            std::cout << "Bot depth (suggest 2-4): ";
            if (!(std::cin >> depth)) return 0;
        }

        negamax::State st;
        st.to_move = 'X';
        st.forced_br = -1;
        st.forced_bc = -1;

        negamax::Weights w;
        std::uint32_t seed = 123;

        while (true) {
            st.g.print_board();

            // Normalize the forced board for BOTH players before moving: if the last
            // move sent the mover to an already-finished board, it becomes a free
            // choice. Without this the bot picks a legal free-choice move that
            // apply_move then rejects against the raw (finished) forced board - which
            // silently loops forever.
            auto forced = st.g.normalize_forced(st.forced_br, st.forced_bc);
            st.forced_br = forced.first;
            st.forced_bc = forced.second;

            if (st.to_move == 'X') {
                ult_ttt::Move m;

                if (st.forced_br != -1) {
                    std::cout << "Forced big block: (" << st.forced_br << "," << st.forced_bc << ")\n";
                    m.br = st.forced_br;
                    m.bc = st.forced_bc;
                } else {
                    std::cout << "Enter big block row col: ";
                    if (!(std::cin >> m.br >> m.bc)) return 0;
                }
                std::cout << "Enter small cell row col: ";
                if (!(std::cin >> m.r >> m.c)) return 0;

                ult_ttt::ApplyResult res;
                if (!negamax::apply(st, m, res)) {
                    std::cout << "Illegal move.\n";
                    continue;
                }
                if (res.game_over) {
                    st.g.print_board();
                    if (res.winner == 'X' || res.winner == 'O') std::cout << res.winner << " wins!\n";
                    else std::cout << "Draw.\n";
                    return 0;
                }
            } else {
                ult_ttt::Move bm;
                long long work = 0;
                const char* unit = "";
                auto t0 = std::chrono::steady_clock::now();
                if (algo == 2) {
                    bm = mcts::best_move(st.g, st.to_move, st.forced_br, st.forced_bc, mc);
                    work = mc.iterations;
                    unit = "paths searched";
                    mc.seed++; // vary rollouts across moves
                } else if (algo == 3) {
                    bm = random_bot::best_move(st.g, st.to_move, st.forced_br, st.forced_bc, seed++);
                } else {
                    bm = negamax::best_move(st, depth, w, seed++);
                    work = negamax::node_counter();
                    unit = "positions searched";
                }
                double secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
                std::cout << "Bot plays: (" << bm.br << "," << bm.bc << ") (" << bm.r << "," << bm.c << ")\n";
                char buf[96];
                if (algo == 3) std::snprintf(buf, sizeof(buf), "  (random move in %.3f seconds)\n", secs);
                else std::snprintf(buf, sizeof(buf), "  (%lld %s in %.3f seconds)\n", work, unit, secs);
                std::cout << buf;
                ult_ttt::ApplyResult res;
                if (!negamax::apply(st, bm, res)) {
                    std::cout << "Internal error: bot produced an illegal move; aborting.\n";
                    return 1;
                }
                if (res.game_over) {
                    st.g.print_board();
                    if (res.winner == 'X' || res.winner == 'O') std::cout << res.winner << " wins!\n";
                    else std::cout << "Draw.\n";
                    return 0;
                }
            }
        }
    }

    if (mode == 3) {
        int depth = 3;
        int games = 10;
        std::cout << "Depth: ";
        if (!(std::cin >> depth)) return 0;
        std::cout << "Games: ";
        if (!(std::cin >> games)) return 0;

        negamax::Weights w;
        int xw = 0, ow = 0, dr = 0;
        for (int i = 0; i < games; i++) {
            int r = negamax::play_bot_game(w, w, depth, 1000u + (unsigned)i);
            if (r == 1) xw++;
            else if (r == -1) ow++;
            else dr++;
        }
        std::cout << "Results (X vs O): Xwins=" << xw << " Owins=" << ow << " Draws=" << dr << "\n";
        return 0;
    }

    if (mode == 4) {
        int depth = 3;
        int iters = 40;
        int games = 10;
        std::cout << "Train depth: ";
        if (!(std::cin >> depth)) return 0;
        std::cout << "Iterations: ";
        if (!(std::cin >> iters)) return 0;
        std::cout << "Games/iter: ";
        if (!(std::cin >> games)) return 0;

        negamax::TrainConfig cfg;
        cfg.depth = depth;
        cfg.iterations = iters;
        cfg.games_per_iter = games;

        negamax::Weights w0;
        std::vector<int> hist;
        negamax::Weights w1 = negamax::train(w0, cfg, &hist);

        std::cout << "Training done. New weights:\n";
        std::cout << " two_in_row=" << w1.two_in_row
                  << " one_in_row=" << w1.one_in_row
                  << " opp_two_in_row=" << w1.opp_two_in_row
                  << " opp_one_in_row=" << w1.opp_one_in_row
                  << " center=" << w1.center
                  << " corner=" << w1.corner
                  << " edge=" << w1.edge
                  << " send_to_finished_penalty=" << w1.send_to_finished_penalty
                  << "\n";
        return 0;
    }

    std::cout << "Unknown mode.\n";
    return 0;
}
