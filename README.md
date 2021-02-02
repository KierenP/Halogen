# Halogen Chess engine

-----------------------------------
**About**

Halogen is a powerful, world-class program capable of analysing and playing chess. It currently is ranked within the top 30 chess engines of all time.

-----------------------------------
**Details**

Written in c++, Halogen implements Null-move pruning, Late move reductions, Quiessence search and a Transposition table that uses Zobrist Hashing. The search routine is multithreaded using the SMP parallelisation technique.

Halogen development is currently supported on the [Openbench](http://chess.grantnet.us/) framework. OpenBench (created by [Andrew Grant](https://github.com/AndyGrant)) is an open-source Sequential Probability Ratio Testing (SPRT) framework designed for self-play testing of engines. OpenBench makes use of distributed computing, allowing anyone to contribute CPU time to further the development of some of the world's most powerful engines.

Since Halogen 7, Halogen has used a neural network for its evaluation function. Halogen makes use of an incrementally updated architecture, inspired by the new NNUE networks in [Stockfish](https://github.com/official-stockfish/Stockfish). Halogen features an innovative neural network implementation with an abstract architecture that allows networks to be dropped in and replaced at will. Networks are trained through a private, from scratch C implementation created in collaboration with Andrew Grant.

-----------------------------------
 
**How to use**

Halogen is not a stand alone application and should be used with any popular chess gui that supports the [UCI](http://wbec-ridderkerk.nl/html/UCIProtocol.html) protocol. [Arena](http://www.playwitharena.de/) chess is a good choice.

-----------------------------------

Halogen is currently ranked 28th in the international CCRL 40/15 leaderboards [Link](https://ccrl.chessdom.com/ccrl/4040/cgi/compare_engines.cgi?class=Single-CPU+engines&print=Rating+list&print=Results+table&print=LOS+table&table_size=12&cross_tables_for_best_versions_only=1)
