#ifndef MOVEGENERATOR_H
#define MOVEGENERATOR_H

#include "Constants.h"
#include "Move.h"

class Board;

class MoveGenerator {
    const int MAX_MOVES_RESERVE = 256;

public:
    MoveGenerator();

    MoveList GenerateMoves(Board &board);
    inline MoveList Moves() { return m_moves; };
    u64 Perft(int depth);
    Move RandomMove();

private:
    MoveList GeneratePseudoMoves(Board &board);
    MoveList GenerateEvasionMoves(Board &board);

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

    Bitboard GenerateAttacks(Board &board, COLOR color);
    Bitboard GenerateKingDangerAttacks(Board &board, COLOR color);
    Bitboard PinnedPieces(Board &board, COLOR color);
    Bitboard FillPinned(Board& board, COLOR color, PIECE_TYPE slidingType, int square, int kingSquare);

    MoveList m_moves;
};

#endif //MOVEGENERATOR_H
