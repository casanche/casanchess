#ifndef MOVEMAKER_H
#define MOVEMAKER_H

#include "Move.h"
#include <string>
class Board;

class MoveMaker {
public:
    //Standard
    void MakeMove(Board& board, Move move);
    void TakeMove(Board& board, Move move);
    //String
    void MakeMove(Board& board, std::string input);
    void TakeMove(Board& board);
    //Null
    void MakeNull(Board& board);
    void TakeNull(Board& board);

private:
    void AddPiece(Board& board, int square, COLORS color, PIECE_TYPE pieceType);
    void RemovePiece(Board& board, int square, COLORS color, PIECE_TYPE pieceType);
    void MovePiece(Board& board, int fromSq, int toSq, COLORS color, PIECE_TYPE pieceType);

    //Castling
    void UpdateCastlingRights(Board& board, const Move& move);
    void RewindCastlingRights(Board& board, const Move& move);

    void AddCastlingRights(Board& board, CASTLING_TYPE castlingType);
    void RemoveCastlingRights(Board& board, CASTLING_TYPE castlingType);
    void ToggleCastlingRights(Board& board, CASTLING_TYPE castlingType);

    //Helpers
    Move StringToMove(const Board& board, std::string input);
    Move DescriptiveToMove(const Board& board, SQUARES fromSq, SQUARES toSq, char promLet);

};

#endif //MOVEMAKER_H
