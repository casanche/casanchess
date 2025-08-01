#ifndef MOVEGENERATOR_H
#define MOVEGENERATOR_H

#include "Constants.h"
#include "Move.h"

class Board;

class MoveGenerator {
public:
    // Generate all legal moves
    MoveList GenerateMoves(Board &board);

    // Generate check evasions when in check
    MoveList GenerateEvasionMoves(Board &board);
    // Generate tactical moves (captures and promotions)
    MoveList GenerateTacticalMoves(Board &board);

    // Pick a random move from the move list
    static Move RandomMove(const MoveList& moves);

private:
    void Init(const Board &board);

    void GeneratePseudoMoves(Board &board);

    void GeneratePawnMoves(Board &board);
    void GenerateKnightMoves(Board &board);
    void GenerateKingMoves(Board &board);
    void GenerateSlidingMoves(PIECE_TYPE pieceType, Board &board);

    void AddMoves(Board &board, PIECE_TYPE piece, int fromSq, Bitboard possibleMoves);
    void AddPromotionMoves(Board &board, int fromSq, Bitboard promotionMoves);
    void AddCastlingMoves(Board &board);

    Bitboard GenerateKingDangerAttacks(Board &board);
    Bitboard PinnedPieces(Board &board, COLOR color);
    Bitboard FillPinned(PIECE_TYPE slidingType, int square, int kingSquare);

    MoveList m_moves;

    COLOR m_color;
    COLOR m_enemyColor;
    bool m_generateQuiet;

    Bitboard m_ownPieces;
    Bitboard m_enemyPieces;
    Bitboard m_allPieces;

    Bitboard m_kingDangerSquares;

    Bitboard m_captureMask; //the piece giving check
    Bitboard m_pushMask; //squares that block a check

    Bitboard m_pinned;
    Bitboard m_pinnedCaptureMask[64];
    Bitboard m_pinnedPushMask[64];
};

#endif //MOVEGENERATOR_H
