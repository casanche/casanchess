<div align="center">

<img src="https://github.com/user-attachments/assets/b3025cbb-8ed8-477e-83be-730cd8edfdd5" height="200" alt="Casanchess Logo">

**UCI chess engine written in C++23 with NNUE evaluation**

<p>
  <a href="https://github.com/casanche/casanchess/actions/workflows/tests.yml"><img src="https://github.com/casanche/casanchess/actions/workflows/tests.yml/badge.svg?branch=master" alt="Build and Test"></a>&nbsp;
  <a href="LICENSE"><img src="https://img.shields.io/badge/License-MIT-blue.svg" alt="License: MIT"></a>&nbsp;
  <a href="https://lichess.org/@/Casanchess-NNUE"><img src="https://img.shields.io/badge/Lichess-Play_vs_Casanchess_bot-d65108?logo=lichess&logoColor=white" alt="Play on Lichess"></a>
</p>

</div>

**Casanchess** aims to balance high playing strength with a clean and highly readable codebase.

Its NNUE evaluation is trained on millions of self-play and random positions, using a custom datagen and trainer.

The engine is designed to be both a strong opponent and an accessible resource for anyone interested in modern chess programming.

## Design philosophy
* ***Clean code:*** The primary goal is to increase strength, keeping the code elegant and easy to follow without sacrificing too much performance.
* ***Originality:*** Another principle is to remain original. Every PR has a clear reason and meaning. While the engine integrates standard and well-tested chess programming techniques, copying the implementation of other engines is actively avoided.
* ***Incremental progress:*** Improvements are made iteratively using strict SPRT. Development takes time since the hardware for testing is limited! But the focus remains on organic growth.

## Getting started
The easiest way is to download the latest compiled release binaries from the **[Releases](https://github.com/casanche/casanchess/releases)** page.
*Requires a CPU with AVX2 support.*

Alternatively, you could compile the source code yourself using a C++23 compliant compiler:
```bash
git clone https://github.com/casanche/casanchess.git
cd casanchess

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
```

## UCI options
* **``ClearHash``**: Reset the transposition table entries.
* **``Hash``**: Transposition table size in MB. Default: 16MB
* **``NNUE_Path``**: Absolute or relative path to the NNUE file.
* **``Ponder``**: Allows the engine to think on opponent's time.

For the NNUE evaluation to work, the `.nnue` file must be accessible.
By default, you should put the network file in the working directory (usually the same directory as the `casanchess` executable).
Alternatively, you can explicitly set its location using the `NNUE_Path` UCI option.

## Contributing
Contributions are more than welcome. Whether it's reporting bugs, suggesting new features, testing or improving the code, every contribution is appreciated.
Feel free to open an issue!

## Special thanks
The engine has evolved significantly since its early beginnings around 2018.
Projects like [Winglet](https://web.archive.org/web/20120621100214/http://www.sluijten.com/winglet/) and [ShallowBlue](https://github.com/GunshipPenguin/shallow-blue) introduced me to bitboards and the very first chess-programming concepts to get started, as well as [cerebrum](https://github.com/david-carteau/cerebrum) for its simple NNUE implementation.

Big thanks to the always amazing chess programming community. The knowledge openly shared on the [Talkchess.com](http://talkchess.com) forums and the [Chess Programming Wiki](https://www.chessprogramming.org) has been an essential resource throughout this journey.

Finally, to the many authors of chess engines, especially those who prioritize originality and independent development: keep up the great work!
