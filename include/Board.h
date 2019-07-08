#ifndef _BOARD_H
#define _BOARD_H

#include "Constants.h"
#include "Fen.h"
// #include "MakeMove.h"
#include "Move.h"
#include "ZobristKeys.h"
#include <string>
#include <map>

const int MAX_PLIES = 3000;

const char PIECE_LETTERS[16] = { ' ', 'P', 'N', 'B', 'R', 'Q', 'K', '-',
                                 'p', 'n', 'b', 'r', 'q', 'k', '-', '-' };

class Board {
	//friend class MakeMove;
    friend class MoveGenerator;
    friend class Fen;

    struct History {
        unsigned int fiftyrule;
        U8 castling;
        U64 zkey;
        U64 enpassant;
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
    U64 Perft(int depth);
    void Print(bool bits = false) const;
    void SetFen(std::string fenString);
    void ShowHistory();
    void ShowMoves();

    // Make/Take move
    void MakeMove(Move move);
    void TakeMove(Move move);

    void MakeMove(std::string input);
    void TakeMove();

    //Null-move
    void MakeNull();
    void TakeNull();

    // Helper methods
    Bitboard AttackersTo(int square) {
        return AttackersTo(ActivePlayer(), square);
    }
    Bitboard AttackersTo(COLORS color, int square);
    PIECE_TYPE GetPieceAtSquare(COLORS color, int square) const;
    bool IsCheck();
    bool IsCheck(COLORS color);
    bool IsMoveLegal(Move move, bool isCheck);
    bool IsRepetitionDraw(int searchPly = 0);
    Bitboard XRayAttackersTo(COLORS color, int square);

    //Debug
    bool CheckIntegrity() const;

    // Static Exchange Evaluation
    int SEE(Move move);

    // Bitboard PinnedPieces(COLORS color);
    // Bitboard XRaydPieces(COLORS color);
    // void MyFunction(Bitboard attacker, Bitboard theKing);

    //Getters
    inline COLORS ActivePlayer() const      { return m_activePlayer; }
    inline Bitboard AllPieces() const       { return m_allpieces; }
    inline U8 CastlingRights() const        { return m_castlingRights; }
    inline Bitboard EnPassantSquare() const { return m_enPassantSquare; };
    inline unsigned int FiftyRule() const   { return m_fiftyrule; };
    inline Bitboard GetPieces(COLORS color, PIECE_TYPE pieceType) const { return m_pieces[color][pieceType]; };
    inline COLORS InactivePlayer() const    { return (COLORS)!m_activePlayer; }
    inline unsigned int MoveNumber() const  { return m_moveNumber; }
    inline Move LastMove() const            { return m_history[m_ply].move; }
    inline Bitboard Piece(COLORS color, PIECE_TYPE pieceType) const     { return m_pieces[color][pieceType]; };
    inline unsigned int Ply() const         { return m_ply; }
    inline U64 ZKey() const                 { return m_zobristKey.Key(); }

    //Operators
    bool operator==(const Board& rhs) const;

private:
    void AddPiece(int square, COLORS color, PIECE_TYPE pieceType);
    void RemovePiece(int square, COLORS color, PIECE_TYPE pieceType);
    void MovePiece(int fromSq, int toSq, COLORS color, PIECE_TYPE pieceType);

    //Make-Take helpers
    int SquareToIndex(std::string squareString);
    Move StringToMove(std::string input);
    Move DescriptiveToMove(SQUARES fromSq, SQUARES toSq, char promLet);

    void AddCastlingRights(CASTLING_TYPE castlingType) {
        if(~ CastlingRights() & castlingType) //if the bit is 0
            ToggleCastlingRights(castlingType);
    }
    void RemoveCastlingRights(CASTLING_TYPE castlingType) {
        if(CastlingRights() & castlingType) //if the bit is 1
            ToggleCastlingRights(castlingType);
    }
    void ToggleCastlingRights(CASTLING_TYPE castlingType) {
        m_castlingRights ^= castlingType;
        m_zobristKey.UpdateCastling(castlingType);
    }

    void ClearBits();
    void UpdateBitboards();
    void UpdateKingAttackers(COLORS color);

    void UpdateCastlingRights(const Move& move);
    void RewindCastlingRights(const Move& move);

    //Static Exchange Evaluation
    Bitboard LeastValuableAttacker(Bitboard attackers, COLORS color, PIECE_TYPE& pieceType);

    //State
    COLORS m_activePlayer;
    unsigned int m_moveNumber;
    unsigned int m_ply; //a ply is half a move
    unsigned int m_fiftyrule;
    ZobristKey m_zobristKey;
    U8 m_castlingRights; //[0-4] bits: the castling rights
    Bitboard m_enPassantSquare;

    //History
    History m_history[MAX_PLIES];

    //Pieces
    Bitboard m_pieces[2][8]; //[COLOR][PIECE_TYPE]
    Bitboard m_allpieces;

    //Helpers
    unsigned int m_initialPly;
    Bitboard m_attackedSquares[2]; //[COLOR]
    Bitboard m_pinnedPieces[2];
    Bitboard m_kingAttackers[2]; //pieces that attack the king
    Bitboard m_kingDangerSquares[2];

    //Helpers: check evasion
    Bitboard m_captureMask; //the piece giving check
    Bitboard m_pushMask; //squares that block a check

    //Static classes
    static Fen m_fen;
};

#endif //_BOARD_H