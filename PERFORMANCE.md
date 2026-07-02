# Performance Analysis: Negamax & MCTS vs a Random Opponent

Benchmark of the two AI engines against a uniform-random legal-move player, to
gauge raw playing strength and how it scales with search effort.

## Methodology

- **Harness:** [`bench_vs_random.cpp`](bench_vs_random.cpp). Each configuration plays
  a fixed number of complete games against the random bot, **alternating who moves
  first** (the engine plays X in half the games and O in the other half) to remove
  first-move bias.
- **Metric:** win-rate from the engine's perspective, `(wins + 0.5·draws) / games`.
- **Determinism:** every game uses a distinct seed; the random opponent is seeded
  per game, and each engine move is given a fresh sub-seed.
- **Build:** `g++ -O3 -std=c++17` (Apple clang, single thread).
- **Reproduce:** `g++ -O3 -std=c++17 bench_vs_random.cpp -o bench && ./bench`

## Results

### MCTS vs Random

| Iterations | Games | W | D | L | Win-rate |
|-----------:|------:|--:|--:|--:|---------:|
| 100        | 300   | 294 | 5 | 1 | **98.8%** |
| 500        | 300   | 299 | 0 | 1 | **99.7%** |
| 1000       | 200   | 200 | 0 | 0 | **100.0%** |
| 5000       | 150   | 150 | 0 | 0 | **100.0%** |
| 20000      | 80    | 80  | 0 | 0 | **100.0%** |

### Negamax vs Random

| Depth | Games | W | D | L | Win-rate |
|------:|------:|--:|--:|--:|---------:|
| 1     | 300   | 19  | 78  | 203 | **19.3%** |
| 2     | 300   | 213 | 77  | 10  | **83.8%** |
| 3     | 300   | 81  | 160 | 59  | **53.7%** |
| 4     | 200   | 122 | 70  | 8   | **78.5%** |
| 5     | 200   | 56  | 114 | 30  | **56.5%** |
| 6     | 100   | 53  | 44  | 3   | **75.0%** |

## Findings

### 1. MCTS is near-perfect against Random, even with tiny budgets

MCTS wins ~99% of games at just **100 iterations** and reaches a clean **100%**
from 1000 iterations onward. This is expected and even a little flattering:
MCTS's default playout policy is **uniform random**, so its rollouts model a
random opponent *exactly*. Against a random player, MCTS is effectively computing
the true win probability of each move. The result confirms the search, the UCT
back-propagation sign, and the win/loss accounting are all correct.

> **Caveat:** dominance over a random opponent is a weak indicator of strength
> against a *strong* opponent. Random rollouts are an ideal model of a random
> foe but a poor model of a skilled one, so these numbers overstate MCTS's edge
> in real play. (Head-to-head, ~20k-iteration MCTS also beats depth-4 negamax.)

### 2. Negamax has a depth-parity bug — odd depths play badly

The win-rate zig-zags with depth: even depths (2/4/6) are strong (75–84%), odd
depths (1/3/5) are weak, and **depth 1 loses to random outright (19.3%)**.

**Root cause.** `negamax::evaluate()` always scores the position from the *root
mover's* fixed perspective (`me_is_x`), while the negamax recursion negates the
child value at every ply. Correct negamax requires the leaf evaluation to be
relative to the side to move at that leaf (equivalently, flipped by ply parity).
Because it isn't:

- At **even** search depth the deepest ply lands on the root mover, the parity
  lines up, and the search is correct minimax.
- At **odd** search depth the deepest ply is the opponent's, so the leaf is
  scored from the wrong side and the extra negation inverts it. The engine then
  *minimizes its own evaluation* at the frontier — at depth 1 it deliberately
  plays its worst-looking move, hence sub-random results.

This is the "fixed-perspective" quirk carried over from the original engine and
preserved through the bitboard rewrite (the rewrite verified the search computes
its value function *consistently* — order-independently and matching exhaustive
minimax of that recurrence — but the recurrence itself is strategically wrong at
odd depths). The benchmark is what exposes the strategic cost.

### 3. Even "working" Negamax is far weaker than MCTS here

At its good (even) depths, negamax tops out around 75–84% with **many draws**
(e.g. depth 3: 160/300 drawn; depth 5: 114/200). Two reasons:

- A shallow, hand-weighted static evaluation is a weaker signal than MCTS's
  full-game rollouts, which directly measure "can I actually win from here?"
  against this opponent.
- Ultimate Tic-Tac-Toe is drawish under imperfect play, and shallow minimax
  neither sets up nor converts long-range threats, so many games drift to draws.

## Recommendations

1. **Fix the negamax evaluation parity.** Make the leaf evaluation relative to
   the side to move (flip its sign by ply parity, or store/score from the
   side-to-move's view). This would make strength increase monotonically with
   depth and eliminate the sub-random odd-depth behavior. Note it changes the
   engine's move choices and would invalidate any weights tuned against the
   current (flawed) objective, so weights should be re-tuned afterward.
2. **Until then, prefer even search depths** for the negamax opponent, or use
   MCTS, which is both correct and much stronger against weak opposition.
3. **Re-benchmark after the fix**, and add a Negamax-vs-MCTS matrix and a
   stronger fixed baseline (e.g. depth-2 negamax) to measure skill beyond the
   random yardstick.

## Raw output

```
== Negamax vs Random ==
Negamax d1     games= 300  W=  19  D= 78  L=203  winrate= 19.3%
Negamax d2     games= 300  W= 213  D= 77  L= 10  winrate= 83.8%
Negamax d3     games= 300  W=  81  D=160  L= 59  winrate= 53.7%
Negamax d4     games= 200  W= 122  D= 70  L=  8  winrate= 78.5%
Negamax d5     games= 200  W=  56  D=114  L= 30  winrate= 56.5%
Negamax d6     games= 100  W=  53  D= 44  L=  3  winrate= 75.0%
== MCTS vs Random ==
MCTS 100       games= 300  W= 294  D=  5  L=  1  winrate= 98.8%
MCTS 500       games= 300  W= 299  D=  0  L=  1  winrate= 99.7%
MCTS 1000      games= 200  W= 200  D=  0  L=  0  winrate=100.0%
MCTS 5000      games= 150  W= 150  D=  0  L=  0  winrate=100.0%
MCTS 20000     games=  80  W=  80  D=  0  L=  0  winrate=100.0%
```
