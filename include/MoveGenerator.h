#ifndef _MOVEGENERATOR_H
#define _MOVEGENERATOR_H

#include "Constants.h"
#include "Move.h"

// extern int count; //test
#include <map>
extern std::map<std::string, int> countMap; //test

class Board;

class MoveGenerator {
    const int MAX_MOVES_RESERVE = 256;

public:
    MoveGenerator();

    MoveList GenerateMoves(Board &board);
    MoveList GeneratePseudoMoves(Board &board);
    MoveList GenerateEvasionMoves(Board &board);
    inline MoveList Moves() { return m_moves; };
    U64 Perft(int depth);
    Move RandomMove();

private:
    void GeneratePawnMoves(Board &board);
    void GenerateWhitePawnMoves(Board &board);
    void GenerateBlackPawnMoves(Board &board);
    void GenerateKnightMoves(Board &board);
    void GenerateKingMoves(Board &board);
    void GenerateSlidingMoves(PIECE_TYPE pieceType, Board &board);

    void AddMoves(Board &board, PIECE_TYPE piece, int fromSq, Bitboard possibleMoves, Bitboard enemyPieces);
    void AddPawnPushMoves(Board &board, int fromSq, Bitboard possibleMoves, MOVE_TYPE moveType);
    void AddPromotionMoves(Board &board, int fromSq, Bitboard promotionMoves);
    void AddCastlingMoves(Board &board);

    // Bitboard KingDangerSquares(Board &board);

    MoveList m_moves;
    MoveList m_legalMoves;
};

#endif //_MOVEGENERATOR_H
