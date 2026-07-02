# Performance Analysis: Negamax & MCTS vs a Random Opponent

Benchmark of the two AI engines against a uniform-random legal-move player, to
gauge raw playing strength and how it scales with search effort. Running this
benchmark surfaced a depth-parity bug in the negamax evaluation, which has since
been fixed; both the fixed results and the original (buggy) results are shown.

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

## Results (current engine)

### Negamax vs Random

| Depth | Games | W | D | L | Win-rate |
|------:|------:|--:|--:|--:|---------:|
| 1     | 300   | 245 | 2 | 53 | **82.0%** |
| 2     | 300   | 295 | 3 | 2  | **98.8%** |
| 3     | 300   | 299 | 1 | 0  | **99.8%** |
| 4     | 200   | 198 | 2 | 0  | **99.5%** |
| 5     | 200   | 199 | 0 | 1  | **99.5%** |
| 6     | 100   | 100 | 0 | 0  | **100.0%** |

### MCTS vs Random

| Iterations | Games | W | D | L | Win-rate |
|-----------:|------:|--:|--:|--:|---------:|
| 100        | 300   | 294 | 5 | 1 | **98.8%** |
| 500        | 300   | 299 | 0 | 1 | **99.7%** |
| 1000       | 200   | 200 | 0 | 0 | **100.0%** |
| 5000       | 150   | 150 | 0 | 0 | **100.0%** |
| 20000      | 80    | 80  | 0 | 0 | **100.0%** |

## The depth-parity bug (found here, then fixed)

The first run of this benchmark produced a bizarre win-rate that **zig-zagged
with search depth**, with depth 1 actually *losing* to random:

| Depth | 1 | 2 | 3 | 4 | 5 | 6 |
|------:|--:|--:|--:|--:|--:|--:|
| buggy win-rate | 19.3% | 83.8% | 53.7% | 78.5% | 56.5% | 75.0% |
| fixed win-rate | 82.0% | 98.8% | 99.8% | 99.5% | 99.5% | 100.0% |

**Root cause.** `negamax::evaluate()` scored the position from a *fixed* root-mover
perspective while the negamax recursion negates the child value at every ply.
Correct negamax requires the leaf evaluation to be relative to the side to move at
that leaf. Because it was fixed:

- At **even** search depths the deepest ply lands on the root mover, the parity
  lines up, and the search behaved as correct minimax.
- At **odd** search depths the deepest ply is the opponent's, so the frontier was
  scored from the wrong side and the extra negation inverted it — the engine then
  *minimized its own evaluation* at the frontier. At depth 1 it deliberately picked
  its worst-looking move, hence sub-random results.

This was a quirk inherited from the original engine and preserved through the
bitboard rewrite. The rewrite had verified the search computes its value function
*consistently* (order-independent, matching an exhaustive no-pruning minimax of
that recurrence) — but the recurrence itself was strategically wrong at odd depths.
Strength benchmarking is what exposed it.

**The fix.** Evaluate every node from the side to move (`evaluate(st, st.to_move ==
'X', w)`) and treat any completed meta line as "the side to move has already lost"
(`return -meta_win`). Win-rate now rises monotonically with depth, odd/even parity
is gone, and the engine remains order-independent (verified: identical move output
with move ordering on and off). Note this changes the engine's move choices, so any
weights tuned against the old objective should be re-tuned.

## Findings

1. **Negamax now scales correctly** — win-rate climbs monotonically and is
   essentially perfect (≈100%) from depth 3 upward. Even depth 1 (a 1-ply
   look-ahead) now wins 82%.

2. **MCTS is near-perfect from a tiny budget** — ~99% at 100 iterations, a clean
   100% from 1000 on.
   > **Caveat:** MCTS's default playouts are uniform-random, so its rollouts model
   > a *random* opponent exactly — this is close to an ideal-case matchup and
   > overstates its edge against a strong opponent. (Head-to-head, ~20k-iteration
   > MCTS also beats depth-4 negamax.)

3. **Random is now a saturated yardstick** — both engines reach ~100% at modest
   settings, so this benchmark no longer discriminates between strong
   configurations. Measuring skill *beyond* random requires stronger baselines.

## Recommendations

1. **Re-tune the negamax weights** against the corrected search (the previous
   tuning targeted the flawed objective).
2. **Add a Negamax-vs-MCTS matrix and a fixed non-trivial baseline** (e.g. depth-2
   negamax) to the benchmark suite to measure strength above the random floor.
3. Consider a light/heuristic MCTS rollout policy and a time-based budget for a
   fairer, stronger MCTS.

## Raw output (current engine)

```
== Negamax vs Random ==
Negamax d1     games= 300  W= 245  D=  2  L= 53  winrate= 82.0%
Negamax d2     games= 300  W= 295  D=  3  L=  2  winrate= 98.8%
Negamax d3     games= 300  W= 299  D=  1  L=  0  winrate= 99.8%
Negamax d4     games= 200  W= 198  D=  2  L=  0  winrate= 99.5%
Negamax d5     games= 200  W= 199  D=  0  L=  1  winrate= 99.5%
Negamax d6     games= 100  W= 100  D=  0  L=  0  winrate=100.0%
== MCTS vs Random ==
MCTS 100       games= 300  W= 294  D=  5  L=  1  winrate= 98.8%
MCTS 500       games= 300  W= 299  D=  0  L=  1  winrate= 99.7%
MCTS 1000      games= 200  W= 200  D=  0  L=  0  winrate=100.0%
MCTS 5000      games= 150  W= 150  D=  0  L=  0  winrate=100.0%
MCTS 20000     games=  80  W=  80  D=  0  L=  0  winrate=100.0%
```
