# Halogen Chess engine

**About**

Halogen is a powerful, world-class program capable of analysing and playing chess. It currently is ranked within the top 30 chess engines of all time.

-----------------------------------
**Details**

Written in C++, Halogen implements Null-move pruning, Late move reductions, Quiessence search and a Transposition table that uses Zobrist Hashing. The search routine is multithreaded using the SMP parallelisation technique.

Halogen development is currently supported on the [Openbench](http://chess.grantnet.us/) framework. OpenBench (created by [Andrew Grant](https://github.com/AndyGrant)) is an open-source Sequential Probability Ratio Testing (SPRT) framework designed for self-play testing of engines. OpenBench makes use of distributed computing, allowing anyone to contribute CPU time to further the development of some of the world's most powerful engines.

Since 2020, Halogen has used a neural network for its evaluation function. Halogen makes use of an incrementally updated architecture, inspired by the new NNUE networks in [Stockfish](https://github.com/official-stockfish/Stockfish). The neural networks were trained using a novel application of Temporaral Difference learning[^1], and then fine tuned using supervised learning on data generated through self-play games using the Bullet trainer[^2].

-----------------------------------
**How to use**

Halogen is not a stand alone application and should be used with any popular chess GUI that supports the [UCI](https://gist.github.com/DOBRO/2592c6dad754ba67e6dcaec8c90165bf) protocol. [Arena](http://www.playwitharena.de/) chess is a popular choice. You can find prebuilt binaries from the last major release in the releases tab, or build Halogen yourself. Halogen is officially supported on the following platforms:

| Platform          | Build |
|-------------------|-------|
| Ubuntu            |       |
| Windows (Mingw64) |       |
| Windows (Clang64) |       |

To build Halogen, simply use the included makefile in the `src` directory:

> $ make

[^1]: https://github.com/KierenP/Halogen/pull/517
[^2]: https://github.com/jw1912/bullet