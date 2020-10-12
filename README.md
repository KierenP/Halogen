# Halogen Chess engine

A c++ chess engine that implements Null-move pruning, Late move reductions, Quiessence search and a Transposition table that uses Zobrist Hashing. The search routine is multithreaded using the SMP parallelisation technique.

Halogen developement is currently supported on [Openbench](http://chess.grantnet.us/). OpenBench (created by [Andrew Grant](https://github.com/AndyGrant)) is an open-source sequential probability ratio testing framework designed for self-play testing by both UCI and xboard compliant engines. It uses distributed computing so that anyone can contribite CPU power and help support the development of some of the world's best Chess engines.

Since Halogen 7, Halogen has used a neural network for its evaluation function. The incrementally updated architecture is inspired by the new NNUE networks in [Stockfish](https://github.com/official-stockfish/Stockfish). Halogen features an innovative neural network implementation with an abstract architecture that allows networks to be dropped in and replaced at will. Networks are trained through a private pytorch implementation created through colarberation with Andrew Grant. 

-----------------------------------
 
**How to use**

Halogen is not a stand alone application and should be used with any popular chess gui that supports the [UCI](http://wbec-ridderkerk.nl/html/UCIProtocol.html) protocol. [Arena](http://www.playwitharena.de/) chess is a good choice.


-----------------------------------

Check it out on the CCRL Blitz rating list: [Link](https://ccrl.chessdom.com/ccrl/404/cgi/compare_engines.cgi?family=Halogen&print=Rating+list&print=Results+table&print=LOS+table&print=Ponder+hit+table&print=Eval+difference+table&print=Comopp+gamenum+table&print=Overlap+table&print=Score+with+common+opponents)

Halogen is currently ranked 150 in the international CCRL blitz leaderboards

