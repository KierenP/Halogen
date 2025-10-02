<div align="center">

<img
  width="200"
  alt="Halogen Logo"
  src="assets/halogen_lightbulbonly-1.png">

# Halogen

[![License](https://img.shields.io/github/license/KierenP/Halogen?style=for-the-badge)](https://github.com/KierenP/Halogen/blob/master/LICENSE)
[![GitHub release (latest by date)](https://img.shields.io/github/v/release/KierenP/Halogen?style=for-the-badge)](https://github.com/KierenP/Halogen/releases/latest)
[![Commits since latest release](https://img.shields.io/github/commits-since/KierenP/Halogen/latest?style=for-the-badge)](https://github.com/KierenP/Halogen/commits/master)
[![GitHub Downloads (specific asset, all releases)](https://img.shields.io/github/downloads/KierenP/Halogen/total?style=for-the-badge)](https://github.com/KierenP/Halogen/releases/latest)

</div>

Halogen is a powerful, world-class program capable of analysing and playing chess. It has a peak rating of 16th on the international Computer Chess Rating List, and is the #1 strongest chess engine in Oceania.

## How to download

The latest version of Halogen can always be downloaded from the [releases](https://github.com/KierenP/Halogen/releases/tag/latest) page

Halogen is not a stand alone application and should be used with any popular chess GUI that supports the [UCI](https://gist.github.com/DOBRO/2592c6dad754ba67e6dcaec8c90165bf) protocol. [Arena](http://www.playwitharena.de/) chess is a popular choice.

## How to build

To build Halogen yourself, simply use the included makefile in the `src` directory:

```bash
> cd src
> make
```

Halogen is officially supported on Windows and Ubuntu when using compilers gcc-13 and clang-16 or newer.

| Platform          | Build |
|-------------------|-------|
| Ubuntu            |  [![Ubuntu](https://github.com/KierenP/Halogen/actions/workflows/ubuntu.yml/badge.svg)](https://github.com/KierenP/Halogen/actions/workflows/ubuntu.yml)     |
| Windows           |  [![Windows](https://github.com/KierenP/Halogen/actions/workflows/windows.yml/badge.svg)](https://github.com/KierenP/Halogen/actions/workflows/windows.yml)     |

## Strength

| Version | [CCRL 40/15][ccrl-4015] | [CCRL Blitz][ccrl-blitz] | [CCRL 40/2 FRC][ccrl-402-frc] | [CEGT 40/20][cegt-4020] | [CEGT 40/4][cegt-404] | [MCERL] |
|:-------:|:-----------------------:|:------------------------:|:-----------------------------:|:---------------------:|:-----------------------:|:-------:|
|  15     |          3575           |           3695           |          3843                 |          3552           |         -             |    -    |
|  14     |          3553           |           3686           |          3838                 |          3531           |         -             |    -    |
|  13     |          3515           |           3632           |          -                    |          3471           |         -             |    -    |
|  12     |          3454           |           3534           |          3643                 |          3376           |         3409          |    3438 |
|  11     |          3377           |           3434           |             -                 |         3282            |          -            |    3422 |

## Details

Written in C++, Halogen implements Null-move pruning, Late move reductions, Quiessence search and a Transposition table that uses Zobrist Hashing. The search routine is multithreaded using the SMP parallelisation technique.

Halogen development is currently supported on the [Openbench](http://chess.grantnet.us/) framework. OpenBench (created by [Andrew Grant](https://github.com/AndyGrant)) is an open-source Sequential Probability Ratio Testing (SPRT) framework designed for self-play testing of engines. OpenBench makes use of distributed computing, allowing anyone to contribute CPU time to further the development of some of the world's most powerful engines.

Since 2020, Halogen has used a neural network for its evaluation function. Halogen makes use of an incrementally updated architecture, inspired by the new NNUE networks in [Stockfish](https://github.com/official-stockfish/Stockfish). The neural networks were trained using a novel application of Temporaral Difference learning[^1], and then fine tuned using supervised learning on data generated through self-play games using the Bullet trainer[^2].


[^1]: https://github.com/KierenP/Halogen/pull/517
[^2]: https://github.com/jw1912/bullet

[ccrl-4015]: https://www.computerchess.org.uk/ccrl/4040/cgi/compare_engines.cgi?class=Single-CPU+engines&only_best_in_class=on&num_best_in_class=1&print=Rating+list
[ccrl-blitz]: https://www.computerchess.org.uk/ccrl/404/cgi/compare_engines.cgi?class=Single-CPU+engines&only_best_in_class=on&num_best_in_class=1&print=Rating+list
[ccrl-402-frc]: https://www.computerchess.org.uk/ccrl/404FRC/cgi/compare_engines.cgi?class=Single-CPU+engines&only_best_in_class=on&num_best_in_class=1&print=Rating+list
[cegt-404]: http://www.cegt.net/40_4_Ratinglist/40_4_single/rangliste.html
[cegt-4020]: http://www.cegt.net/40_40%20Rating%20List/40_40%20SingleVersion/rangliste.html
[mcerl]: https://www.chessengeria.eu/mcerl