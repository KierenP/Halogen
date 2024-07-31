<div align="center">

# Halogen

[![License](https://img.shields.io/github/license/KierenP/Halogen?style=for-the-badge)](https://github.com/KierenP/Halogen/blob/main/LICENSE)
[![GitHub release (latest by date)](https://img.shields.io/github/v/release/KierenP/Halogen?style=for-the-badge)](https://github.com/KierenP/Halogen/releases/latest)
[![Commits since latest release](https://img.shields.io/github/commits-since/KierenP/Halogen/latest?style=for-the-badge)](https://github.com/KierenP/Halogen/commits/main)
![GitHub Downloads (specific asset, all releases)](https://img.shields.io/github/downloads/KierenP/Halogen/Halogen11-windows-popcnt.exe?style=for-the-badge)

</div>

Halogen is a powerful, world-class program capable of analysing and playing chess. It currently is ranked within the top 30 chess engines of all time.


## Details

Written in C++, Halogen implements Null-move pruning, Late move reductions, Quiessence search and a Transposition table that uses Zobrist Hashing. The search routine is multithreaded using the SMP parallelisation technique.

Halogen development is currently supported on the [Openbench](http://chess.grantnet.us/) framework. OpenBench (created by [Andrew Grant](https://github.com/AndyGrant)) is an open-source Sequential Probability Ratio Testing (SPRT) framework designed for self-play testing of engines. OpenBench makes use of distributed computing, allowing anyone to contribute CPU time to further the development of some of the world's most powerful engines.

Since 2020, Halogen has used a neural network for its evaluation function. Halogen makes use of an incrementally updated architecture, inspired by the new NNUE networks in [Stockfish](https://github.com/official-stockfish/Stockfish). The neural networks were trained using a novel application of Temporaral Difference learning[^1], and then fine tuned using supervised learning on data generated through self-play games using the Bullet trainer[^2].


## Strength

| Version | [CCRL 40/15][ccrl-4015] | [CCRL Blitz][ccrl-blitz] | [CCRL 40/2 FRC][ccrl-402-frc] | [CEGT 40/20][cegt-4020] | [CEGT 40/4][cegt-404] | [MCERL] |
|:-------:|:-----------------------:|:------------------------:|:-----------------------------:|:---------------------:|:-----------------------:|:-------:|
|  11.4.0 |            -            |           -           |             3437              |           -           |          -           |    -    |
|  11.0.0 |          3380           |           3433           |             -              |         3288             |          3313           |    3398    |
|  10.0.0 |          3196           |           3220           |             -              |         3084          |            -         |  -   |
|  9.0.0  |          3153           |           3178           |             -              |           3013           |          -           |  -   |
|  8.1.0  |          -           |           3015           |             -              |         2866          |             -           |  -   |


## How to use

Halogen is not a stand alone application and should be used with any popular chess GUI that supports the [UCI](https://gist.github.com/DOBRO/2592c6dad754ba67e6dcaec8c90165bf) protocol. [Arena](http://www.playwitharena.de/) chess is a popular choice. You can find prebuilt binaries from the last major release in the releases tab, or build Halogen yourself. Halogen is officially supported on the following platforms:

| Platform          | Build |
|-------------------|-------|
| Ubuntu            |  [![Ubuntu](https://github.com/KierenP/Halogen/actions/workflows/ubuntu.yml/badge.svg)](https://github.com/KierenP/Halogen/actions/workflows/ubuntu.yml)     |
| Windows (Mingw64) |  [![Windows (MinGW64)](https://github.com/KierenP/Halogen/actions/workflows/windows-mingw64.yml/badge.svg)](https://github.com/KierenP/Halogen/actions/workflows/windows-mingw64.yml)     |
| Windows (Clang64) |  [![Windows (Clang64)](https://github.com/KierenP/Halogen/actions/workflows/windows-clang64.yml/badge.svg)](https://github.com/KierenP/Halogen/actions/workflows/windows-clang64.yml)     |

To build Halogen, simply use the included makefile in the `src` directory:

```bash
> make
```

[^1]: https://github.com/KierenP/Halogen/pull/517
[^2]: https://github.com/jw1912/bullet

[ccrl-4015]: https://www.computerchess.org.uk/ccrl/4040/cgi/compare_engines.cgi?class=Single-CPU+engines&only_best_in_class=on&num_best_in_class=1&print=Rating+list
[ccrl-blitz]: https://www.computerchess.org.uk/ccrl/404/cgi/compare_engines.cgi?class=Single-CPU+engines&only_best_in_class=on&num_best_in_class=1&print=Rating+list
[ccrl-402-frc]: https://www.computerchess.org.uk/ccrl/404FRC/cgi/compare_engines.cgi?class=Single-CPU+engines&only_best_in_class=on&num_best_in_class=1&print=Rating+list
[cegt-404]: http://www.cegt.net/40_4_Ratinglist/40_4_single/rangliste.html
[cegt-4020]: http://www.cegt.net/40_40%20Rating%20List/40_40%20All%20Versions/rangliste.html
[mcerl]: https://www.chessengeria.eu/mcerl