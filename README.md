# Casanchess
A [UCI](http://wbec-ridderkerk.nl/html/UCIProtocol.html) compatible chess engine written in C++.

The NNUE evaluation is trained on millions of positions from auto-games and randomly generated FENs, evaluated with its own classical evaluation upto depth 7.

Play me on Lichess: [Casanchess-NNUE](https://lichess.org/@/Casanchess-NNUE)

## Build from source code
```sh
git clone https://github.com/casanche/casanchess.git
mkdir casanchess/build
cd casanchess/build
cmake ..
make -j
```

## To do
* Syzygy tablebases
* Multi-thread implementation

## Thanks
I would like to thank the whole chess programming community, the many open source engines and their authors who have this niche hobby.

Specially to:
- [Talkchess.com](http://talkchess.com) forum. For the countless hours spent on reading technical stuff.
- [Chess programming wiki](https://www.chessprogramming.org). Always an amazing resource to learn
- [ShallowBlue](https://github.com/GunshipPenguin/shallow-blue). Clean C++ engine that helped me get started
- [cerebrum](https://github.com/david-carteau/cerebrum). Nice and simple nnue implementation