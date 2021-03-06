#ifndef MOVE_H
#define MOVE_H

#include "Constants.h"
#include "BitboardUtils.h"
#include <vector>

enum MOVE_TYPE { NULLMOVE, NORMAL, CAPTURE, CASTLING, PROMOTION, DOUBLE_PUSH, PROMOTION_CAPTURE, ENPASSANT }; //CAPTURE + PROMOTION = PROMOTION_CAPTURE
enum PROMOTION_TYPE { PROMOTION_QUEEN, PROMOTION_KNIGHT, PROMOTION_ROOK, PROMOTION_BISHOP };
enum CASTLING_TYPE { CASTLING_K=1, CASTLING_Q=2, CASTLING_k=4, CASTLING_q=8 };

struct MoveData;

class Move {
public:
    Move();
    Move(u32 from, u32 to, PIECE_TYPE piece, MOVE_TYPE moveType);

    std::string Notation();
    void Print();

    inline u32            FromSq()        const { return                  RetrieveBits(m_move, 6, 0);  };
    inline u32            ToSq()          const { return                  RetrieveBits(m_move, 6, 6);  };
    inline PIECE_TYPE     PieceType()     const { return (PIECE_TYPE)     RetrieveBits(m_move, 3, 12); };
    inline MOVE_TYPE      MoveType()      const { return (MOVE_TYPE)      RetrieveBits(m_move, 3, 15); };
    inline PIECE_TYPE     CapturedType()  const { return (PIECE_TYPE)     RetrieveBits(m_move, 3, 18); };
    inline PROMOTION_TYPE PromotionType() const { return (PROMOTION_TYPE) RetrieveBits(m_move, 2, 21); };
    inline u8             Score()         const { return (u8)             RetrieveBits(m_move, 8, 23); };

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
    bool IsUnderpromotion() const;

    inline void SetCapturedType(PIECE_TYPE capturedType) {
        m_move &= ClearMask(3, 18); //Clears the bits 18-20
        m_move |= PushBits(capturedType, 3, 18);
    }
    inline void SetPromotionFlag(PROMOTION_TYPE flag) {
        m_move &= ClearMask(2, 21); //Clears the bits 21-22
        m_move |= PushBits(flag, 2, 21);
    }
    inline void SetScore(u8 score) {
        m_move &= ClearMask(8, 23); //Clears the bits 23-30
        m_move |= PushBits(score, 8, 23);
    }

    bool operator==(const Move& rmove) const {
        return (this->m_move & BitMask(23)) == (rmove.m_move & BitMask(23)); //Get only the 0-22 bits
    };
    bool operator!=(const Move& rmove) const {
        return !(*this == rmove);
    };

private:
    std::string IndexToNotation(int index);
    std::string PieceTypeToNotation(PIECE_TYPE pieceType);
    std::string DescriptiveNotation();

    void PrintBits32(u32 word, int startBit, int endBit) const;

    // Integer that stores all the move data
    //  0-5 bits: From Square (6)
    //  6-11 bits: To Square (6)
    //  12-14 bits: Piece Type (3)
    //  15-17 bits: Move Type (3)
    //  18-20 bits: Captured Type (3)
    //  21-22 bits: Promotion Flag (2): 0-QUEEN 1-KNIGHT 2-ROOK 3-BISHOP
    //  23-30 bits: Score bits (8): a value 0..255 used for move ordering
    u32 m_move;
};

struct MoveData {
    int fromSq;
    int toSq;
    PIECE_TYPE pieceType;
    MOVE_TYPE moveType;
    PIECE_TYPE capturedType;
};

typedef std::vector<Move> MoveList;

#endif //MOVE_H
