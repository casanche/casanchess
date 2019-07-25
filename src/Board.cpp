#include "Board.h"

#include "Attacks.h"
using namespace Attacks;
#include "BitboardUtils.h"
using namespace BitboardUtils;
#include "Evaluation.h"
using namespace Evaluation;
#include "Move.h"
#include "MoveGenerator.h"

#include <iostream>
#include <sstream>
#include <string>

const std::string STARTFEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

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
    Attacks::Init();
    Evaluation::Init(); //after Attacks
    ZobristKeys::Init();
    Init();
}

void Board::Init() {
    SetFen(STARTFEN);
}

void Board::Divide(int depth) {
    U64 nodesTotal = 0;

    MoveGenerator generator;
    MoveList moves = generator.GenerateMoves(*this);

    for(auto &move : moves) {
        MakeMove(move);
        U64 nodes = Perft(depth-1);
        P( move.Notation() << " \t" << nodes );
        nodesTotal += nodes;
        TakeMove(move);
    }

    P("nodes: " << nodesTotal);
}

void Board::Mirror() {
    m_activePlayer = (COLORS)!m_activePlayer;
    for(PIECE_TYPE pieceType = PAWN; pieceType <= KING; ++pieceType) {
        m_pieces[WHITE][pieceType] = BitboardUtils::Mirror(m_pieces[WHITE][pieceType]);
        m_pieces[BLACK][pieceType] = BitboardUtils::Mirror(m_pieces[BLACK][pieceType]);
        std::swap(m_pieces[WHITE][pieceType], m_pieces[BLACK][pieceType]);
    }
    if(m_enPassantSquare)
        m_enPassantSquare = BitboardUtils::Mirror(m_enPassantSquare);
    if(m_castlingRights) {
        U8 mirroredCastlingRights = 0;
        if(m_castlingRights & CASTLING_K) mirroredCastlingRights ^= CASTLING_k;
        if(m_castlingRights & CASTLING_Q) mirroredCastlingRights ^= CASTLING_q;
        if(m_castlingRights & CASTLING_k) mirroredCastlingRights ^= CASTLING_K;
        if(m_castlingRights & CASTLING_q) mirroredCastlingRights ^= CASTLING_Q;
        m_castlingRights = mirroredCastlingRights;
    }
    UpdateBitboards();
}

PIECE_TYPE Board::GetPieceAtSquare(COLORS color, int square) const {
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

void Board::MakeMove(Move move) {
    // Get move information
    COLORS color = ActivePlayer();
    int fromSq = move.FromSq();
    int toSq = move.ToSq();
    PIECE_TYPE pieceType = move.PieceType();
    MOVE_TYPE moveType = move.MoveType();

    //Increase ply
    ++m_ply;
    if(color == BLACK) {
        ++m_moveNumber;
    }
    //Fifty-rule
    if(pieceType == PAWN || moveType == CAPTURE) {
        m_fiftyrule = 0;
    } else {
        ++m_fiftyrule;
    }

    //Reset en-passant square
    if(m_enPassantSquare) {
        m_zobristKey.UpdateEnpassant(m_enPassantSquare);
        m_enPassantSquare = ZERO;
    }

    // Move the active piece
    MovePiece(fromSq, toSq, color, pieceType);

    //Remove captured piece
    if(moveType == CAPTURE || moveType == PROMOTION_CAPTURE) {
        RemovePiece(toSq, InactivePlayer(), move.CapturedType());
    }
    else if(moveType == DOUBLE_PUSH) {
        int squareShift = -8 + 16*color;
        m_enPassantSquare = SquareBB(toSq + squareShift);
        m_zobristKey.UpdateEnpassant(m_enPassantSquare);
    }
    else if(moveType == ENPASSANT) {
        COLORS enemyColor = InactivePlayer();
        int squareShift = -8 + 16*color;
        RemovePiece(toSq + squareShift, enemyColor, PAWN);
    }
    if(moveType == PROMOTION || moveType == PROMOTION_CAPTURE) {
        RemovePiece(toSq, color, PAWN);
        PROMOTION_TYPE promotionType = move.PromotionType();
        switch(promotionType) {
            case(PROMOTION_QUEEN):  AddPiece(toSq, color, QUEEN);   break;
            case(PROMOTION_KNIGHT): AddPiece(toSq, color, KNIGHT);  break;
            case(PROMOTION_ROOK):   AddPiece(toSq, color, ROOK);    break;
            case(PROMOTION_BISHOP): AddPiece(toSq, color, BISHOP);  break;
            default: assert(false);
        };
    }

    //Castling rights
    UpdateCastlingRights(move);

    //Change the active player
    m_activePlayer = InactivePlayer();
    m_zobristKey.UpdateColor();

    //Store irreversible information (to help a later TakeMove)
    assert(m_ply >= 0 && m_ply <= MAX_PLIES);
    m_history[m_ply].fiftyrule = m_fiftyrule;
    m_history[m_ply].castling = m_castlingRights;
    m_history[m_ply].zkey = ZKey();
    m_history[m_ply].enpassant = m_enPassantSquare;
    m_history[m_ply].move = move;

    //Update helper bitboards
    UpdateBitboards();

    //Asserts
    assert(CheckIntegrity());
    assert(~m_allpieces & SquareBB(fromSq) );
    assert( m_allpieces & SquareBB(toSq) );
}

void Board::TakeMove(Move move) {
    // Get move information
    int fromSq = move.FromSq();
    int toSq = move.ToSq();
    PIECE_TYPE pieceType = move.PieceType();
    MOVE_TYPE moveType = move.MoveType();

    //Change the active player
    m_activePlayer = InactivePlayer();
    COLORS color = m_activePlayer;
    m_zobristKey.UpdateColor();

    //Decrease the move number if black made the move
    --m_ply;
    if(color == BLACK) {
        --m_moveNumber;
    }

    //Promotions before moving piece
    if(moveType == PROMOTION || moveType == PROMOTION_CAPTURE) {
        AddPiece(toSq, color, PAWN);
        PROMOTION_TYPE promotionType = move.PromotionType();
        switch(promotionType) {
            case(PROMOTION_QUEEN):  RemovePiece(toSq, color, QUEEN);   break;
            case(PROMOTION_KNIGHT): RemovePiece(toSq, color, KNIGHT);  break;
            case(PROMOTION_ROOK):   RemovePiece(toSq, color, ROOK);    break;
            case(PROMOTION_BISHOP): RemovePiece(toSq, color, BISHOP);  break;
            default: assert(false);
        };
    }

    // Move the active piece
    MovePiece(toSq, fromSq, color, pieceType);

    //Remove captured piece
    if(moveType == CAPTURE || moveType == PROMOTION_CAPTURE) {
        AddPiece(toSq, InactivePlayer(), move.CapturedType());
    }
    else if(moveType == ENPASSANT) {
        COLORS enemyColor = InactivePlayer();
        int squareShift = -8 + 16*color;
        AddPiece(toSq + squareShift, enemyColor, PAWN);
    }

    //Retrieve history

    //Retrieve castling rights
    RewindCastlingRights(move);

    //Retrieve 50-move counter
    m_fiftyrule = m_history[m_ply].fiftyrule;

    //Retrieve enpassant
    Bitboard changedBits = m_enPassantSquare ^ m_history[m_ply].enpassant;
    if(changedBits) {
        if(m_enPassantSquare)
            m_zobristKey.UpdateEnpassant(m_enPassantSquare);
        if(m_history[m_ply].enpassant)
            m_zobristKey.UpdateEnpassant(m_history[m_ply].enpassant);
        m_enPassantSquare = m_history[m_ply].enpassant;
    }

    //Update helper bitboards
    UpdateBitboards();

    //Asserts
    assert(CheckIntegrity());
    assert(m_allpieces & SquareBB(fromSq));
}

void Board::MakeMove(std::string input) {
    MakeMove( StringToMove(input) );
}
void Board::TakeMove() {
    TakeMove( m_history[m_ply].move );
}

void Board::MakeNull() {
    Move move = Move();

    m_ply++;
    assert(m_ply <= MAX_PLIES);

    //Reset en-passant square
    if(m_enPassantSquare) {
        m_zobristKey.UpdateEnpassant(m_enPassantSquare);
        m_enPassantSquare = ZERO;
    }

    //Change the active player
    m_activePlayer = InactivePlayer();
    m_zobristKey.UpdateColor();

    //Store irreversible information (to help a later TakeMove)
    m_history[m_ply].fiftyrule = m_fiftyrule;
    m_history[m_ply].castling = m_castlingRights;
    m_history[m_ply].zkey = ZKey();
    m_history[m_ply].enpassant = m_enPassantSquare;
    m_history[m_ply].move = move;

    //Asserts
    assert(move.MoveType() == NULLMOVE);
}

void Board::TakeNull() {
    //Change the active player
    m_activePlayer = InactivePlayer();

    m_ply--;
    assert(m_ply >= 0);

    //Retrieve state
    m_fiftyrule = m_history[m_ply].fiftyrule;
    m_castlingRights = m_history[m_ply].castling;
    m_zobristKey.SetKey( m_history[m_ply].zkey );
    m_enPassantSquare = m_history[m_ply].enpassant;
}

int Board::SquareToIndex(std::string square) {
    const int asciiToNumber = 96; //uncapital letters to numbers

    int file = square[0] - asciiToNumber;
    int rank = atoi(&square[1]);

    int index = (file-1) + (rank-1)*8;
    assert(index >= 0 && index < 64);
    return index;
}

Move Board::StringToMove(std::string input) {

    bool incorrectMoveSize = input.size() < 4 || input.size() > 5;
    if(incorrectMoveSize) {
        std::cout << "Not a move: " << input << " (incorrect move size)" << std::endl;
        assert(false);
    }

    int fromSq = SquareToIndex( input.substr(0, 2) );
    int toSq = SquareToIndex( input.substr(2, 2) );

    char promLet = '\0';
    if(input.size() == 5)
        promLet = input[4];

    return DescriptiveToMove((SQUARES)fromSq, (SQUARES)toSq, promLet);
}
Move Board::DescriptiveToMove(SQUARES fromSq, SQUARES toSq, char promLet) {
    Move move;

    PIECE_TYPE activePiece = GetPieceAtSquare(m_activePlayer, fromSq);
    PIECE_TYPE capturedPiece = GetPieceAtSquare( (COLORS)!m_activePlayer, toSq);

    int sign = (m_activePlayer == WHITE) ? +1 : -1;

    bool isCastling = (activePiece == KING)
        && (fromSq == E1 || fromSq == E8)
        && (toSq == G8 || toSq == G1 || toSq == C8 || toSq == C1);
    bool isDoublePush = (activePiece == PAWN)
        && (abs(Rank(fromSq) - Rank(toSq)) == 2);
    bool isEnpassant = (activePiece == PAWN)
        && ~Piece(InactivePlayer(),ALL_PIECES) & SquareBB(toSq) //no piece in the destination square
        && Piece(InactivePlayer(),PAWN) & ( SquareBB(toSq - 8 * sign) ); //enemy pawn behind the destination square
    bool isPromotion = (activePiece == PAWN)
        && ((Rank(toSq) == RANK8 && m_activePlayer == WHITE)
        || (Rank(toSq) == RANK1 && m_activePlayer == BLACK));
    
    if(isDoublePush) {
        move = Move(fromSq, toSq, activePiece, MOVE_TYPE::DOUBLE_PUSH);
    }
    else if(isCastling) {
        move = Move(fromSq, toSq, activePiece, MOVE_TYPE::CASTLING);
    }
    else if(isEnpassant) {
        move = Move(fromSq, toSq, activePiece, MOVE_TYPE::ENPASSANT);
        move.SetCapturedType(PAWN);
    }
    else if(isPromotion) {
        if(capturedPiece) {
            move = Move(fromSq, toSq, activePiece, MOVE_TYPE::PROMOTION_CAPTURE);
            move.SetCapturedType(capturedPiece);
        } else {
            move = Move(fromSq, toSq, activePiece, MOVE_TYPE::PROMOTION);
        }
        switch(promLet) {
            case 'q': move.SetPromotionFlag(PROMOTION_QUEEN);  break;
            case 'n': move.SetPromotionFlag(PROMOTION_KNIGHT); break;
            case 'r': move.SetPromotionFlag(PROMOTION_ROOK);   break;
            case 'b': move.SetPromotionFlag(PROMOTION_BISHOP); break;
            default: { P("ASSERT Failing promLetter: " << promLet); assert(false); }
        };
    }
    else if(capturedPiece) {
        move = Move(fromSq, toSq, activePiece, MOVE_TYPE::CAPTURE);
        move.SetCapturedType(capturedPiece);
    }
    else {
        move = Move(fromSq, toSq, activePiece, MOVE_TYPE::NORMAL);
    }

    // move.Print();

    return move;
}

U64 Board::Perft(int depth) {
    U64 nodes = 0;

    MoveGenerator generator;
    MoveList moves = generator.GenerateMoves(*this);

    if(depth == 0) return 1;
    if(depth == 1) return moves.size();

    for(auto &move : moves) {
        
        //Integrity check: before
        #ifdef DEBUG
        Board boardBefore = *this;
        #endif

        MakeMove(move);
        nodes += Perft(depth - 1);
        TakeMove(move);

        //Integrity check: after
        #ifdef DEBUG
        Board boardAfter = *this;
        assert(boardBefore == boardAfter);
        #endif
    }

    return nodes;
}

void Board::Print(bool bits) const {
    if(bits) {
        PrintBits( m_pieces[WHITE][ALL_PIECES] | m_pieces[BLACK][ALL_PIECES] );
        return;
    }

    std::string squareMap[64];
    for (int i = 0; i < 64; ++i) {
        squareMap[i] = PIECE_LETTERS[NO_PIECE];
    }

    for(int color = WHITE; color <= BLACK; color++) {
        for(int piece = PAWN; piece <= KING; piece++) {
            Bitboard thePieces = m_pieces[color][piece];
            while(thePieces) {
                int index = ResetLsb(thePieces);
                squareMap[index] = '\0';
                squareMap[index] += (color == WHITE) ? "\033[1;97m" : "\033[1;31m";
                squareMap[index] += PIECE_LETTERS[ PieceTypeToPieces(piece, color) ];
                squareMap[index] += "\033[0m";
            }
        }
    }

    std::cout << "--------------------------------" << std::endl;
    int i = A8;
    while(i != -8) {
        std::cout << " " << squareMap[i] << " |";
        if(i % 8 == 7) {
            i -= 16;
            std::cout << std::endl << "--------------------------------" << std::endl;
        }
        i++;
    }
}

void Board::SetFen(std::string fenString) {
    m_fen.SetPosition(*this, fenString);
}

void Board::ShowHistory() {
    for(uint i = m_initialPly + 1; i <= m_ply; i++) {
        std::cout << i << ". " << m_history[i].move.Notation() << " ";
    }
}

void Board::ShowMoves() {
    MoveGenerator moveGenerator;
    MoveList moves = moveGenerator.GenerateMoves(*this);
    for(auto move : moves ) {
        move.Print();
    }
    P("size: " << moveGenerator.Moves().size());
}

//Attackers
Bitboard Board::AttackersTo(COLORS color, int square) const {
    Bitboard attackers = ZERO;

    COLORS enemyColor = (COLORS)!color;

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

Bitboard Board::AttackersTo(COLORS color, int square, Bitboard blockers) const {
    Bitboard attackers = ZERO;
    COLORS enemyColor = (COLORS)!color;

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

bool Board::IsCheck() {
    COLORS color = ActivePlayer();
    UpdateKingAttackers(color);
    return m_kingAttackers[color];
}
bool Board::IsCheck(COLORS color) {
    UpdateKingAttackers(color);
    return m_kingAttackers[color];
}

bool Board::IsRepetitionDraw(int searchPly) {
    int rep = 0;

    int imax = std::max(searchPly, (int)m_fiftyrule);
    if(imax < 4) //FIXME
        return false;

    for(int i = 4; i <= imax; i += 2) {
        // if(m_ply - (uint)i < 0) return false;
        if((m_ply - (uint)i) < m_initialPly) return false; //non-defined history
        // assert(m_ply >= (uint)i);

        if(ZKey() == m_history[m_ply - i].zkey) rep++;
        if(rep == 1 && searchPly > i) return true;
        if(rep == 2) return true;
    }
    return false;
}

Bitboard Board::XRayAttackersTo(COLORS color, int square) {
    Bitboard attackers = ZERO;

    COLORS enemyColor = (COLORS)!color;

    //Non-sliding
    Bitboard pawnAttackers = AttacksPawns(color, square) & Piece(enemyColor, PAWN);
    attackers |= pawnAttackers;
    attackers |= AttacksKnights(square) & Piece(enemyColor, KNIGHT);
    attackers |= AttacksKing(square) & Piece(enemyColor, KING);

    //Eliminate enemy king for sliding calculation
    Bitboard blockers = m_allpieces ^ Piece(color, KING); //will be attacked after check

    //Bishop
    pawnAttackers |= AttacksPawns(enemyColor, square) & Piece(color, PAWN); //add all pawns (TEST!)
    Bitboard diagonalPieces = Piece(enemyColor, BISHOP) | Piece(enemyColor, QUEEN);
    Bitboard diagonalBlockers = blockers ^ (pawnAttackers | diagonalPieces);
    attackers |= AttacksSliding(BISHOP, square, diagonalBlockers) & diagonalPieces;

    //Rook
    Bitboard straightPieces = Piece(enemyColor, ROOK) | Piece(enemyColor, QUEEN);
    Bitboard straightBlockers = blockers ^ straightPieces;
    attackers |= AttacksSliding(ROOK, square, straightBlockers) & straightPieces;

    return attackers;
}

//Static Exchange Evaluator
int Board::SEE(Move move) {
    
    // retrieve info
    COLORS color = ActivePlayer();
    COLORS enemyColor = InactivePlayer();
    MoveData data = move.Data();

    // list of scores to be filled
    const int MAX_CAPTURES = 32;
    int scores[MAX_CAPTURES];
    int numCapture = 0;

    // direct attackers
    Bitboard attackers = XRayAttackersTo(color, data.toSq) | XRayAttackersTo(enemyColor, data.toSq);

    // handle separately the initial capture
    attackers ^= SquareBB(data.fromSq);

    // simulate the initial capture
    scores[numCapture] = MATERIAL_VALUES[data.capturedType];
    int pieceValue = MATERIAL_VALUES[data.pieceType]; //what is actually on the square

    // switch the active player
    color = (COLORS)!color;
    enemyColor = (COLORS)!enemyColor;

    //the main loop, where all the captures are simulated
    while(attackers) {
        Bitboard activeAttackers = attackers & Piece(color, ALL_PIECES);
        if(!activeAttackers) break;

        //get the Least Valuable Attacker
        PIECE_TYPE pieceType;
        Bitboard LVA = LeastValuableAttacker(activeAttackers, color, pieceType);
        attackers ^= LVA;

        //if king... do smth

        //fill an array of scores / current-piece-on-square
        numCapture++;
        scores[numCapture] = pieceValue - scores[numCapture-1];
        pieceValue = MATERIAL_VALUES[pieceType];

        // switch the active player
        color = (COLORS)!color;
        enemyColor = (COLORS)!enemyColor;
    }

    // add hidden pieces
    //another options is to check for bishops, rooks and queen in the moving direction?? (don't do for kings and knights)

    //retrieve best score
    while(numCapture > 0) {
        if(scores[numCapture] > -scores[numCapture-1]) { //scores[numCapture-1] > -scores[numCapture]
            scores[numCapture-1] = -scores[numCapture];
        }
        numCapture--;
    }

    return scores[0];
}

Bitboard Board::LeastValuableAttacker(Bitboard attackers, COLORS color, PIECE_TYPE& pieceType) {
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

//Operators
bool Board::operator==(const Board& rhs) const {
    bool pieceIntegrity = true;
    for(int color = WHITE; color <= BLACK; color++) {
        for(int piece = PAWN; piece <= KING; piece++) {
            if( this->GetPieces((COLORS)color, (PIECE_TYPE)piece) !=
                rhs.GetPieces((COLORS)color, (PIECE_TYPE)piece) )
            {
                pieceIntegrity = false;
            }
        }
    }
    if( this->m_allpieces != rhs.m_allpieces )
                pieceIntegrity = false;
    
    return pieceIntegrity && 
        this->ActivePlayer() == rhs.ActivePlayer() &&
        this->CastlingRights() == rhs.CastlingRights() &&
        this->EnPassantSquare() == rhs.EnPassantSquare() &&
        this->MoveNumber() == rhs.MoveNumber() &&
        this->Ply() == rhs.Ply() &&
        this->ZKey() == rhs.ZKey();
}

//Private
void Board::AddPiece(int square, COLORS color, PIECE_TYPE pieceType) {
    Bitboard &bb = m_pieces[color][pieceType];
    bb |= SquareBB(square); //add bit, OR
    m_zobristKey.UpdatePiece(color, pieceType, square); //modify the Zobrist key (XOR)
}
void Board::RemovePiece(int square, COLORS color, PIECE_TYPE pieceType) {
    Bitboard &bb = m_pieces[color][pieceType];
    bb ^= SquareBB(square); //remove bit, XOR
    m_zobristKey.UpdatePiece(color, pieceType, square); //modify the Zobrist key (XOR)
}
void Board::MovePiece(int fromSq, int toSq, COLORS color, PIECE_TYPE pieceType) {
    AddPiece(toSq, color, pieceType);
    RemovePiece(fromSq, color, pieceType);
}

void Board::ClearBits() {
    for(int color = WHITE; color <= BLACK; color++) {
        m_attackedSquares[color] = 0;
        m_pinnedPieces[color] = 0;
        m_kingAttackers[color] = 0;
        m_kingDangerSquares[color] = 0;

        for(int pieceType = PAWN; pieceType <= KING; pieceType++) {
            m_pieces[color][pieceType] = 0;
        }
    }

    for(int i = 0; i < MAX_PLIES; ++i) {
        m_history[i].Clear();
    }

    for(int i = 0; i < 63; ++i) {
        m_pinnedPushMask[i] = 0;
        m_pinnedCaptureMask[i] = 0;
    }

    m_enPassantSquare = ZERO;
    m_castlingRights = 0;

    m_activePlayer = WHITE;
    m_moveNumber = 1;
    m_ply = 0;
    m_fiftyrule = 0;

    UpdateBitboards();
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

void Board::UpdateCastlingRights(const Move& move) {
    if( !CastlingRights() || move.MoveType() == NULLMOVE ) {
        assert(move.MoveType() != CASTLING);
        return;
    }

    COLORS color = ActivePlayer();
    MOVE_TYPE moveType = move.MoveType();
    PIECE_TYPE pieceType = move.PieceType();
    PIECE_TYPE capturedType = move.CapturedType();
    int toSq = move.ToSq();
    int fromSq = move.FromSq();


    //King castles
    if(moveType == CASTLING) {
        switch(toSq) {
        case G1: {
            MovePiece(H1, F1, color, ROOK);
            RemoveCastlingRights(CASTLING_K);
            RemoveCastlingRights(CASTLING_Q);
        } break;
        case C1: {
            MovePiece(A1, D1, color, ROOK);
            RemoveCastlingRights(CASTLING_K);
            RemoveCastlingRights(CASTLING_Q);
        } break;
        case G8: {
            MovePiece(H8, F8, color, ROOK);
            RemoveCastlingRights(CASTLING_k);
            RemoveCastlingRights(CASTLING_q);
        } break;
        case C8: {
            MovePiece(A8, D8, color, ROOK);
            RemoveCastlingRights(CASTLING_k);
            RemoveCastlingRights(CASTLING_q);
        } break;
        default: assert(false);
        };
    }

    //Rook moves
    if(pieceType == ROOK) {
        switch(fromSq) {
            case H1: RemoveCastlingRights(CASTLING_K); break;
            case A1: RemoveCastlingRights(CASTLING_Q); break;
            case H8: RemoveCastlingRights(CASTLING_k); break;
            case A8: RemoveCastlingRights(CASTLING_q); break;
            default: break;
        };
    }
    //King moves
    else if(pieceType == KING && moveType != CASTLING) {
        if(color == WHITE) {
            RemoveCastlingRights(CASTLING_K);
            RemoveCastlingRights(CASTLING_Q);
        } else if(color == BLACK) {
            RemoveCastlingRights(CASTLING_k);
            RemoveCastlingRights(CASTLING_q);
        }
    }

    //Rook is captured
    if(capturedType == ROOK) {
        switch(toSq) {
            case H1: RemoveCastlingRights(CASTLING_K); break;
            case A1: RemoveCastlingRights(CASTLING_Q); break;
            case H8: RemoveCastlingRights(CASTLING_k); break;
            case A8: RemoveCastlingRights(CASTLING_q); break;
            default: break;
        };
    }
}

void Board::RewindCastlingRights(const Move& move) {

    if(move.MoveType() == CASTLING) {
        COLORS color = ActivePlayer();
        switch(move.ToSq()) {
            case G1: MovePiece(F1, H1, color, ROOK); break;
            case C1: MovePiece(D1, A1, color, ROOK); break;
            case G8: MovePiece(F8, H8, color, ROOK); break;
            case C8: MovePiece(D8, A8, color, ROOK); break;
            default: assert(false);
        };
    }

    //Update the zkeys
    U8 changedBits = m_castlingRights ^ m_history[m_ply].castling;
    if(changedBits & 0b0001) m_zobristKey.UpdateCastling(CASTLING_K);
    if(changedBits & 0b0010) m_zobristKey.UpdateCastling(CASTLING_Q);
    if(changedBits & 0b0100) m_zobristKey.UpdateCastling(CASTLING_k);
    if(changedBits & 0b1000) m_zobristKey.UpdateCastling(CASTLING_q);

    //Retrieve castling rights
    m_castlingRights = m_history[m_ply].castling;
}

void Board::UpdateKingAttackers(COLORS color) {
    Bitboard theKing = GetPieces(color, KING);
    int kingSquare = BitscanForward(theKing);

    m_kingAttackers[color] = AttackersTo(color, kingSquare);
}

bool Board::CheckIntegrity() const {
    return PopCount( Piece(WHITE, KING) ) == 1
             && PopCount( Piece(BLACK, KING) ) == 1
             && PopCount( Piece(WHITE, PAWN) ) <= 8
             && PopCount( Piece(BLACK, PAWN) ) <= 8
             && PopCount( EnPassantSquare() ) <= 1
             && Ply() <= MAX_PLIES
             && ActivePlayer() != NO_COLOR
        ;
}
