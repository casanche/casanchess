#include "ZobristKeys.h"

#include "BitboardUtils.h"
#include "Board.h"

U64 ZobristKeys::m_zkeyPieces[2][8][64];
U64 ZobristKeys::m_zkeyCastling[2][2];
U64 ZobristKeys::m_zkeyEnpassant[8];
U64 ZobristKeys::m_zkeyColor;

//Generate the zKey bitwords. Call once before running the program
void ZobristKeys::Init() {
    PRNG random;
    //color
    m_zkeyColor = random.Random();
    for(int color = WHITE; color <= BLACK; color++) {
        //Castling rights
        for(int castlingType = 0; castlingType <= 1; castlingType++) {
            m_zkeyCastling[color][castlingType] = random.Random();
        }
        //Pieces
        for(int pieceType = PAWN; pieceType <= KING; pieceType++) {
            for(int square = A1; square <= H8; square++) {
                m_zkeyPieces[color][pieceType][square] = random.Random();
            }
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

    //Castling rights
    if(board.CastlingRights() & CASTLING_K) m_key ^= ZobristKeys::m_zkeyCastling[WHITE][0];
    if(board.CastlingRights() & CASTLING_Q) m_key ^= ZobristKeys::m_zkeyCastling[WHITE][1];
    if(board.CastlingRights() & CASTLING_k) m_key ^= ZobristKeys::m_zkeyCastling[BLACK][0];
    if(board.CastlingRights() & CASTLING_q) m_key ^= ZobristKeys::m_zkeyCastling[BLACK][1];

    for(int color = WHITE; color <= BLACK; color++) {
        //Pieces
        for(int pieceType = PAWN; pieceType <= KING; pieceType++) {
            Bitboard thePieces = board.GetPieces((COLORS)color, (PIECE_TYPE)pieceType);
            while(thePieces) {
                int square = ResetLsb(thePieces);
                m_key ^= ZobristKeys::m_zkeyPieces[color][pieceType][square];
            }
        }
    }

    //Enpassant
    Bitboard epSquare = board.EnPassantSquare();
    int file = File( BitscanForward(epSquare) );
    m_key ^= ZobristKeys::m_zkeyEnpassant[file];
}

void ZobristKey::UpdateColor() {
    m_key ^= ZobristKeys::m_zkeyColor;
}
void ZobristKey::UpdateEnpassant(Bitboard enpassant) {
    int file = File( BitscanForward(enpassant) );
    m_key ^= ZobristKeys::m_zkeyEnpassant[file];
}
void ZobristKey::UpdatePiece(COLORS color, PIECE_TYPE pieceType, int square) {
    m_key ^= ZobristKeys::m_zkeyPieces[color][pieceType][square];
}
void ZobristKey::UpdateCastling(CASTLING_TYPE castlingType) {
    switch(castlingType) {
        case CASTLING_K: m_key ^= ZobristKeys::m_zkeyCastling[WHITE][0]; break;
        case CASTLING_Q: m_key ^= ZobristKeys::m_zkeyCastling[WHITE][1]; break;
        case CASTLING_k: m_key ^= ZobristKeys::m_zkeyCastling[BLACK][0]; break;
        case CASTLING_q: m_key ^= ZobristKeys::m_zkeyCastling[BLACK][1]; break;
        default: assert(false);
    };
}

//Pseudo RNG for 64-bitwords
PRNG::PRNG() : m_mersenne(m_device()) {}

//Generate a random 64-bitword
U64 PRNG::Random() {
    return m_distribution(m_mersenne);
}
