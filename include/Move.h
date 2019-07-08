#ifndef MOVE_H
#define MOVE_H

#include "Constants.h"
#include "BitboardUtils.h"
#include <vector>

//CAPTURE + PROMOTION = PROMOTION_CAPTURE
enum MOVE_TYPE { NULLMOVE, NORMAL, CAPTURE, CASTLING, PROMOTION, DOUBLE_PUSH, PROMOTION_CAPTURE, ENPASSANT };
enum PROMOTION_TYPE { PROMOTION_QUEEN, PROMOTION_KNIGHT, PROMOTION_ROOK, PROMOTION_BISHOP };
enum CASTLING_TYPE { CASTLING_K=1, CASTLING_Q=2, CASTLING_k=4, CASTLING_q=8 };

struct MoveData;

class Move {
public:
    Move();
    Move(U32 from, U32 to, PIECE_TYPE piece, MOVE_TYPE moveType);

    std::string Notation();
    void Print();
    void PrintLine();

    inline U32 FromSq() const { return m_move & 0x3f; }; //0x3f = CreateMask(0, 6)
    inline U32 ToSq() const { return (m_move & 0xfc0) >> 6; };
    inline PIECE_TYPE PieceType()         const { return (PIECE_TYPE)( (m_move & 0x7000) >> 12);       };
    inline MOVE_TYPE MoveType()           const { return (MOVE_TYPE)( (m_move & 0x38000) >> 15);       };
    inline PIECE_TYPE CapturedType()      const { return (PIECE_TYPE)( (m_move & 0x1c0000) >> 18);     };
    inline PROMOTION_TYPE PromotionType() const { return (PROMOTION_TYPE)( (m_move & 0x600000) >> 21); };
    inline U8 Score()                     const { return (U8)( (m_move & 0x7f800000) >> 23);           };

    MoveData Data() const;

    inline bool IsQuiet() const {
        return !IsCapture() && !IsPromotion();
    }
    inline bool IsCapture() const {
        MOVE_TYPE moveType = MoveType();
        return moveType == CAPTURE || moveType == PROMOTION_CAPTURE || moveType == ENPASSANT;
    }
    inline bool IsPromotion() const {
        MOVE_TYPE moveType = MoveType();
        return moveType == PROMOTION || moveType == PROMOTION_CAPTURE;
    }

    inline void SetCapturedType(PIECE_TYPE capturedType) {
        m_move &= ~0x1c0000; //Clears the bits 18-20
        m_move |= ( (capturedType & 0x7) << 18); //Take the first three bits, and move them 18 times
    }
    inline void SetPromotionFlag(PROMOTION_TYPE flag) {
        m_move &= ~0x600000; //Clears the bits 21-22
        m_move |= ( (flag & 0x3) << 21 );
    }
    inline void SetScore(U8 score) {
        m_move &= ~0x7f800000; //Clears the bits 23-30
        m_move |= ( (score & 0xff) << 23 );
    }

    bool operator==(const Move& rmove) const {
        return (this->m_move & 0x7fffff) == (rmove.m_move & 0x7fffff); //Get only the 0-22 bits
    };
    bool operator!=(const Move& rmove) const {
        return !(*this == rmove);
    };

private:
    std::string IndexToNotation(int index);
    std::string PieceTypeToNotation(PIECE_TYPE pieceType);
    std::string DescriptiveNotation();

    void PrintBits32(U32 word, int start, int length) const;

    //Returns the bits in the range [startBit, startBit + length]
    // inline U32 GetMoveBits(int startBit, int length) {
    //     return ( m_move & CreateMask(startBit, startBit + length) ) >> startBit;
    // };

    // Integer that stores all the move data
    //  0-5 bits: From Square (6)
    //  6-11 bits: To Square (6)
    //  12-14 bits: Piece Type (3)
    //  15-17 bits: Move Type (3)
    //  18-20 bits: Captured Type (3)
    //  21-22 bits: Promotion Flag (2): 0-QUEEN 1-KNIGHT 2-ROOK 3-BISHOP
    //  23-30 bits: Score bits (8): a value 0..255 used for move ordering
    U32 m_move;
};

struct MoveData {
    int fromSq;
    int toSq;
    MOVE_TYPE moveType;
    PIECE_TYPE pieceType;
    PIECE_TYPE capturedType;
};

typedef std::vector<Move> MoveList;

#endif //MOVE_H
