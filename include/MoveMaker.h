#ifndef MOVEMAKER_H
#define MOVEMAKER_H

#include "Move.h"
#include <string>
class Board;

class MoveMaker {
public:
    //Standard
    static void MakeMove(Board& board, Move move);
    static void TakeMove(Board& board, Move move);
    //String
    static void MakeMove(Board& board, std::string input);
    static void TakeMove(Board& board);
    //Null
    static void MakeNull(Board& board);
    static void TakeNull(Board& board);

private:
    static void AddPiece(Board& board, int square, COLOR color, PIECE_TYPE pieceType);
    static void RemovePiece(Board& board, int square, COLOR color, PIECE_TYPE pieceType);
    static void MovePiece(Board& board, int fromSq, int toSq, COLOR color, PIECE_TYPE pieceType);

    //Castling
    static void UpdateCastlingRights(Board& board, const Move& move);
    static void RewindCastlingRights(Board& board, const Move& move);

    static void AddCastlingRights(Board& board, CASTLING_TYPE castlingType);
    static void RemoveCastlingRights(Board& board, CASTLING_TYPE castlingType);
    static void ToggleCastlingRights(Board& board, CASTLING_TYPE castlingType);

    //Helpers
    static Move StringToMove(const Board& board, std::string input);
    static Move DescriptiveToMove(const Board& board, SQUARES fromSq, SQUARES toSq, char promLet);
};

#endif //MOVEMAKER_H
