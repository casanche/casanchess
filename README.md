# Notes
## FEATURES
* IID, Futility pruning, razoring, delta pruning
* Treat checks in QS

## TODO
* Git framework
* Bool variable if we are null-moving -> use it to avoid LMR or pruning?

* UCI stop (handle analysis correctly for infinite analysis)
* UCI ponder

* Generate efficiently (without checking checks)
* Move Generation: Evasion moves (after check)
* Generate by steps. For example, only captures is useful
https://peterellisjones.com/posts/generating-legal-chess-moves-efficiently/

## FIXME
* Is there a problem with 3-fold repetition? Try to get some endpositions.

### Evaluation
* Passed pawns (free and unstoppable)
* King safety
* Pawn hash

## IDEAS
* Single-response extensions when the enemy king-safety is low, and there are lots of heavy pieces
* checks in first ply of quiescence
* Phase count: popcounting pieces + queen worth double
* TTEntry: make it 128-bits, make score I16

## SUITES
* STS Stat

# Testing
-pgnout /home/carlos/Tournaments/cutechesscli.pgn

cutechess-cli -engine name=CASANCHESS cmd=casanchess -engine name=Napoleon cmd=Napoleon -each proto=uci tc=0/10+0.1 book=/home/carlos/sw/arena/Books/gm2001.bin bookdepth=3 -repeat -concurrency 3 -tournament gauntlet -rounds 400 -games 2

##Sobremesa
cutechess-cli -engine name=HOTFIX cmd=casanchess-51 -engine name=StandPat cmd=casanchess-50 -each proto=uci tc=0/8+0.08 -openings file=/home/carlos/sw/arena/Books/book5.epd format=epd order=random -repeat -concurrency 3 -ratinginterval 25 -sprt alpha=0.05 beta=0.05 elo0=0 elo1=30 -rounds 1000
cutechess-cli -engine name=INFINITE_SCORE cmd=casanchess-49 -engine name=StandPat cmd=casanchess-47 -each proto=uci tc=0/6+0.06 -openings file=/home/carlos/sw/arena/Books/book5.epd format=epd order=random -repeat -concurrency 3 -ratinginterval 25 -sprt alpha=0.05 beta=0.05 elo0=-10 elo1=20 -rounds 200
##Laptop
cutechess-cli -engine name=FUTILITY_NON_PV cmd=casanchess-29 -engine name=StandPat cmd=casanchess-28 -each proto=uci tc=0/10+0.1 -openings file=/home/carlos/sw/arena/Books/book5.epd format=epd order=random -repeat -concurrency 5 -ratinginterval 25 -sprt alpha=0.05 beta=0.05 elo0=-10 elo1=25 -rounds 1000

##Queue
cutechess-cli -engine name=HOTFIX cmd=casanchess-51 -engine name=StandPat cmd=casanchess-50 -each proto=uci tc=0/7+0.07 -openings file=/home/carlos/sw/arena/Books/book5.epd format=epd order=random -repeat -concurrency 3 -ratinginterval 25 -sprt alpha=0.05 beta=0.05 elo0=-5 elo1=15 -rounds 1000
cutechess-cli -engine name=HOTFIX_2 cmd=casanchess-52 -engine name=StandPat cmd=casanchess-51 -each proto=uci tc=0/7+0.07 -openings file=/home/carlos/sw/arena/Books/book5.epd format=epd order=random -repeat -concurrency 3 -ratinginterval 25 -sprt alpha=0.05 beta=0.05 elo0=-5 elo1=15 -rounds 1000

# Two-fold repetition
* Topple
```c++
bool board_t::is_repetition_draw(int search_ply) const {
    int rep = 1;

    int max = std::max(record[now].halfmove_clock, search_ply);

    for (int i = 2; i <= max; i += 2) {
        if (record[now - i].hash == record[now].hash) rep++;
        if (rep >= 3) return true;
        if (rep >= 2 && i < search_ply) return true;
    }

    return false;
}
```

* Deepov
```c++
//check for 1-move repetition
std::vector<Zkey> keys = myBoard->getKeysHistory();
Zkey currentKey = myBoard->key;
keys.pop_back();

if (std::find(keys.begin(), keys.end(), currentKey) != keys.end())
{
    //Draw
    return Eval::DRAW_SCORE;
}
```

* Fruit
```cpp
bool board_is_repetition(const board_t * board) {
   int i;

   ASSERT(board!=NULL);

   // 50-move rule

   if (board->ply_nb >= 100) { // potential draw

      if (board->ply_nb > 100) return true;

      ASSERT(board->ply_nb==100);
      return !board_is_mate(board);
   }

   // position repetition

   ASSERT(board->sp>=board->ply_nb);

   for (i = 4; i <= board->ply_nb; i += 2) {
      if (board->stack[board->sp-i] == board->key) return true;
   }

   return false;
}
```

* CPW
```cpp
int isRepetition() {
	for (int i = 0; i < b.rep_index; i++) {
		if (b.rep_stack[i] == b.hash)
			return 1;
	}
	return 0;
}
```

* Arasan
```cpp
int Board::repCount(int target) const
{
    int entries = state.moveCount - 2;
    if (entries <= 0) return 0;
    hash_t to_match = hashCode();
    int count = 0;
    for (hash_t *repList=repListHead-3;
       entries>=0;
       repList-=2,entries-=2)
    {
      if (*repList == to_match)
      {
         count++;
         if (count >= target)
         {
            return count;
         }
      }
   }
   return count;
}
```

* Stockfish
```cpp
bool Position::is_draw(int ply) const {

  if (st->rule50 > 99 && (!checkers() || MoveList<LEGAL>(*this).size()))
      return true;

  int end = std::min(st->rule50, st->pliesFromNull);

  if (end < 4)
    return false;

  StateInfo* stp = st->previous->previous;
  int cnt = 0;

  for (int i = 4; i <= end; i += 2)
  {
      stp = stp->previous->previous;

      // At root position ply is 1, so return a draw score if a position
      // repeats once earlier but after or at the root, or repeats twice
      // strictly before the root.
      if (   stp->key == st->key
          && ++cnt + (ply - i > 0) == 2)
          return true;
  }

  return false;
}
```

* chess overflow
```cpp
int i;
for (i = 4; i <= board->fiifty_move; i += 2) {
    if (board->zkey == board->history[board->ply - i].zkey) {
        return true;
    }
}

```

# Helpers
## PROFILING
Add `-pg` to the compiler options\
Run the program\
`gprof <bin> gmon.out > analysis.txt`

`valgrind --tool=callgrind ./<bin>`\
`kcachegrind <out>`

## DEBUGGER
* Add -g to the compiler flags
* Run `gdb` on the executable file
* `break file:line` or `break function` #Adds a breakpoint
* `run`

## Bitboard
```
56 57 58 59 60 61 62 63
48 49 50 51 52 53 54 55
40 41 42 43 44 45 46 47
32 33 34 35 36 37 38 39
24 25 26 27 28 29 30 31
16 17 18 19 20 21 22 23
08 09 10 11 12 13 14 15
00 01 02 03 04 05 06 07
```

# Links
## Documentation
http://www.frayn.net/beowulf/theory.html Colin Frayn
## Tutorials and documents
http://pages.cs.wisc.edu/~psilord/blog/data/chess-pages/ (tutorial) (excellent)
http://web.archive.org/web/20120621100214/http://www.sluijten.com/winglet/ (tutorial)
http://www.tckerrigan.com/
http://www.cs.cmu.edu/afs/cs/academic/class/15418-s12/www/competition/www.contrib.andrew.cmu.edu/~jvirdo/rasmussen-2004.pdf (document)
https://stackoverflow.com/questions/494721/what-are-some-good-resources-for-writing-a-chess-engine (links)
## Perft
https://www.chessprogramming.net/perfect-perft/
## Source codes
https://github.com/GunshipPenguin/shallow-blue (source) (excellent)
https://github.com/RomainGoussault/Deepov
https://github.com/peterellisjones/rust_move_gen (source)
https://github.com/official-stockfish/Stockfish (source)
https://github.com/Mk-Chan/Teki (source)
## Misc
http://graphics.stanford.edu/~seander/bithacks.html (Bitwise operations)
http://cinnamonchess.altervista.org/bitboard_calculator/Calc.html (Bitboard calculator)
http://bernd.bplaced.net/fengenerator/fengenerator.html (FEN random generator)
## Links (archive)
https://www.youtube.com/watch?v=l-hh51ncgDI (video: minimax algorithm)

# DIRTY ANNOTATIONS
BB pseudo:
E(x) O O O O O O O
x O O O O O O O
x O O O O O O O
x O O O O O O O
x O O O O O O O
x O O O O O O O
x O O O O O O O
R allx

BB attacks:
O O O O O O O O
O O O O O O O O
O O O O O O O O
B(x) O O O O O O O
x O O O O O O O
x O O O O O O O
x O O O O O O O
R allx

Enemy piece B(x) is a blocker.
Enemy piece E(x) is said to be x-ray-attacked (xrayd)
BB xrayd = (pseudo AND enemy) AND ~attacks
if(xrayd.popcnt == 1) pinned
If E(x) is a KING, then a piece B(x) is pinned if:
    - pseudo ^ ~attacks ^ enemyKing (in the sight of the king)
    - pseudo ^ attacks ^ enemyPieces (THE PINNED PIECE, ONLY IF...)
    - pseudo ^ ~attacks ^ ~enemyKing & ~noPieces
