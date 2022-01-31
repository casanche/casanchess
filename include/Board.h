#ifndef BOARD_H
#define BOARD_H

#include "Constants.h"
#include "ZobristKeys.h"

#include "Fen.h"
#include "MoveMaker.h"

const int MAX_PLIES = 2560;

struct BoardHistory {
    u64 zkey;
    u64 enpassant;
    Move move;
    u8 castling;
    u8 fiftyrule;
    
    BoardHistory() { Clear(); }
    void Clear() {
        zkey = 0;
        enpassant = 0;
        move = Move();
        castling = 0;
        fiftyrule = 0;
    }
};

class Board {
public:
    Board();
    void Init();
    
    u64 Perft(int depth);
    void Divide(int depth);
    
    //Print, Debug
    void Print(bool bits = false) const;
    void ShowHashMoves();
    void ShowHistory();
    void ShowMoves();

    // Fen
    void SetFen(std::string fenString) { Fen::SetPosition(*this, fenString); }
    std::string GetSimplifiedFen() { return Fen::GetSimplifiedFen(*this); };

    // MoveMaker
    void MakeMove(Move move) { MoveMaker::MakeMove(*this, move); }
    void TakeMove(Move move) { MoveMaker::TakeMove(*this, move); }
    //
    void MakeMove(std::string input) { MoveMaker::MakeMove(*this, input); }
    void TakeMove() { MoveMaker::TakeMove(*this); }
    //
    void MakeNull() { MoveMaker::MakeNull(*this); }
    void TakeNull() { MoveMaker::TakeNull(*this); }

    // Static Exchange Evaluation
    int SEE(Move move);

    // Helper methods
    Bitboard AttackersTo(COLOR color, int square, Bitboard blockers) const;
    Bitboard AttackersTo(COLOR color, int square) const;
    Bitboard AttackersTo(int square) const { return AttackersTo(ActivePlayer(), square); }
    PIECE_TYPE GetPieceAtSquare(COLOR color, int square) const;
    bool IsAttacked(COLOR color, int square) const;
    bool IsCheck();
    bool IsRepetitionDraw(int searchPly = 0);
    void Mirror();
    int SquareToIndex(std::string square) const;
    Bitboard XRayAttackersTo(COLOR color, int square);

    //Getters
    inline COLOR ActivePlayer() const       { return m_activePlayer; }
    inline Bitboard AllPieces() const       { return m_allpieces; }
    inline u8 CastlingRights() const        { return m_castlingRights; }
    inline Bitboard Checkers() const        { return m_kingAttackers[m_activePlayer]; }
    inline Bitboard EnPassantSquare() const { return m_enPassantSquare; }
    inline u8 FiftyRule() const             { return m_fiftyrule; }
    inline Bitboard GetPieces(COLOR color, PIECE_TYPE pieceType) const { return m_pieces[color][pieceType]; }
    inline COLOR InactivePlayer() const     { return (COLOR)!m_activePlayer; }
    inline unsigned int MoveNumber() const  { return m_moveNumber; }
    inline Move LastMove() const            { return m_history[m_ply].move; }
    inline u64 PawnKey() const              { return m_pawnKey.Key(); }
    inline Bitboard Piece(COLOR color, PIECE_TYPE pieceType) const     { return m_pieces[color][pieceType]; }
    inline unsigned int Ply() const         { return m_ply; }
    inline u64 ZKey() const                 { return m_zobristKey.Key(); }

    //Operators
    bool operator==(const Board& rhs) const;

private:
    void ClearBits();
    void UpdateBitboards();
    void UpdateKingAttackers(COLOR color);
    void InitStateAndHistory();

    //Static Exchange Evaluation
    Bitboard LeastValuableAttacker(Bitboard attackers, COLOR color, PIECE_TYPE& pieceType);

    // Debug
    bool CheckIntegrity() const;

    //State
    COLOR m_activePlayer;
    u8 m_castlingRights; //[0-4] bits: the castling rights
    u8 m_fiftyrule;
    unsigned int m_moveNumber;
    unsigned int m_ply; //a ply is half a move
    Bitboard m_enPassantSquare;
    ZobristKey m_zobristKey;
    ZobristKey m_pawnKey;

    //Pieces
    Bitboard m_pieces[2][8]; //[COLOR][PIECE_TYPE]
    Bitboard m_allpieces;

    //Helpers
    Bitboard m_kingAttackers[2]; //pieces that attack the king
    bool m_checkCalculated;

    //History
    unsigned int m_initialPly;
    BoardHistory m_history[MAX_PLIES];

    friend class Fen;
    friend class MoveMaker;
    friend class MoveGenerator;
};

#endif //BOARD_H
