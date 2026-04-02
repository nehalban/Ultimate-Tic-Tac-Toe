#include "ultimate_ttt.hpp"
#include "ultimate_bot.hpp"

#include <cstdint>
#include <iostream>
#include <vector>

int main() {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);
    std::cout.tie(nullptr);

    std::cout << "Pick mode:\n"
              << "  1) play (human vs human)\n"
              << "  2) bot  (human vs minimax, you are X)\n"
              << "  3) self (bot vs bot)\n"
              << "  4) train (simple self-play tuning)\n"
              << "Enter 1-4: ";
    int mode = 1;
    if (!(std::cin >> mode)) return 0;

    if (mode == 1) {
        ult_ttt game;
        game.play();
        return 0;
    }

    if (mode == 2) {
        int depth = 3;
        std::cout << "Bot depth (suggest 2-4): ";
        if (!(std::cin >> depth)) return 0;

        ultimate_bot::State st;
        st.to_move = 'X';
        st.forced_br = -1;
        st.forced_bc = -1;

        ultimate_bot::Weights w;
        std::uint32_t seed = 123;

        while (true) {
            st.g.print_board();
            if (st.to_move == 'X') {
                ult_ttt::Move m;
                auto forced = st.g.normalize_forced(st.forced_br, st.forced_bc);
                st.forced_br = forced.first;
                st.forced_bc = forced.second;

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
                if (!ultimate_bot::apply(st, m, res)) {
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
                ult_ttt::Move bm = ultimate_bot::best_move(st, depth, w, seed++);
                std::cout << "Bot plays: (" << bm.br << "," << bm.bc << ") (" << bm.r << "," << bm.c << ")\n";
                ult_ttt::ApplyResult res;
                (void)ultimate_bot::apply(st, bm, res);
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

        ultimate_bot::Weights w;
        int xw = 0, ow = 0, dr = 0;
        for (int i = 0; i < games; i++) {
            int r = ultimate_bot::play_bot_game(w, w, depth, 1000u + (unsigned)i);
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

        ultimate_bot::TrainConfig cfg;
        cfg.depth = depth;
        cfg.iterations = iters;
        cfg.games_per_iter = games;

        ultimate_bot::Weights w0;
        std::vector<int> hist;
        ultimate_bot::Weights w1 = ultimate_bot::train(w0, cfg, &hist);

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
