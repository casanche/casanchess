#pragma once

#include "Constants.h"
class Board;

#include <map>
#include <string>

const bool DEBUG_PRINT_STATISTICS = true;

const bool DEBUG_SEARCH_TREE = false;

// Macros
#ifdef DEBUG
    #define D(x) x
#else
    #define D(x) ((void)0)
#endif

// Elements used in Board comparison
struct BoardIdentity {
    // Position
    bool activePlayer;
    u64 zkey;
    u64 pawnKey;
    u8 castlingRights;
    Bitboard enPassantSquare;

    // Piece Bitboards
    Bitboard allPieces;
    Bitboard pieces[2][8];
    
    // State
    unsigned int moveNumber;
    unsigned int ply;
    u8 fiftyRule;

    bool operator==(const BoardIdentity& rhs) const;
};

std::ostream& operator<<(std::ostream& os, const BoardIdentity& boardIdentity);

class BoardIntegrityChecker {
public:
    static bool CheckIntegrity(const Board& board);
    static BoardIdentity GenerateBoardIdentity(const Board& board);
};

// Object to collect and print search statistics
class SearchDebug {
public:
    void Clear() { debugVariables.clear(); };
    void Increment(const std::string& theVariable);
    void Print();
private:
    std::map<std::string, int> debugVariables;
};
