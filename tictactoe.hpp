// Simple 3x3 tic-tac-toe board used both for small boards and the meta board.
// Cell values:
// - '.' empty
// - 'X' or 'O' occupied / winner
// - 'D' draw (only used on the meta board, or as a status)

#include <iostream>

struct ttt {
    char cell[3][3] = {
        {'.', '.', '.'},
        {'.', '.', '.'},
        {'.', '.', '.'},
    };
    /// Placed symbols (each cell at most once for normal play).
    int moves = 0;

    inline bool in_bounds(int r, int c) const {
        return 0 <= r && r <= 2 && 0 <= c && c <= 2;
    }

    /// Meta board: fill when a small board resolves (X / O / D). Assumes cell was empty.
    void meta_fill(int row, int col, char mark) {
        cell[row][col] = mark;
        ++moves;
    }

    bool move(int row, int col, char player) {
        if (!in_bounds(row, col)) {
            std::cout << "Invalid move. Row/col must be 0..2.\n";
            return false;
        }
        if (cell[row][col] != '.') {
            std::cout << "Invalid move. Cell already filled.\n";
            return false;
        }
        cell[row][col] = player;
        ++moves;
        return true;
    }

    void print_row(int row) const {
        for (int j = 0; j < 3; j++) {
            std::cout << cell[row][j];
            if (j < 2) std::cout << "|";
        }
    }

    bool is_full() const { return moves >= 9; }

    /// Only lines through (row,col) can change on this move. 'D' cannot win a line.
    char winner_after_move(int row, int col) const {
        char p = cell[row][col];
        if (p != 'X' && p != 'O') return '.';
        if (cell[row][0] == p && cell[row][1] == p && cell[row][2] == p) return p;
        if (cell[0][col] == p && cell[1][col] == p && cell[2][col] == p) return p;
        if (row == col) {
            if (cell[0][0] == p && cell[1][1] == p && cell[2][2] == p) return p;
        }
        if (row + col == 2) {
            if (cell[0][2] == p && cell[1][1] == p && cell[2][0] == p) return p;
        }
        return '.';
    }
};
