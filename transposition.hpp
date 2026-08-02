#pragma once

// Transposition table for the negamax search. Keyed by the bitstate's Zobrist
// hash (see bitboard.hpp). Entries are scoped to a single search via a generation
// counter: best_move() bumps the generation each call, so entries from a previous
// search (which may have used a different root perspective or weights) are ignored
// rather than reused. This keeps the table a pure speed optimization that does not
// change which move is chosen.
//
// Why same-search reuse is exact here: in Ultimate Tic-Tac-Toe a position's stone
// count is fixed, so any position recurs at the SAME remaining depth within one
// fixed-depth search. There is no cross-depth contamination, so a depth>=needed
// hit returns precisely the value a fresh search would compute.

#include <cstddef>
#include <cstdint>
#include <vector>

namespace tt {

enum Flag : std::uint8_t { NONE = 0, EXACT = 1, LOWER = 2, UPPER = 3 };

constexpr std::uint8_t NO_MOVE = 0xFF; // sentinel for "no stored best move"

struct Entry {
    std::uint64_t key = 0;       // full key, verified on probe (collision-safe)
    std::int32_t value = 0;
    std::uint16_t gen = 0;       // search generation this entry belongs to
    std::uint8_t depth = 0;      // remaining depth this value was searched to
    std::uint8_t flag = NONE;    // EXACT / LOWER / UPPER
    std::uint8_t mv = NO_MOVE;   // best move, encoded board*9 + cell
};

class Table {
public:
    explicit Table(int bits = 20) { resize(bits); }

    void resize(int bits) {
        entries_.assign(std::size_t(1) << bits, Entry{});
        mask_ = (std::size_t(1) << bits) - 1;
        gen_ = 0;
    }

    // Begin a new search: invalidate all previous entries cheaply by bumping the
    // generation. On the rare wrap, clear generations so stale slots can't alias.
    void new_search() {
        if (++gen_ == 0) {
            for (Entry& e : entries_) e.gen = 0;
            gen_ = 1;
        }
    }

    // On a usable cutoff, set out_val and return true. Always sets tt_move to the
    // stored best move for this key (NO_MOVE if absent) for move ordering.
    bool probe(std::uint64_t key, int depth, int alpha, int beta,
               int& out_val, std::uint8_t& tt_move) const {
        const Entry& e = entries_[key & mask_];
        tt_move = NO_MOVE;
        if (e.gen == gen_ && e.key == key) {
            tt_move = e.mv;
            if (e.depth >= depth) {
                if (e.flag == EXACT) { out_val = e.value; return true; }
                if (e.flag == LOWER && e.value >= beta) { out_val = e.value; return true; }
                if (e.flag == UPPER && e.value <= alpha) { out_val = e.value; return true; }
            }
        }
        return false;
    }

    void store(std::uint64_t key, int depth, int value, std::uint8_t flag, std::uint8_t mv) {
        Entry& e = entries_[key & mask_];
        // Replace stale entries, a different position, or a shallower-or-equal result.
        if (e.gen != gen_ || e.key != key || depth >= e.depth) {
            e.key = key;
            e.value = value;
            e.gen = gen_;
            e.depth = static_cast<std::uint8_t>(depth);
            e.flag = flag;
            e.mv = mv;
        }
    }

private:
    std::vector<Entry> entries_;
    std::size_t mask_ = 0;
    std::uint16_t gen_ = 0;
};

} // namespace tt
