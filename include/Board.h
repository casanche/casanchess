#ifndef BOARD_H
#define BOARD_H

#include "Constants.h"
#include "ZobristKeys.h"

#include "Fen.h"
#include "MoveMaker.h"

#include <map>

const int MAX_PLIES = 3000;
const int SEE_MATERIAL_VALUES[8] = {0, 100, 350, 350, 500, 1050, 0}; //[PIECE]

const char PIECE_LETTERS[2][8] = { {' ', 'P', 'N', 'B', 'R', 'Q', 'K', '-',},
                                   {' ', 'p', 'n', 'b', 'r', 'q', 'k', '-'} }; //[COLOR][PIECE]

class Board {
    friend class Fen;
    friend class MoveMaker;
    friend class MoveGenerator;

    struct History {
        unsigned int fiftyrule;
        u8 castling;
        u64 zkey;
        u64 enpassant;
        Move move;
        
        History() { Clear(); }
        void Clear() {
            fiftyrule = 0;
            castling = 0;
            zkey = 0;
            enpassant = 0;
            move = Move();
        }
    };

public:
    Board();
    void Init();
    
    void Divide(int depth);
    void Mirror();
    u64 Perft(int depth);
    void Print(bool bits = false) const;
    void ShowHistory();
    void ShowMoves();

    // Fen
    void SetFen(std::string fenString) { m_fen.SetPosition(*this, fenString); }

    // MoveMaker
    void MakeMove(Move move) { m_moveMaker.MakeMove(*this, move); }
    void TakeMove(Move move) { m_moveMaker.TakeMove(*this, move); }
    //
    void MakeMove(std::string input) { m_moveMaker.MakeMove(*this, input); }
    void TakeMove() { m_moveMaker.TakeMove(*this); }
    //
    void MakeNull() { m_moveMaker.MakeNull(*this); }
    void TakeNull() { m_moveMaker.TakeNull(*this); }

    // Helper methods
    Bitboard AttackersTo(int square) const {
        return AttackersTo(ActivePlayer(), square);
    }
    Bitboard AttackersTo(COLOR color, int square) const;
    Bitboard AttackersTo(COLOR color, int square, Bitboard blockers) const;
    PIECE_TYPE GetPieceAtSquare(COLOR color, int square) const;
    bool IsAttacked(COLOR color, int square) const;
    bool IsCheck();
    bool IsRepetitionDraw(int searchPly = 0);
    int SquareToIndex(std::string square) const;
    Bitboard XRayAttackersTo(COLOR color, int square);

    // Debug
    bool CheckIntegrity() const;

    // Static Exchange Evaluation
    int SEE(Move move);

    //Getters
    inline COLOR ActivePlayer() const       { return m_activePlayer; }
    inline Bitboard AllPieces() const       { return m_allpieces; }
    inline u8 CastlingRights() const        { return m_castlingRights; }
    inline Bitboard Checkers() const        { return m_kingAttackers[m_activePlayer]; }
    inline Bitboard EnPassantSquare() const { return m_enPassantSquare; }
    inline unsigned int FiftyRule() const   { return m_fiftyrule; }
    inline Bitboard GetPieces(COLOR color, PIECE_TYPE pieceType) const { return m_pieces[color][pieceType]; }
    inline COLOR InactivePlayer() const    { return (COLOR)!m_activePlayer; }
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

    //State
    COLOR m_activePlayer;
    unsigned int m_moveNumber;
    unsigned int m_ply; //a ply is half a move
    unsigned int m_fiftyrule;
    u8 m_castlingRights; //[0-4] bits: the castling rights
    Bitboard m_enPassantSquare;
    ZobristKey m_zobristKey;
    ZobristKey m_pawnKey;

    //History
    History m_history[MAX_PLIES];

    //Pieces
    Bitboard m_pieces[2][8]; //[COLOR][PIECE_TYPE]
    Bitboard m_allpieces;

    //Helpers
    unsigned int m_initialPly;
    Bitboard m_kingAttackers[2]; //pieces that attack the king
    bool m_checkCalculated;

    //Static classes
    inline static Fen m_fen;
    inline static MoveMaker m_moveMaker;
};

#endif //BOARD_H
