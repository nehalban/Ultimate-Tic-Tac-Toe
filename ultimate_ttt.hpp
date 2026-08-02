#pragma once

#include "tictactoe.hpp"

#include <string>
#include <utility>
#include <vector>

struct ult_ttt {
    ttt big_board;
    ttt small_boards[3][3];

    // Status of each small board:
    // '.' ongoing, 'X' won by X, 'O' won by O, 'D' draw/full no winner
    char small_status[3][3] = {
        {'.', '.', '.'},
        {'.', '.', '.'},
        {'.', '.', '.'},
    };

    struct Move {
        int br, bc; // big-row, big-col (0..2)
        int r, c;   // small-row, small-col (0..2)
    };

    struct ApplyResult {
        bool ok = false;
        bool game_over = false;
        char winner = '.'; // 'X' / 'O' / '.' (no winner yet)
        bool draw = false;
        // Next forced board: (-1,-1) means free choice
        int next_br = -1;
        int next_bc = -1;
    };

    static bool read_int(int& out) {
        if (std::cin >> out) return true;
        std::cin.clear();
        std::string junk;
        std::cin >> junk;
        return false;
    }

    bool board_done(int br, int bc) const { return small_status[br][bc] != '.'; }
    bool in_bounds_3(int x) const { return 0 <= x && x <= 2; }

    bool is_legal_move(const Move& m, int forced_br, int forced_bc) const { // forced sanity check unnecessary
        if (!in_bounds_3(m.br) || !in_bounds_3(m.bc) || !in_bounds_3(m.r) || !in_bounds_3(m.c)) return false;
        if (board_done(m.br, m.bc)) return false;
        if (forced_br != -1 && forced_bc != -1) {
            if (m.br != forced_br || m.bc != forced_bc) return false;
        }
        return small_boards[m.br][m.bc].cell[m.r][m.c] == '.';
    }

    void update_small_status(int br, int bc, int sr, int sc) {
        char w = small_boards[br][bc].winner_after_move(sr, sc);
        if (w == 'X' || w == 'O') {
            small_status[br][bc] = w;
            big_board.meta_fill(br, bc, w);
            return;
        }
        if (small_boards[br][bc].is_full()) {
            small_status[br][bc] = 'D';
            big_board.meta_fill(br, bc, 'D');
        }
    }

    // Forced board is either (forced_br,forced_bc) if playable, else (-1,-1)
    std::pair<int, int> normalize_forced(int forced_br, int forced_bc) const {
        if (forced_br == -1 || forced_bc == -1) return {-1, -1};
        if (!in_bounds_3(forced_br) || !in_bounds_3(forced_bc)) return {-1, -1};
        if (board_done(forced_br, forced_bc)) return {-1, -1};
        return {forced_br, forced_bc};
    }

    std::vector<Move> legal_moves(int forced_br = -1, int forced_bc = -1) const {
        std::vector<Move> moves;
        auto forced = normalize_forced(forced_br, forced_bc);
        forced_br = forced.first;
        forced_bc = forced.second;

        auto push_board = [&](int br, int bc) {
            if (board_done(br, bc)) return;
            for (int r = 0; r < 3; r++) {
                for (int c = 0; c < 3; c++) {
                    if (small_boards[br][bc].cell[r][c] == '.') {
                        moves.push_back(Move{br, bc, r, c});
                    }
                }
            }
        };

        if (forced_br != -1) {
            push_board(forced_br, forced_bc);
            return moves;
        }

        for (int br = 0; br < 3; br++) {
            for (int bc = 0; bc < 3; bc++) {
                push_board(br, bc);
            }
        }
        return moves;
    }

    ApplyResult apply_move(const Move& m, char player, int forced_br = -1, int forced_bc = -1) {
        ApplyResult res;
        if (player != 'X' && player != 'O') return res;
        if (!is_legal_move(m, forced_br, forced_bc)) return res;

        if (!small_boards[m.br][m.bc].move(m.r, m.c, player)) return res;

        update_small_status(m.br, m.bc, m.r, m.c);

        char big_w = '.';
        if (big_board.cell[m.br][m.bc] == 'X' || big_board.cell[m.br][m.bc] == 'O')
            big_w = big_board.winner_after_move(m.br, m.bc);
        if (big_w == 'X' || big_w == 'O') {
            res.ok = true;
            res.game_over = true;
            res.winner = big_w;
            return res;
        }
        if (big_board.is_full()) {
            res.ok = true;
            res.game_over = true;
            res.draw = true;
            return res;
        }

        // Forced next board is the small cell you just played into. If that board is
        // already finished the caller (normalize_forced) turns it into a free choice.
        res.next_br = m.r;
        res.next_bc = m.c;
        res.ok = true;
        return res;
    }

    void play() {
        std::cout << "Welcome to Ultimate Tic Tac Toe!\n";
        char curr_player = 'X';
        int forced_br = -1, forced_bc = -1;

        while (true) {
            print_board();

            auto forced = normalize_forced(forced_br, forced_bc);
            forced_br = forced.first;
            forced_bc = forced.second;

            Move m{-1, -1, -1, -1};
            std::cout << curr_player << "'s turn\n";

            if (forced_br != -1) {
                std::cout << "Forced big block: (" << forced_br << "," << forced_bc << ")\n";
                m.br = forced_br;
                m.bc = forced_bc;
            } else {
                std::cout << "Choose any big block (0-2, 0-2).\n";
                while (true) {
                    std::cout << "Enter big block row col: ";
                    if (!read_int(m.br) || !read_int(m.bc)) continue;
                    if (!in_bounds_3(m.br) || !in_bounds_3(m.bc)) {
                        std::cout << "Invalid big block. Row/col must be 0..2.\n";
                        continue;
                    }
                    if (board_done(m.br, m.bc)) {
                        std::cout << "That big block is complete (" << small_status[m.br][m.bc] << "). Pick another.\n";
                        continue;
                    }
                    break;
                }
            }

            std::cout << "Enter small cell row col (0-2, 0-2): ";
            if (!read_int(m.r) || !read_int(m.c)) continue;
            std::cout << '\n';

            ApplyResult res = apply_move(m, curr_player, forced_br, forced_bc);
            if (!res.ok) {
                std::cout << "Illegal move. Try again.\n";
                continue;
            }

            if (res.game_over) {
                print_board();
                if (res.winner == 'X' || res.winner == 'O') std::cout << res.winner << " wins the game!\n";
                else std::cout << "Game over: draw.\n";
                return;
            }

            forced_br = res.next_br;
            forced_bc = res.next_bc;
            curr_player = (curr_player == 'X') ? 'O' : 'X';
        }
    }

    void print_board() {
        for (int br = 0; br < 3; br++) {
            for (int r = 0; r < 3; r++) {
                for (int bc = 0; bc < 3; bc++) {
                    if (small_status[br][bc] == 'X') {
                        if (r == 0) std::cout << "\\   /";
                        if (r == 1) std::cout << "  X  ";
                        if (r == 2) std::cout << "/   \\";
                    } else if (small_status[br][bc] == 'O') {
                        if (r == 0) std::cout << "  _  ";
                        if (r == 1) std::cout << "|   |";
                        if (r == 2) std::cout << " \\_/ ";
                    } else if (small_status[br][bc] == 'D') {
                        if (r == 0) std::cout << "-----";
                        if (r == 1) std::cout << " DRAW";
                        if (r == 2) std::cout << "-----";
                    } else {
                        small_boards[br][bc].print_row(r);
                    }
                    if (bc < 2) std::cout << " || ";
                }

                if (r < 2) {
                    std::cout << '\n';
                    for (int bc = 0; bc < 3; bc++) {
                        if (small_status[br][bc] == 'X') {
                            if (r == 0) std::cout << " \\ / ";
                            if (r == 1) std::cout << " / \\ ";
                        } else if (small_status[br][bc] == 'O') {
                            if (r == 0) std::cout << " / \\ ";
                            if (r == 1) std::cout << "\\   /";
                        } else if (small_status[br][bc] == 'D') {
                            std::cout << "-----";
                        } else {
                            std::cout << "-+-+-";
                        }
                        if (bc < 2) std::cout << " || ";
                    }
                    std::cout << '\n';
                }
            }
            if (br < 2) std::cout << "\n      ||       ||\n======++=======++=======\n      ||       ||\n";
        }
        std::cout << "\n\n";
    }
};

