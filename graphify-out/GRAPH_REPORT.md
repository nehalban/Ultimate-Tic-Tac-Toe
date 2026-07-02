# Graph Report - .  (2026-07-02)

## Corpus Check
- Corpus is ~9,427 words - fits in a single context window. You may not need a graph.

## Summary
- 193 nodes · 315 edges · 10 communities (9 shown, 1 thin omitted)
- Extraction: 98% EXTRACTED · 2% INFERRED · 0% AMBIGUOUS · INFERRED: 5 edges (avg confidence: 0.85)
- Token cost: 0 input · 0 output

## Community Hubs (Navigation)
- [[_COMMUNITY_Negamax Engine & Search|Negamax Engine & Search]]
- [[_COMMUNITY_Bitboard Position & Ops|Bitboard Position & Ops]]
- [[_COMMUNITY_MCTS Bot|MCTS Bot]]
- [[_COMMUNITY_Transposition Table|Transposition Table]]
- [[_COMMUNITY_Game Rules & State (ult_ttt)|Game Rules & State (ult_ttt)]]
- [[_COMMUNITY_Vs-Random Benchmark|Vs-Random Benchmark]]
- [[_COMMUNITY_3x3 Board Primitive (ttt)|3x3 Board Primitive (ttt)]]
- [[_COMMUNITY_Training Config|Training Config]]
- [[_COMMUNITY_Search State Wrapper|Search State Wrapper]]
- [[_COMMUNITY_Coding Guidelines|Coding Guidelines]]

## God Nodes (most connected - your core abstractions)
1. `Weights` - 20 edges
2. `BitState` - 19 edges
3. `ult_ttt` - 18 edges
4. `Table` - 16 edges
5. `Node` - 14 edges
6. `negamax()` - 14 edges
7. `best_move()` - 13 edges
8. `Entry` - 12 edges
9. `best_move()` - 11 edges
10. `order_moves()` - 10 edges

## Surprising Connections (you probably didn't know these)
- `gather_moves()` --semantically_similar_to--> `ult_ttt`  [INFERRED] [semantically similar]
  bitboard.hpp → ultimate_ttt.hpp
- `Forced/free moves rule` --references--> `apply()`  [INFERRED]
  README.md → bitboard.hpp
- `best_move()` --semantically_similar_to--> `best_move()`  [INFERRED] [semantically similar]
  mcts_bot.hpp → random_bot.hpp
- `best_move()` --semantically_similar_to--> `best_move()`  [INFERRED] [semantically similar]
  negamax_bot.hpp → mcts_bot.hpp
- `Ultimate Tic-Tac-Toe project` --references--> `main()`  [INFERRED]
  README.md → ultimate_ttt.cpp

## Import Cycles
- None detected.

## Hyperedges (group relationships)
- **Three AI opponents share the best_move interface** — negamax_bot_best_move, mcts_bot_best_move, random_bot_best_move [INFERRED 0.85]
- **Negamax search pipeline from root conversion to evaluation** — bitboard_from_ult, bitboard_gather_moves, negamax_bot_evaluate [INFERRED 0.85]
- **Bitboard move primitives used by both engines** — bitboard_bitstate, bitboard_gather_moves, bitboard_apply [INFERRED 0.85]

## Communities (10 total, 1 thin omitted)

### Community 0 - "Negamax Engine & Search"
Cohesion: 0.09
Nodes (38): BMove, apply(), best_move(), evaluate(), ApplyResult, BitState, Move, mt19937 (+30 more)

### Community 1 - "Bitboard Position & Ops"
Cohesion: 0.08
Nodes (33): apply(), BitState, forced, key, meta_d, meta_o, meta_x, so (+25 more)

### Community 2 - "MCTS Bot"
Cohesion: 0.10
Nodes (25): best_move(), Config, c, iterations, seed, BitState, Move, mt19937 (+17 more)

### Community 3 - "Transposition Table"
Cohesion: 0.12
Nodes (17): int32_t, size_t, Entry, depth, flag, gen, key, mv (+9 more)

### Community 4 - "Game Rules & State (ult_ttt)"
Cohesion: 0.18
Nodes (9): pair, ttt, ApplyResult, Move, vector, ult_ttt, big_board, small_boards (+1 more)

### Community 5 - "Vs-Random Benchmark"
Cohesion: 0.14
Nodes (17): Bot, k, param, Move, mt19937, uint32_t, ult_ttt, eval() (+9 more)

### Community 6 - "3x3 Board Primitive (ttt)"
Cohesion: 0.25
Nodes (3): ttt, cell, moves

### Community 7 - "Training Config"
Cohesion: 0.29
Nodes (7): uint32_t, TrainConfig, depth, games_per_iter, iterations, seed, weight_step

### Community 8 - "Search State Wrapper"
Cohesion: 0.33
Nodes (6): ult_ttt, State, forced_bc, forced_br, g, to_move

## Knowledge Gaps
- **96 isolated node(s):** `Kind`, `k`, `param`, `Move`, `ult_ttt` (+91 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **1 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `ult_ttt` connect `Game Rules & State (ult_ttt)` to `Bitboard Position & Ops`, `3x3 Board Primitive (ttt)`?**
  _High betweenness centrality (0.272) - this node is a cross-community bridge._
- **Why does `negamax()` connect `Negamax Engine & Search` to `Bitboard Position & Ops`, `Transposition Table`?**
  _High betweenness centrality (0.214) - this node is a cross-community bridge._
- **Why does `gather_moves()` connect `Bitboard Position & Ops` to `Negamax Engine & Search`, `MCTS Bot`, `Game Rules & State (ult_ttt)`?**
  _High betweenness centrality (0.169) - this node is a cross-community bridge._
- **What connects `Kind`, `k`, `param` to the rest of the system?**
  _96 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Negamax Engine & Search` be split into smaller, more focused modules?**
  _Cohesion score 0.08943089430894309 - nodes in this community are weakly interconnected._
- **Should `Bitboard Position & Ops` be split into smaller, more focused modules?**
  _Cohesion score 0.08108108108108109 - nodes in this community are weakly interconnected._
- **Should `MCTS Bot` be split into smaller, more focused modules?**
  _Cohesion score 0.10461538461538461 - nodes in this community are weakly interconnected._