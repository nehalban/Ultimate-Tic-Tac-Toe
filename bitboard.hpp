#pragma once

// Compact bitboard representation of an Ultimate Tic-Tac-Toe position, used by
// the search (negamax). Each 3x3 board is encoded as two 9-bit masks (one per
// player); a cell at (r,c) maps to bit r*3 + c. This makes per-node copies cheap
// (~44 bytes, trivially copyable) and turns win/full detection into table lookups
// and legal-move generation into a bit scan.
//
// This file is pure game mechanics: no heuristics live here. Scoring/search that
// depends on Weights stays in ultimate_bot.hpp.

#include "ultimate_ttt.hpp"

#include <array>
#include <cstdint>

namespace ultimate_bot {
namespace bb {

constexpr std::uint16_t FULL = 0x1FF; // all 9 cells of a 3x3 board

// Precomputed per-9-bit-mask tables: is this mask a tic-tac-toe win, and popcount.
struct Tables {
    bool win[512];
    unsigned char pc[512];
};

constexpr Tables make_tables() {
    Tables t = {};
    // 8 winning lines as bitmasks (octal): 3 rows, 3 cols, 2 diagonals.
    const int lines[8] = {0007, 0070, 0700, 0111, 0222, 0444, 0421, 0124};
    for (int m = 0; m < 512; m++) {
        bool wn = false;
        for (int i = 0; i < 8; i++) {
            if ((m & lines[i]) == lines[i]) { wn = true; break; }
        }
        t.win[m] = wn;
        int c = 0;
        for (int b = 0; b < 9; b++) if (m & (1 << b)) c++;
        t.pc[m] = static_cast<unsigned char>(c);
    }
    return t;
}

constexpr Tables TAB = make_tables();

inline bool is_win(std::uint16_t mask) { return TAB.win[mask & FULL]; }
inline int popcount9(std::uint16_t mask) { return TAB.pc[mask & FULL]; }

// The 8 winning lines, also handy for line-by-line heuristic scoring.
constexpr std::array<std::uint16_t, 8> LINES = {
    0007, 0070, 0700, 0111, 0222, 0444, 0421, 0124,
};

// ---- Zobrist hashing for the transposition table ----
// A position is fully described by per-board occupancy (sx/so), side to move,
// and the forced board. The meta status is a pure function of sx/so, so it is
// NOT hashed separately. Keys come from a fixed-seed splitmix64 stream, so they
// are identical across runs/builds.
struct Zobrist {
    std::uint64_t cell[9][9][2]; // [board][cell][X=0 / O=1]
    std::uint64_t side;          // XOR-ed in when it is O's turn
    std::uint64_t forced[10];    // forced board in -1..8 -> index 0..9
};

constexpr std::uint64_t sm_next(std::uint64_t& s) {
    s += 0x9E3779B97F4A7C15ull;
    std::uint64_t x = s;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
    return x ^ (x >> 31);
}

constexpr Zobrist make_zobrist() {
    Zobrist z = {};
    std::uint64_t s = 0x0123456789ABCDEFull;
    for (int b = 0; b < 9; b++)
        for (int c = 0; c < 9; c++)
            for (int p = 0; p < 2; p++)
                z.cell[b][c][p] = sm_next(s);
    z.side = sm_next(s);
    for (int i = 0; i < 10; i++) z.forced[i] = sm_next(s);
    return z;
}

constexpr Zobrist ZOB = make_zobrist();

struct BitState {
    std::uint16_t sx[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0}; // X marks per small board (board = br*3+bc)
    std::uint16_t so[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0}; // O marks per small board
    std::uint16_t meta_x = 0; // boards won by X (bit b)
    std::uint16_t meta_o = 0; // boards won by O
    std::uint16_t meta_d = 0; // boards drawn (full, no winner)
    std::int8_t forced = -1;  // raw forced board 0..8, or -1 for free choice
    char to_move = 'X';
    std::uint64_t key = 0;    // Zobrist hash, maintained incrementally by apply()

    std::uint16_t resolved() const { return meta_x | meta_o | meta_d; }
    bool board_resolved(int b) const { return (resolved() >> b) & 1; }
    bool all_resolved() const { return (resolved() & FULL) == FULL; }
};

// Convert the authoritative ult_ttt game state into a search bitstate.
// forced is kept raw (it may point at an already-resolved board); gather_moves()
// applies the same normalization ult_ttt::normalize_forced does.
inline BitState from_ult(const ult_ttt& g, char to_move, int forced_br, int forced_bc) {
    BitState bs;
    for (int br = 0; br < 3; br++) {
        for (int bc = 0; bc < 3; bc++) {
            int b = br * 3 + bc;
            std::uint16_t x = 0, o = 0;
            for (int r = 0; r < 3; r++) {
                for (int c = 0; c < 3; c++) {
                    char ch = g.small_boards[br][bc].cell[r][c];
                    int p = r * 3 + c;
                    if (ch == 'X') x |= (1u << p);
                    else if (ch == 'O') o |= (1u << p);
                }
            }
            bs.sx[b] = x;
            bs.so[b] = o;
            char st = g.small_status[br][bc];
            if (st == 'X') bs.meta_x |= (1u << b);
            else if (st == 'O') bs.meta_o |= (1u << b);
            else if (st == 'D') bs.meta_d |= (1u << b);
        }
    }
    bs.to_move = to_move;
    bs.forced = (forced_br < 0 || forced_bc < 0)
                    ? static_cast<std::int8_t>(-1)
                    : static_cast<std::int8_t>(forced_br * 3 + forced_bc);

    // Full Zobrist key (apply() maintains it incrementally thereafter).
    std::uint64_t k = 0;
    for (int b = 0; b < 9; b++) {
        std::uint16_t x = bs.sx[b];
        while (x) { int c = __builtin_ctz(x); k ^= ZOB.cell[b][c][0]; x &= x - 1; }
        std::uint16_t o = bs.so[b];
        while (o) { int c = __builtin_ctz(o); k ^= ZOB.cell[b][c][1]; o &= o - 1; }
    }
    if (bs.to_move == 'O') k ^= ZOB.side;
    k ^= ZOB.forced[bs.forced + 1];
    bs.key = k;
    return bs;
}

// A move is just (board, cell), each 0..8.
struct BMove {
    int board;
    int cell;
};

// Fill `out` with legal moves in row-major order (boards 0..8, cells 0..8),
// mirroring ult_ttt::legal_moves after normalize_forced. Returns the count.
inline int gather_moves(const BitState& st, BMove* out) {
    int n = 0;
    const std::uint16_t resolved = st.resolved();
    auto emit = [&](int b) {
        std::uint16_t empty = static_cast<std::uint16_t>(~(st.sx[b] | st.so[b]) & FULL);
        while (empty) {
            int cell = __builtin_ctz(empty);
            out[n++] = BMove{b, cell};
            empty &= empty - 1;
        }
    };
    const int f = st.forced;
    if (f >= 0 && !((resolved >> f) & 1)) {
        emit(f); // forced board still playable
    } else {
        for (int b = 0; b < 9; b++) {
            if (!((resolved >> b) & 1)) emit(b);
        }
    }
    return n;
}

// Apply a move for st.to_move at (board, cell). Mutates st: places the mark,
// resolves the small board if won/full, updates the meta board, sets the next
// forced board (NORMALIZED: -1 when the game is over, or when the target board
// is already resolved so the opponent gets a free choice), and flips side to move.
// Returns true if this move ends the game (meta win or all boards resolved).
//
// After a normal move st.forced >= 0. After a move that sends the opponent to an
// already-finished board, st.forced == -1 while game_over == false: the search
// reads exactly that condition to apply the send_to_finished penalty (which the
// original engine intended but never actually triggered).
inline bool apply(BitState& st, int board, int cell) {
    const bool x_to_move = (st.to_move == 'X');
    const std::uint16_t bit = static_cast<std::uint16_t>(1u << cell);
    const std::int8_t old_forced = st.forced;

    std::uint16_t& mine = x_to_move ? st.sx[board] : st.so[board];
    mine |= bit;
    st.key ^= ZOB.cell[board][cell][x_to_move ? 0 : 1]; // place the stone

    bool game_over = false;
    if (is_win(mine)) {
        if (x_to_move) st.meta_x |= (1u << board);
        else st.meta_o |= (1u << board);
        std::uint16_t meta_mine = x_to_move ? st.meta_x : st.meta_o;
        if (is_win(meta_mine)) game_over = true;
    } else if (((st.sx[board] | st.so[board]) & FULL) == FULL) {
        st.meta_d |= (1u << board);
    }
    if (!game_over && st.all_resolved()) game_over = true;

    if (game_over) st.forced = -1;
    else if (st.board_resolved(cell)) st.forced = -1; // sent to a finished board -> free choice
    else st.forced = static_cast<std::int8_t>(cell);

    st.key ^= ZOB.forced[old_forced + 1] ^ ZOB.forced[st.forced + 1]; // forced board change
    st.key ^= ZOB.side;                                               // side to move flips
    st.to_move = x_to_move ? 'O' : 'X';
    return game_over;
}

} // namespace bb
} // namespace ultimate_bot
