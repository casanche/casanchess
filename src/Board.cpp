#include "Board.h"

#include "Attacks.h"
#include "Uci.h"
using namespace Attacks;
#include "BitboardUtils.h"
using namespace BitboardUtils;
#include "Hash.h"
#include "Heuristics.h"
#include "Move.h"
#include "MoveGenerator.h"
#include "NNUE.h"

#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

const char PIECE_NOTATION[2][8] = { {' ', 'P', 'N', 'B', 'R', 'Q', 'K', '-',},
                                    {' ', 'p', 'n', 'b', 'r', 'q', 'k', '-'} }; //[COLOR][PIECE]

/*
------------
Useful sites
------------
Basics:
    http://pages.cs.wisc.edu/~psilord/blog/data/chess-pages/
Move generation:
    https://peterellisjones.com/posts/generating-legal-chess-moves-efficiently/
    https://www.chessprogramming.org/Efficient_Generation_of_Sliding_Piece_Attacks
*/

Board::Board() {
    Init();
}

void Board::ClearBits() {
    std::memset(m_pieces, 0, sizeof(m_pieces));

    for(COLOR color : {WHITE, BLACK}) {
        m_kingAttackers[color] = 0;
    }

    for(int i = 0; i < MAX_PLY_HISTORY; ++i) {
        m_history[i].Clear();
    }

    m_enPassantSquare = ZERO;
    m_castlingRights = 0;

    m_activePlayer = WHITE;
    m_moveNumber = 1;
    m_ply = 0;
    m_initialPly = 0;
    m_fiftyrule = 0;

    UpdateBitboards();
}

void Board::Init() {
    SetFen(INITIAL_POSITION_FEN);
}

u64 Board::Perft(int depth) {
    u64 nodes = 0;

    MoveList moves = MoveGenerator::GenerateMoves(*this);

    if(depth == 0) return 1;
    if(depth == 1) return moves.size();

    for(auto move : moves) {
        
        //Integrity check: before
        D( BoardIdentity bef = BoardIntegrityChecker::GenerateBoardIdentity(*this); );

        MakeMove(move);
        nodes += Perft(depth - 1);
        TakeMove(move);

        //Integrity check: after
        D( BoardIdentity aft = BoardIntegrityChecker::GenerateBoardIdentity(*this); );
        D( assert(bef == aft); );
        D( assert(BoardIntegrityChecker::CheckIntegrity(*this)); );
    }

    return nodes;
}

void Board::InitStateAndHistory() {
    UpdateBitboards();

    m_history[m_ply].fiftyrule = m_fiftyrule;
    m_history[m_ply].castling = m_castlingRights;
    m_history[m_ply].enpassant = m_enPassantSquare;
    m_zobristKey.SetKey(*this);
    m_pawnKey.SetPawnKey(*this);
    m_history[m_ply].zkey = ZKey();

    m_checkCalculated = false;

    if(!UCI_CLASSICAL_EVAL)
        m_nnue.Inputs_FullUpdate(m_ply, m_pieces);
}

void Board::Divide(int depth) {
    u64 nodesTotal = 0;

    MoveList moves = MoveGenerator::GenerateMoves(*this);

    for(auto move : moves) {
        MakeMove(move);
        u64 nodes = Perft(depth-1);
        P( move.Notation() << " \t" << nodes );
        nodesTotal += nodes;
        TakeMove(move);
    }

    P("nodes: " << nodesTotal);
}

void Board::Print(bool bits) const {
    if(bits) {
        PrintBits( m_pieces[WHITE][ALL_PIECES] | m_pieces[BLACK][ALL_PIECES] );
        return;
    }

    std::string squareMap[64];
    for (int i = 0; i < 64; ++i) {
        squareMap[i] = PIECE_NOTATION[0][NO_PIECE];
    }

    for(COLOR color : {WHITE, BLACK}) {
        for(PIECE_TYPE piece = PAWN; piece <= KING; ++piece) {
            Bitboard thePieces = m_pieces[color][piece];
            for(int index : BitboardIterator(thePieces)) {
                squareMap[index] = '\0';
                squareMap[index] += (color == WHITE) ? "\033[1;97m" : "\033[1;31m"; //white or red
                squareMap[index] += PIECE_NOTATION[color][piece];
                squareMap[index] += "\033[0m";
            }
        }
    }

    std::cout << "--------------------------------" << std::endl;
    int square = A8;
    const int nextRank = 8;
    while(square >= 0) {
        std::cout << " " << squareMap[square] << " |";
        if(File(square) == FILEH) {
            square -= nextRank * 2;
            std::cout << std::endl << "--------------------------------" << std::endl;
        }
        square++;
    }

    std::cout << "FEN (simplified): " << GetSimplifiedFen() << std::endl;
}

void Board::ShowHistory() {
    for(uint i = m_initialPly + 1; i <= m_ply; i++) {
        std::cout << i << ". " << m_history[i].move.Notation() << " ";
    }
}

void Board::ShowMoves() {
    MoveList moves = MoveGenerator::GenerateMoves(*this);
    for(auto move : moves)
        move.Print();
    P("size: " << moves.size());
}

// Static Exchange Evaluation (SEE)
// Evaluates a capture sequence to determine if it wins or loses material.
// Simulates all recaptures on the target square using least valuable attacker first.
//
// It works as well for normal moves, en-passant and promotions.
//
// Example: White Pawn takes Black Knight, Black Pawn recaptures
//   gain[0] = 300 (Black Knight captured by White)
//   gain[1] = 100 (White Pawn captured by Black)
//
//   Minimax evaluation (bottom-up):
//   depth 1: gain[0] -= max(0, gain[1]) -> 300 - max(0, 100) = 200
//   Result: +200 (Net: Knight gained, Pawn lost).
int Board::SEE(Move move) const {
    MoveData moveData = move.Data();

    // Array to store the captured piece value at each depth
    int gain[32];
    int depth = 0;

    // Piece value of the captured piece
    gain[0] = SEE::MATERIAL_VALUES[moveData.capturedType];

    // Our piece that captures
    PIECE_TYPE attackingPiece = moveData.pieceType;

    // Special case: promotions
    if(move.IsPromotion()) {
        switch(move.PromotionType()) {
            case PROMOTION_QUEEN:  attackingPiece = QUEEN;  break;
            case PROMOTION_KNIGHT: attackingPiece = KNIGHT; break;
            case PROMOTION_ROOK:   attackingPiece = ROOK;   break;
            case PROMOTION_BISHOP: attackingPiece = BISHOP; break;
        }
        gain[0] += SEE::MATERIAL_VALUES[attackingPiece] - SEE::MATERIAL_VALUES[PAWN];
    }

    // Remove the attacker from the original square
    Bitboard occupied = m_allpieces ^ SquareBB(moveData.fromSq);
    COLOR sideToMove = InactivePlayer();

    // Special case: en-passant
    if(moveData.moveType == ENPASSANT) {
        int capturedPawnSq = moveData.toSq + (ActivePlayer() == WHITE ? -8 : 8);
        // Move the attacker pawn to the target square and remove the captured pawn
        occupied ^= (SquareBB(moveData.toSq) | SquareBB(capturedPawnSq));
    }

    // Keep iterating until there are no more attackers on the target square
    while(true) {
        Bitboard attackers = AttackersTo((COLOR)!sideToMove, moveData.toSq, occupied) & occupied;

        if(!attackers) break;

        PIECE_TYPE lvaPiece;
        Bitboard lvaBitboard = LeastValuableAttacker(attackers, sideToMove, lvaPiece);

        gain[depth + 1] = SEE::MATERIAL_VALUES[attackingPiece];

        // New piece at target square
        attackingPiece = lvaPiece;

        // Promotions during recapture
        if (attackingPiece == PAWN && (Rank(moveData.toSq) == RANK1 || Rank(moveData.toSq) == RANK8)) {
            attackingPiece = QUEEN;
            gain[depth + 1] += SEE::MATERIAL_VALUES[QUEEN] - SEE::MATERIAL_VALUES[PAWN];
        }

        // Remove the LVA from the original square
        occupied ^= lvaBitboard;
        sideToMove = (COLOR)!sideToMove;

        depth++;
    }

    // Evaluate sequence from depth to 0, negamax style
    for(int i = depth; i > 0; i--) {
        gain[i - 1] -= std::max(0, gain[i]); // Don't capture if the gain is negative
    }

    return gain[0];
}

//Attackers
Bitboard Board::AttackersTo(COLOR color, int square) const {
    assert(square >= 0 && square < 64);

    Bitboard attackers = ZERO;

    COLOR enemyColor = (COLOR)!color;

    //Non-sliding
    attackers |= AttacksPawns(color, square) & GetPieces(enemyColor, PAWN);
    attackers |= AttacksKnights(square) & GetPieces(enemyColor, KNIGHT);
    attackers |= AttacksKing(square) & GetPieces(enemyColor, KING);

    //Eliminate enemy king for sliding calculation
    Bitboard blockers = m_allpieces ^ GetPieces(color, KING);

    //Bishop
    Bitboard potentialAttackers = AttacksSliding(BISHOP, square, blockers);
    attackers |= potentialAttackers & ( GetPieces(enemyColor, BISHOP) | GetPieces(enemyColor, QUEEN) );

    //Rook
    potentialAttackers = AttacksSliding(ROOK, square, blockers);
    attackers |= potentialAttackers & ( GetPieces(enemyColor, ROOK) | GetPieces(enemyColor, QUEEN) );

    return attackers;
}

Bitboard Board::AttackersTo(COLOR color, int square, Bitboard blockers) const {
    assert(square >= 0 && square < 64);
    
    Bitboard attackers = ZERO;
    COLOR enemyColor = (COLOR)!color;

    //Non-sliding
    attackers |= AttacksPawns(color, square) & GetPieces(enemyColor, PAWN);
    attackers |= AttacksKnights(square) & GetPieces(enemyColor, KNIGHT);
    attackers |= AttacksKing(square) & GetPieces(enemyColor, KING);

    //Bishop
    Bitboard potentialAttackers = AttacksSliding(BISHOP, square, blockers);
    attackers |= potentialAttackers & ( GetPieces(enemyColor, BISHOP) | GetPieces(enemyColor, QUEEN) );

    //Rook
    potentialAttackers = AttacksSliding(ROOK, square, blockers);
    attackers |= potentialAttackers & ( GetPieces(enemyColor, ROOK) | GetPieces(enemyColor, QUEEN) );

    return attackers;
}

PIECE_TYPE Board::GetPieceAtSquare(COLOR color, int square) const {
    Bitboard bit = SquareBB(square);

    PIECE_TYPE piece = NO_PIECE;

    if(      bit & m_pieces[color][PAWN] )   piece = PAWN;
    else if( bit & m_pieces[color][KNIGHT] ) piece = KNIGHT;
    else if( bit & m_pieces[color][BISHOP] ) piece = BISHOP;
    else if( bit & m_pieces[color][ROOK] )   piece = ROOK;
    else if( bit & m_pieces[color][QUEEN] )  piece = QUEEN;
    else if( bit & m_pieces[color][KING] )   piece = KING;

    return piece;
}

bool Board::IsAttacked(COLOR color, int square) const {
    COLOR enemyColor = (COLOR)!color;

    if( AttacksPawns(color, square) & Piece(enemyColor, PAWN) ) return true;
    if( AttacksKnights(square) & Piece(enemyColor, KNIGHT) ) return true;
    if( AttacksKing(square) & Piece(enemyColor, KING) ) return true;

    Bitboard blockers = m_allpieces;
    Bitboard diagonalPieces = Piece(enemyColor, BISHOP) | Piece(enemyColor, QUEEN);
    if( AttacksSliding(BISHOP, square, blockers) & diagonalPieces ) return true;
    Bitboard straightPieces = Piece(enemyColor, ROOK) | Piece(enemyColor, QUEEN);
    if( AttacksSliding(ROOK, square, blockers) & straightPieces ) return true;

    return false;
}

bool Board::IsCheck() {
    COLOR color = ActivePlayer();
    if(!m_checkCalculated) {
        UpdateKingAttackers(color);
        m_checkCalculated = true;
    }
    return m_kingAttackers[color];
}

bool Board::IsCheckAnyColor() {
    //Active color
    m_checkCalculated = false;
    bool check_active = IsCheck();

    //Inactive color
    m_checkCalculated = false;
    m_activePlayer = (COLOR)!m_activePlayer;
    bool check_inactive = IsCheck();
    m_activePlayer = (COLOR)!m_activePlayer;
    m_checkCalculated = false;

    return check_active || check_inactive;
}

// Detects a position repetition within a search (same Zobrist Key)
bool Board::IsRepetitionDraw() const {
    const int rule_limit = (int)m_fiftyrule;
    const int ply_limit = m_ply - m_initialPly;

    const int limit = std::min(rule_limit, ply_limit);

    for(int i = 4; i <= limit; i += 2) {
        if(m_history[m_ply - i].zkey == ZKey()) {
            return true;
        }
    }

    return false;
}

void Board::Mirror() {
    m_activePlayer = (COLOR)!m_activePlayer;
    for(PIECE_TYPE pieceType = PAWN; pieceType <= KING; ++pieceType) {
        m_pieces[WHITE][pieceType] = BitboardUtils::Mirror(m_pieces[WHITE][pieceType]);
        m_pieces[BLACK][pieceType] = BitboardUtils::Mirror(m_pieces[BLACK][pieceType]);
        std::swap(m_pieces[WHITE][pieceType], m_pieces[BLACK][pieceType]);
    }
    if(m_enPassantSquare)
        m_enPassantSquare = BitboardUtils::Mirror(m_enPassantSquare);
    if(m_castlingRights) {
        u8 mirroredCastlingRights = 0;
        if(m_castlingRights & CASTLING_K) mirroredCastlingRights ^= CASTLING_k;
        if(m_castlingRights & CASTLING_Q) mirroredCastlingRights ^= CASTLING_q;
        if(m_castlingRights & CASTLING_k) mirroredCastlingRights ^= CASTLING_K;
        if(m_castlingRights & CASTLING_q) mirroredCastlingRights ^= CASTLING_Q;
        m_castlingRights = mirroredCastlingRights;
    }
    m_zobristKey.SetKey(*this);
    m_pawnKey.SetPawnKey(*this);
    UpdateBitboards();
}

int Board::SquareToIndex(std::string square) const {
    const int asciiToNumber = -96;  //lowercase to numbers

    int file = square[0] + asciiToNumber;
    int rank = atoi(&square[1]);

    int index = (file-1) + (rank-1)*8;
    assert(index >= 0 && index < 64);
    return index;
}

Bitboard Board::LeastValuableAttacker(Bitboard attackers, COLOR color, PIECE_TYPE& pieceType) const {
    assert(attackers);
    for(PIECE_TYPE ipiece = PAWN; ipiece <= KING; ++ipiece) {
        Bitboard pieceAttackers = attackers & Piece(color, ipiece);
        if(pieceAttackers) {
            pieceType = ipiece;
            return IsolateLsb(pieceAttackers);
        }
    }
    assert(false);
    return ZERO;
}

//Private

void Board::PutPiece(COLOR color, PIECE_TYPE pieceType, int square) {
    assert( !(m_allpieces & SquareBB(square)) );
    m_pieces[color][pieceType] ^= SquareBB(square);
}

void Board::UpdateBitboards() {
    m_pieces[WHITE][ALL_PIECES] =
        m_pieces[WHITE][PAWN] | 
        m_pieces[WHITE][KNIGHT] | 
        m_pieces[WHITE][BISHOP] | 
        m_pieces[WHITE][ROOK] | 
        m_pieces[WHITE][QUEEN] | 
        m_pieces[WHITE][KING];
    m_pieces[BLACK][ALL_PIECES] =
        m_pieces[BLACK][PAWN] | 
        m_pieces[BLACK][KNIGHT] | 
        m_pieces[BLACK][BISHOP] | 
        m_pieces[BLACK][ROOK] | 
        m_pieces[BLACK][QUEEN] | 
        m_pieces[BLACK][KING];
    m_allpieces = m_pieces[WHITE][ALL_PIECES] | m_pieces[BLACK][ALL_PIECES];
}

void Board::UpdateKingAttackers(COLOR color) {
    Bitboard theKing = GetPieces(color, KING);
    int kingSquare = BitscanForward(theKing);

    m_kingAttackers[color] = AttackersTo(color, kingSquare);
}
