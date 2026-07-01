# Ultimate Tic-Tac-Toe

A robust, terminal-based implementation of [Ultimate Tic-Tac-Toe](https://en.wikipedia.org/wiki/Ultimate_tic-tac-toe) written in modern C++. This project features a full game engine, an intuitive ASCII interface, and two AI opponents: a Negamax engine (alpha-beta pruning, transposition table, move ordering) with automated weight tuning, and a Monte Carlo Tree Search (MCTS) bot.

## Features

- **Standard Rules Engine:** Fully enforces the complex rules of Ultimate Tic-Tac-Toe, including forced board transitions, small board resolution (Wins/Draws), and big board victory conditions.
- **Two AI opponents:** Choose your opponent — a **Negamax** engine (alpha-beta pruning + transposition table + move ordering) or an **MCTS** bot (UCT). Each move reports the work done (e.g. `20000 paths searched in 0.05 seconds`).
- **Bitboard search:** Both engines search on a compact bitboard position for speed; the char-based board is used only for rules and rendering.
- **Heuristic Evaluation:** The Negamax bot evaluates lines, center/corner/edge preferences, and penalizes moves that send the opponent to a completed board (giving them a free move).
- **Automated AI Training:** A built-in hill-climbing self-play loop tunes the Negamax evaluation weights.
- **Beautiful CLI:** Dynamic ASCII rendering of the board that clearly distinguishes active boards, won boards (large `X` or `O`), and drawn boards.

## Game Modes

When you run the game, you will be prompted to select one of four modes:

1. **Play (Human vs Human):** Local 2-player mode.
2. **Bot (Human vs AI):** Play against the AI (You are 'X'). Choose **Negamax** (configure search depth, 2-4 recommended) or **MCTS** (configure iterations, e.g. 20000).
3. **Self (Negamax vs Negamax):** Watch the Negamax bot play itself to benchmark different depths and test win rates.
4. **Train:** Run a simple hill-climbing self-play loop to tune the Negamax heuristic weights (e.g., how much it values two-in-a-row, center control, etc.).

## Project Structure

* `tictactoe.hpp` - The foundational 3x3 board struct (`ttt`). Handles local cell states, win conditions, and bounds checking. Used for both the small local boards and the overarching "meta" board.
* `ultimate_ttt.hpp` - The core game logic (`ult_ttt`). Manages the 9 small boards, tracks the meta-board status, handles forced moves, validates legality, and contains the ASCII CLI renderer.
* `bitboard.hpp` - Compact bitboard position (`bb::BitState`) used by both AIs for fast search, plus Zobrist hashing.
* `transposition.hpp` - Transposition table (`tt::Table`) used by the Negamax search.
* `negamax_bot.hpp` - The Negamax engine (`negamax` namespace): evaluation, alpha-beta search with TT and move ordering, and the self-play training loop.
* `mcts_bot.hpp` - The Monte Carlo Tree Search bot (`mcts` namespace).
* `ultimate_ttt.cpp` - The entry point. Handles user input for the main menu and coordinates the selected game modes.

## Getting Started

### Prerequisites
You need a standard C++ compiler (like `g++`, `clang++`, or MSVC) that supports C++11 or higher.

### Building
To compile the game, run the following command in your terminal. It is highly recommended to compile with optimization flags (e.g., `-O3`) to ensure the AI runs fast at higher depths.
```bash
g++ -O3 ultimate_ttt.cpp -o ultimate_ttt
```

### Running
Execute the compiled binary:

```bash
# On Linux / macOS
./ultimate_ttt

# On Windows
ultimate_ttt.exe
```

## How to Play
**The Grid:** The game is played on a 3x3 grid of standard Tic-Tac-Toe boards.

**First Move:** The first player ('X') can place their mark anywhere on the grid.

**Forced Moves:** The cell you pick determines which small board your opponent must play in next. For example, if you play in the top-right square of a small board, your opponent must play their next move in the top-right small board.

**Free Moves:** If you are sent to a board that is already resolved (won or drawn), you get a "free move" and can play in any available small board.

**Winning:** Winning a small board claims that board for you on the big meta-board. Win the game by getting three won small boards in a row (horizontally, vertically, or diagonally).

## Controls
The game expects coordinates in the format of row col (0-indexed).

**Big Block Coordinates:** 0 0 is the top-left board, 2 2 is the bottom-right board.

**Small Cell Coordinates:** 0 0 is the top-left cell of the targeted small board.
