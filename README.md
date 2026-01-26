# Casanchess
[![Build Status](https://github.com/casanche/casanchess/actions/workflows/tests.yml/badge.svg)](https://github.com/casanche/casanchess/actions/workflows/tests.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

Open-source [UCI](http://wbec-ridderkerk.nl/html/UCIProtocol.html)-compatible chess engine written from scratch in modern C++.

The project's ambition is to achieve high playing strength while maintaining a clean, well-structured, and understandable codebase.

The NNUE evaluation is trained on millions of chess positions, generated from self-play games and randomly created FENs. Each position was evaluated at depth 7 from its own evaluation function.

## Play it on Lichess!
Challenge the last version of the engine on:
<p align="center">
  <a href="https://lichess.org/@/Casanchess-NNUE">
    <strong>Play against Casanchess-NNUE</strong>
  </a>
</p>

## Getting started
The easiest way is to download the latest compiled release binaries from the **[Releases](https://github.com/casanche/casanchess/releases)** page.

Alternatively, you could compile the source code yourself using a C++23 compliant compiler:
```sh
git clone https://github.com/casanche/casanchess.git
cd casanchess

mkdir build && cd build
cmake ..
cmake --build . --parallel
```

### Configuration
For the NNUE evaluation to work, the `.nnue` file must be accessible:

* **Default**: Place the `.nnue` file in the working directory. Usually the same directory as the `casanchess` executable.
* **Custom path**: if the `.nnue` file is in a different location, set its path via the `NNUE_Path` UCI option.

The following UCI options are available:

* **``Hash``**: the transposition table size in MB. Default: 16MB
* **``Ponder``**: allows the engine to think on the opponent's time. Off by default.
* **``ClearHash``**: clears the transposition table.
* **``ClassicalEval``**: turn on to switch to the classical evaluation without NNUE. Off by default.
* **``NNUE_Path``**: Path to the NNUE file.

## Future roadmap
* Multi-thread implementation

## Special thanks
This project definitely would not be possible without the amazing chess programming community and the knowledge they openly share.
And to the many authors of chess engines, especially those with an inclination towards originality: keep up the work!

Contributions are welcome! Whether it's reporting bugs, suggesting new features, or improving the code, every contribution is appreciated.

Thanks to:
- [Talkchess.com](http://talkchess.com) forum. For the countless hours reading technical stuff.
- [Chess programming wiki](https://www.chessprogramming.org). Always an amazing resource of learning.
- [ShallowBlue](https://github.com/GunshipPenguin/shallow-blue). Clean C++ engine that provided initial inspiration.
- [cerebrum](https://github.com/david-carteau/cerebrum). Nice and simple NNUE implementation.