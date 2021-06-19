#include "ZobristKeys.h"

#include "BitboardUtils.h"
#include "Board.h"
#include "Utils.h"

u64 ZobristKeys::m_zkeyColor;
u64 ZobristKeys::m_zkeyPieces[2][8][64]; //[COLOR][PIECE_TYPE][SQUARE]
u64 ZobristKeys::m_zkeyCastling[2][2]; //[COLOR][CASTLING_TYPE_SIMPLE]
u64 ZobristKeys::m_zkeyEnpassant[8]; //[FILE]

enum CASTLING_TYPE_SIMPLE { CASTLING_KING=0, CASTLING_QUEEN=1 };

//Generate the zKey bitwords. Call once before running the program
void ZobristKeys::Init() {
    Utils::PRNG_64 random(70);
    //Color
    m_zkeyColor = random.Random();
    for(COLOR color : {WHITE, BLACK}) {
        //Pieces
        for(int pieceType = PAWN; pieceType <= KING; pieceType++) {
            for(int square = A1; square <= H8; square++) {
                m_zkeyPieces[color][pieceType][square] = random.Random();
            }
        }
        //Castling rights
        for(CASTLING_TYPE_SIMPLE castlingType : {CASTLING_KING, CASTLING_QUEEN}) {
            m_zkeyCastling[color][castlingType] = random.Random();
        }
    }
    //En passant
    for(int file = FILEA; file <= FILEH; file++) {
        m_zkeyEnpassant[file] = random.Random();
    }
}

ZobristKey::ZobristKey() : m_key(0) {}

void ZobristKey::SetKey(Board& board) {
    m_key = 0;

    //Color
    if(board.ActivePlayer() == BLACK) {
        m_key ^= ZobristKeys::m_zkeyColor;
    }

    for(COLOR color : {WHITE, BLACK}) {
        //Pieces
        for(PIECE_TYPE pieceType = PAWN; pieceType <= KING; ++pieceType) {
            Bitboard thePieces = board.GetPieces(color, pieceType);
            while(thePieces) {
                int square = ResetLsb(thePieces);
                m_key ^= ZobristKeys::m_zkeyPieces[color][pieceType][square];
            }
        }
    }

    //Castling rights
    if(board.CastlingRights() & CASTLING_K) m_key ^= ZobristKeys::m_zkeyCastling[WHITE][CASTLING_KING];
    if(board.CastlingRights() & CASTLING_Q) m_key ^= ZobristKeys::m_zkeyCastling[WHITE][CASTLING_QUEEN];
    if(board.CastlingRights() & CASTLING_k) m_key ^= ZobristKeys::m_zkeyCastling[BLACK][CASTLING_KING];
    if(board.CastlingRights() & CASTLING_q) m_key ^= ZobristKeys::m_zkeyCastling[BLACK][CASTLING_QUEEN];

    //Enpassant
    Bitboard epSquare = board.EnPassantSquare();
    int file = File( BitscanForward(epSquare) );
    m_key ^= ZobristKeys::m_zkeyEnpassant[file];
}

void ZobristKey::SetPawnKey(Board& board) {
    m_key = 0;

    for(COLOR color : {WHITE, BLACK}) {
        Bitboard thePawns = board.GetPieces(color, PAWN);
        while(thePawns) {
            int square = ResetLsb(thePawns);
            m_key ^= ZobristKeys::m_zkeyPieces[color][PAWN][square];
        }
    }
}

void ZobristKey::UpdateColor() {
    m_key ^= ZobristKeys::m_zkeyColor;
}
void ZobristKey::UpdatePiece(COLOR color, PIECE_TYPE pieceType, int square) {
    m_key ^= ZobristKeys::m_zkeyPieces[color][pieceType][square];
}
void ZobristKey::UpdateCastling(CASTLING_TYPE castlingType) {
    switch(castlingType) {
        case CASTLING_K: m_key ^= ZobristKeys::m_zkeyCastling[WHITE][CASTLING_KING]; break;
        case CASTLING_Q: m_key ^= ZobristKeys::m_zkeyCastling[WHITE][CASTLING_QUEEN]; break;
        case CASTLING_k: m_key ^= ZobristKeys::m_zkeyCastling[BLACK][CASTLING_KING]; break;
        case CASTLING_q: m_key ^= ZobristKeys::m_zkeyCastling[BLACK][CASTLING_QUEEN]; break;
        default: assert(false);
    };
}
void ZobristKey::UpdateEnpassant(Bitboard enpassant) {
    int file = File( BitscanForward(enpassant) );
    m_key ^= ZobristKeys::m_zkeyEnpassant[file];
}
