#include "Move.h"

#include <iostream>

const char RANKS_NOTATION[] = {'1', '2', '3', '4', '5', '6', '7', '8'};
const char FILES_NOTATION[] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'};
const char PIECES_NOTATION[] = {'\0', '\0', 'N', 'B', 'R', 'Q', 'K', '\0'};

Move::Move() : m_move(0) {}

Move::Move(u32 from, u32 to, PIECE_TYPE piece, MOVE_TYPE moveType) : m_move(0) {
    m_move =
            (from & 0x3f)
        | ( (to & 0x3f) << 6)
        | ( (piece & 0xf) << 12)
        | ( (moveType & 0xf) << 15);
}

std::string Move::Notation() {
    if( MoveType() == NULLMOVE ) return "0000";
    
    bool descriptive = 1;
    if(descriptive) {
        return DescriptiveNotation();
    }

    MoveData data = Data();

    //Symbolic notation
    std::string prefix;
    if( data.pieceType == PAWN && IsCapture() ) {
        prefix = FILES_NOTATION[ File(data.fromSq) ];
    } else if(data.moveType == CASTLING) {
        switch(data.toSq) {
            case G1:
            case G8: prefix = "O-O"; break;
            case C1: 
            case C8: prefix = "O-O-O"; break;
        };
        return "\033[1;33m" + prefix + "\033[0m";
    } else {
        prefix = PieceTypeToNotation(data.pieceType);
    }
    
    //Capture cross
    if( IsCapture() )
        prefix.append("x");

    //Promotion suffix
    std::string promotionSuffix = "";
    if( IsPromotion() ) {
        PROMOTION_TYPE promotionType = PromotionType();
        switch(promotionType) {
            case PROMOTION_QUEEN:  promotionSuffix = "=Q"; break;
            case PROMOTION_KNIGHT: promotionSuffix = "=N"; break;
            case PROMOTION_ROOK:   promotionSuffix = "=R"; break;
            case PROMOTION_BISHOP: promotionSuffix = "=B"; break;
            default: break;
        };
    }

    return prefix + IndexToNotation(data.toSq) + promotionSuffix;
}

std::string Move::DescriptiveNotation() {
    MoveData data = Data();

    //Promotion suffix
    std::string promotionSuffix = "";
    if( IsPromotion() ) {
        PROMOTION_TYPE promotionType = PromotionType();
        switch(promotionType) {
            case PROMOTION_QUEEN:  promotionSuffix = "q"; break;
            case PROMOTION_KNIGHT: promotionSuffix = "n"; break;
            case PROMOTION_ROOK:   promotionSuffix = "r"; break;
            case PROMOTION_BISHOP: promotionSuffix = "b"; break;
            default: assert(false);
        };
    }

    return IndexToNotation(data.fromSq) + IndexToNotation(data.toSq) + promotionSuffix;
}

std::string Move::IndexToNotation(int index) {
    char file = FILES_NOTATION[index % 8];
    char rank = RANKS_NOTATION[index / 8];
    return std::string({file, rank});
}
std::string Move::PieceTypeToNotation(PIECE_TYPE pieceType) {
    return std::string({PIECES_NOTATION[pieceType]});
}

void Move::Print() {
    std::cout << "Printing move: ";
    std::cout << "\033[1;33m" << Notation() << "\033[0m" << std::flush;
    std::cout << std::endl << "From Square (6): " << FromSq() << ", bits: ";
    PrintBits32(FromSq(), 0, 6);
    std::cout << std::endl << "To Square (6): " << ToSq() << ", bits: ";
    PrintBits32(ToSq(), 0, 6);
    std::cout << std::endl << "Piece Type (3): " << PieceType() << ", bits: ";
    PrintBits32(PieceType(), 0, 3);
    std::cout << " Captured Type (3): " << CapturedType() << ", bits: ";
    PrintBits32(CapturedType(), 0, 3);
    std::cout << " Move Type (3): " << MoveType() << ", bits: ";
    PrintBits32(MoveType(), 0, 3);
    std::cout << " Promotion Type (2): " << PromotionType() << ", bits: ";
    PrintBits32(PromotionType(), 0, 2);
    std::cout << std::endl;
}

void Move::PrintLine() {
    std::cout
        << Notation()
        << " " << " bla "
        << std::endl;
}

void Move::PrintBits32(u32 word, int startBit, int endBit) const {
    for(int i = startBit; i < endBit; ++i) {
        std::cout << GetBit(word, i) << "";
    }
}

MoveData Move::Data() const {
    MoveData data;
    data.fromSq = FromSq();
    data.toSq = ToSq();
    data.pieceType = PieceType();
    data.moveType = MoveType();
    data.capturedType = CapturedType();
    return data;
}
