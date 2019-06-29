# Halogen Chess engine

A c++ chess engine that implements Null-move pruning, Late move reductions, Quiessence search and a transposition table that uses Zobrist Hashing.

The relative elo of various versions can be seen below. v2.5 is about as strong as MicroMax 3.2 for reference.

Name | Elo | games
--- | --- | ---
Halogen2-5 | 186 | 101
Halogen2-4 | 171 | 101
Halogen2-2 | -9 | 102
Halogen2-3 | -87 | 102
Halogen2-1 | -261 | 102

