#include "MoveGenerator.h"
#include "Attacks.h"
#include "BitboardUtils.h"
#include "Board.h"
using namespace Attacks;
using namespace BitboardUtils;

#include <algorithm>
#include <cstdlib> //random
#include <iostream>

#ifdef _MSC_VER
	#include <time.h>
#endif

MoveGenerator::MoveGenerator() { }

//Legal moves
MoveList MoveGenerator::GenerateMoves(Board &board) {
    //Attacked squares
    COLORS color = board.ActivePlayer();
    COLORS enemyColor = board.InactivePlayer();
    board.m_kingDangerSquares[enemyColor] = GenerateKingDangerAttacks(board, enemyColor);
    board.m_attackedSquares[enemyColor] = board.m_kingDangerSquares[enemyColor] & ~board.Piece(enemyColor,ALL_PIECES);

    // Initialize the capture and push masks
    board.m_captureMask = ALL;
    board.m_pushMask = ALL;
    //Pinned pieces
    for(int i = 0; i < 64; ++i) {
        board.m_pinnedPushMask[i] = ALL;
        board.m_pinnedCaptureMask[i] = ALL;
    }
    board.m_pinnedPieces[color] = PinnedPieces(board, color);

    if(board.IsCheck()) {
        GenerateEvasionMoves(board);
    } else {
        GeneratePseudoMoves(board);
    }

    return m_moves;
}

MoveList MoveGenerator::GeneratePseudoMoves(Board &board) {
    m_moves.reserve(MAX_MOVES_RESERVE);

    GenerateKingMoves(board);
    GenerateKnightMoves(board);
    GeneratePawnMoves(board);

    COLORS color = board.ActivePlayer();
    board.m_kingDangerSquares[color] = board.m_attackedSquares[color];

    GenerateSlidingMoves(BISHOP, board);
    GenerateSlidingMoves(ROOK, board);
    GenerateSlidingMoves(QUEEN, board);

    return m_moves;
}

MoveList MoveGenerator::GenerateEvasionMoves(Board &board) {
    m_moves.reserve(MAX_MOVES_RESERVE);

    COLORS color = board.ActivePlayer();
    Bitboard checkers = board.m_kingAttackers[color];

    int numCheckers = PopCount(checkers);
    if(numCheckers == 1) {
        //Create the capture mask
        board.m_captureMask = checkers;

        int attackerSq = BitscanForward(checkers);
        PIECE_TYPE checkerType = board.GetPieceAtSquare(board.InactivePlayer(), attackerSq);
        switch(checkerType) {
            case PAWN: case KNIGHT: {
                board.m_pushMask = ZERO; //only capture solves the check
                GeneratePseudoMoves(board);
            } break;
            case BISHOP: case ROOK: case QUEEN: {
                //Create the block mask
                Bitboard theKing = board.Piece(color, KING);
                int kingSq = BitscanForward(theKing);
                board.m_pushMask = Attacks::Between(attackerSq, kingSq);

                // board.m_pushMask = ALL; //oponent slider rays to square... (board)

                GeneratePseudoMoves(board);
            } break;
            default: assert(false);
        };
    } else if(numCheckers > 1) {
        GenerateKingMoves(board);
    }

    return m_moves;
}

Move MoveGenerator::RandomMove() {
    int min = 0;
    size_t max = m_moves.size();
    
    srand( (uint)time(nullptr) );

    int randomIndex = (rand() % max) + min;

    return m_moves[randomIndex];
}

void MoveGenerator::GeneratePawnMoves(Board &board) {
    if(board.ActivePlayer() == WHITE) {
        GenerateWhitePawnMoves(board);
    } else {
        GenerateBlackPawnMoves(board);
    }
}
void MoveGenerator::GenerateWhitePawnMoves(Board &board) {
    PIECE_TYPE piece = PAWN;

    Bitboard thePawns = board.GetPieces( board.ActivePlayer(), piece );
    Bitboard enemyPieces = board.GetPieces( board.InactivePlayer(), ALL_PIECES );
    Bitboard allPieces = board.m_allpieces;

    Bitboard oneStep =  North(thePawns) & ~allPieces;
    Bitboard twoSteps = North(oneStep & MaskRank[RANK3]) & ~allPieces & board.m_pushMask;
    Bitboard attackLeft = ( (thePawns & ClearFile[FILEA]) << 7) & enemyPieces & board.m_captureMask;
    Bitboard attackRight = ( (thePawns & ClearFile[FILEH]) << 9) & enemyPieces & board.m_captureMask;
    Bitboard promotions = (oneStep & MaskRank[RANK8]) & board.m_pushMask;
    Bitboard promotionsLeft = attackLeft & MaskRank[RANK8];
    Bitboard promotionsRight = attackRight & MaskRank[RANK8];
    Bitboard enpassantLeft = ( (thePawns & ClearFile[FILEA]) << 7) & board.m_enPassantSquare;
    Bitboard enpassantRight = ( (thePawns & ClearFile[FILEH]) << 9) & board.m_enPassantSquare;

    oneStep &= ClearRank[RANK8] & board.m_pushMask;
    attackLeft &= ClearRank[RANK8];
    attackRight &= ClearRank[RANK8];

    bool enpassantEvasion = (board.m_captureMask & South(board.m_enPassantSquare)) || (board.m_pushMask & board.m_enPassantSquare);
    enpassantLeft *= enpassantEvasion;
    enpassantRight *= enpassantEvasion;

    while(oneStep) {
        int toSq = ResetLsb(oneStep);
        int fromSq = toSq - 8;
        Bitboard toBitboard = ONE << toSq;
        toBitboard &= board.m_pinnedPushMask[fromSq];
        AddPawnPushMoves(board, fromSq, toBitboard, MOVE_TYPE::NORMAL);
    }
    while(twoSteps) {
        int toSq = ResetLsb(twoSteps);
        int fromSq = toSq - 16;
        Bitboard toBitboard = ONE << toSq;
        toBitboard &= board.m_pinnedPushMask[fromSq];
        AddPawnPushMoves(board, fromSq, toBitboard, MOVE_TYPE::DOUBLE_PUSH);
    }
    while(attackLeft) {
        int toSq = ResetLsb(attackLeft);
        int fromSq = toSq - 7;
        Bitboard toBitboard = ONE << toSq;
        toBitboard &= board.m_pinnedCaptureMask[fromSq];
        AddMoves(board, piece, fromSq, toBitboard, enemyPieces);
    }
    while(attackRight) {
        int toSq = ResetLsb(attackRight);
        int fromSq = toSq - 9;
        Bitboard toBitboard = ONE << toSq;
        toBitboard &= board.m_pinnedCaptureMask[fromSq];
        AddMoves(board, piece, fromSq, toBitboard, enemyPieces);
    }
    while(promotions) {
        int toSq = ResetLsb(promotions);
        int fromSq = toSq - 8;
        Bitboard toBitboard = ONE << toSq;
        toBitboard &= board.m_pinnedPushMask[fromSq];
        AddPromotionMoves(board, fromSq, toBitboard);
    }
    while(promotionsLeft) {
        int toSq = ResetLsb(promotionsLeft);
        int fromSq = toSq - 7;
        Bitboard toBitboard = ONE << toSq;
        toBitboard &= board.m_pinnedCaptureMask[fromSq];
        AddPromotionMoves(board, fromSq, toBitboard);
    }
    while(promotionsRight) {
        int toSq = ResetLsb(promotionsRight);
        int fromSq = toSq - 9;
        Bitboard toBitboard = ONE << toSq;
        toBitboard &= board.m_pinnedCaptureMask[fromSq];
        AddPromotionMoves(board, fromSq, toBitboard);
    }
    while(enpassantLeft) {
        int toSq = ResetLsb(enpassantLeft);
        int fromSq = toSq - 7;
        Bitboard toBitboard = ONE << toSq;
        Bitboard theChecker = South(board.m_enPassantSquare);
        bool pinnedMask = (board.m_pinnedCaptureMask[fromSq] & theChecker) || (board.m_pinnedPushMask[fromSq] & board.m_enPassantSquare);
        toBitboard *= pinnedMask;

        COLORS color = board.ActivePlayer();
        Bitboard blockers = board.m_allpieces ^ SquareBB(fromSq) ^ theChecker;
        if( board.AttackersTo( color, BitscanForward(board.Piece(color,KING)), blockers ) & ~theChecker )
            continue;

        AddPawnPushMoves(board, fromSq, toBitboard, MOVE_TYPE::ENPASSANT);
    }
    while(enpassantRight) {
        int toSq = ResetLsb(enpassantRight);
        int fromSq = toSq - 9;
        Bitboard toBitboard = ONE << toSq;
        Bitboard theChecker = South(board.m_enPassantSquare);
        bool pinnedMask = (board.m_pinnedCaptureMask[fromSq] & theChecker) || (board.m_pinnedPushMask[fromSq] & board.m_enPassantSquare);
        toBitboard *= pinnedMask;

        COLORS color = board.ActivePlayer();
        Bitboard blockers = board.m_allpieces ^ SquareBB(fromSq) ^ theChecker;
        if( board.AttackersTo( color, BitscanForward(board.Piece(color,KING)), blockers ) & ~theChecker )
            continue;

        AddPawnPushMoves(board, fromSq, toBitboard, MOVE_TYPE::ENPASSANT);
    }
}

void MoveGenerator::GenerateBlackPawnMoves(Board &board) {
    PIECE_TYPE piece = PAWN;
    
    Bitboard thePawns = board.GetPieces( board.ActivePlayer(), piece );
    Bitboard enemyPieces = board.GetPieces( board.InactivePlayer(), ALL_PIECES );
    Bitboard allPieces = board.m_allpieces;

    Bitboard oneStep = South(thePawns) & ~allPieces;
    Bitboard twoSteps = South(oneStep & MaskRank[RANK6]) & ~allPieces & board.m_pushMask;
    Bitboard attackLeft = ( (thePawns & ClearFile[FILEH]) >> 7) & enemyPieces & board.m_captureMask;
    Bitboard attackRight = ( (thePawns & ClearFile[FILEA]) >> 9) & enemyPieces & board.m_captureMask;
    Bitboard promotions = (oneStep & MaskRank[RANK1]) & board.m_pushMask;
    Bitboard promotionsLeft = attackLeft & MaskRank[RANK1];
    Bitboard promotionsRight = attackRight & MaskRank[RANK1];
    Bitboard enpassantLeft = ( (thePawns & ClearFile[FILEH]) >> 7) & board.m_enPassantSquare;
    Bitboard enpassantRight = ( (thePawns & ClearFile[FILEA]) >> 9) & board.m_enPassantSquare;

    oneStep &= ClearRank[RANK1] & board.m_pushMask;
    attackLeft &= ClearRank[RANK1];
    attackRight &= ClearRank[RANK1];

    bool enpassantEvasion = (board.m_captureMask & North(board.m_enPassantSquare)) || (board.m_pushMask & board.m_enPassantSquare);
    enpassantLeft *= enpassantEvasion;
    enpassantRight *= enpassantEvasion;

    while(oneStep) {
        int toSq = ResetLsb(oneStep);
        int fromSq = toSq + 8;
        Bitboard toBitboard = ONE << toSq;
        toBitboard &= board.m_pinnedPushMask[fromSq];
        AddPawnPushMoves(board, fromSq, toBitboard, MOVE_TYPE::NORMAL);
    }
    while(twoSteps) {
        int toSq = ResetLsb(twoSteps);
        int fromSq = toSq + 16;
        Bitboard toBitboard = ONE << toSq;
        toBitboard &= board.m_pinnedPushMask[fromSq];
        AddPawnPushMoves(board, fromSq, toBitboard, MOVE_TYPE::DOUBLE_PUSH);
    }
    while(attackLeft) {
        int toSq = ResetLsb(attackLeft);
        int fromSq = toSq + 7;
        Bitboard toBitboard = ONE << toSq;
        toBitboard &= board.m_pinnedCaptureMask[fromSq];
        AddMoves(board, piece, fromSq, toBitboard, enemyPieces);
    }
    while(attackRight) {
        int toSq = ResetLsb(attackRight);
        int fromSq = toSq + 9;
        Bitboard toBitboard = ONE << toSq;
        toBitboard &= board.m_pinnedCaptureMask[fromSq];
        AddMoves(board, piece, fromSq, toBitboard, enemyPieces);
    }
    while(promotions) {
        int toSq = ResetLsb(promotions);
        int fromSq = toSq + 8;
        Bitboard toBitboard = ONE << toSq;
        toBitboard &= board.m_pinnedPushMask[fromSq];
        AddPromotionMoves(board, fromSq, toBitboard);
    }
    while(promotionsLeft) {
        int toSq = ResetLsb(promotionsLeft);
        int fromSq = toSq + 7;
        Bitboard toBitboard = ONE << toSq;
        toBitboard &= board.m_pinnedCaptureMask[fromSq];
        AddPromotionMoves(board, fromSq, toBitboard);
    }
    while(promotionsRight) {
        int toSq = ResetLsb(promotionsRight);
        int fromSq = toSq + 9;
        Bitboard toBitboard = ONE << toSq;
        toBitboard &= board.m_pinnedCaptureMask[fromSq];
        AddPromotionMoves(board, fromSq, toBitboard);
    }
    while(enpassantLeft) {
        int toSq = ResetLsb(enpassantLeft);
        int fromSq = toSq + 7;
        Bitboard toBitboard = ONE << toSq;
        Bitboard theChecker = North(board.m_enPassantSquare);
        bool pinnedMask = (board.m_pinnedCaptureMask[fromSq] & theChecker) || (board.m_pinnedPushMask[fromSq] & board.m_enPassantSquare);
        toBitboard *= pinnedMask;

        COLORS color = board.ActivePlayer();
        Bitboard blockers = board.m_allpieces ^ SquareBB(fromSq) ^ theChecker;
        if( board.AttackersTo( color, BitscanForward(board.Piece(color,KING)), blockers ) & ~theChecker )
            continue;

        AddPawnPushMoves(board, fromSq, toBitboard, MOVE_TYPE::ENPASSANT);
    }
    while(enpassantRight) {
        int toSq = ResetLsb(enpassantRight);
        int fromSq = toSq + 9;
        Bitboard toBitboard = ONE << toSq;
        Bitboard theChecker = North(board.m_enPassantSquare);
        bool pinnedMask = (board.m_pinnedCaptureMask[fromSq] & theChecker) || (board.m_pinnedPushMask[fromSq] & board.m_enPassantSquare);
        toBitboard *= pinnedMask;

        COLORS color = board.ActivePlayer();
        Bitboard blockers = board.m_allpieces ^ SquareBB(fromSq) ^ theChecker;
        if( board.AttackersTo( color, BitscanForward(board.Piece(color,KING)), blockers ) & ~theChecker )
            continue;
        
        AddPawnPushMoves(board, fromSq, toBitboard, MOVE_TYPE::ENPASSANT);
    }
}

void MoveGenerator::GenerateKnightMoves(Board &board) {
    PIECE_TYPE piece = KNIGHT;
    Bitboard thePieces = board.GetPieces( board.ActivePlayer(), piece );
    Bitboard ownPieces = board.GetPieces( board.ActivePlayer(), ALL_PIECES );
    Bitboard enemyPieces = board.GetPieces( board.InactivePlayer(), ALL_PIECES );

    while(thePieces) {
        int fromSq = ResetLsb(thePieces);
        Bitboard attacks = AttacksKnights(fromSq);
        attacks &= ~ownPieces;

        if( board.m_pinnedPieces[board.ActivePlayer()] & SquareBB(fromSq) )
            continue;

        AddMoves(board, piece, fromSq, attacks, enemyPieces);
    }
}
void MoveGenerator::GenerateKingMoves(Board &board) {
    PIECE_TYPE piece = KING;
    Bitboard theKing = board.GetPieces( board.ActivePlayer(), piece );
    Bitboard ownPieces = board.GetPieces( board.ActivePlayer(), ALL_PIECES );
    Bitboard enemyPieces = board.GetPieces( board.InactivePlayer(), ALL_PIECES );

    //Exit if no king on the board
    assert(theKing);

    //Attacks
    int fromSq = BitscanForward(theKing);
    Bitboard attacks = AttacksKing(fromSq);
    attacks &= ~ownPieces;

    //Evade attacked squares
    attacks &= ~board.m_kingDangerSquares[ board.InactivePlayer() ];

    //Moves
    AddMoves(board, piece, fromSq, attacks, enemyPieces);

    //Castling. Don't generate in evasion
    if(board.m_pushMask == ALL)
        AddCastlingMoves(board);
}
void MoveGenerator::GenerateSlidingMoves(PIECE_TYPE pieceType, Board &board) {
    COLORS color = board.ActivePlayer();
    Bitboard thePieces = board.GetPieces( color, pieceType );
    Bitboard ownPieces = board.GetPieces( color, ALL_PIECES );
    Bitboard enemyPieces = board.GetPieces( board.InactivePlayer(), ALL_PIECES );

    while(thePieces) {
        //Attacks
        int fromSq = ResetLsb(thePieces);
        Bitboard attacks = AttacksSliding(pieceType, fromSq, board.m_allpieces);
        attacks &= ~ownPieces;
        attacks &= board.m_pinnedPushMask[fromSq] | board.m_pinnedCaptureMask[fromSq];

        AddMoves(board, pieceType, fromSq, attacks, enemyPieces);
    }
}

void MoveGenerator::AddMoves(Board &board, PIECE_TYPE piece, int fromSq, Bitboard possibleMoves, Bitboard enemyPieces) {

    // ===================
    // == Capture moves ==
    // ===================
    Bitboard captureMoves = possibleMoves & enemyPieces;
    if(piece != KING) {
        captureMoves &= board.m_captureMask;
    }
    while(captureMoves) {
        int toSq = ResetLsb(captureMoves);
        PIECE_TYPE capturedPiece = board.GetPieceAtSquare( board.InactivePlayer(), toSq);
        Move move = Move(fromSq, toSq, piece, MOVE_TYPE::CAPTURE);
        move.SetCapturedType(capturedPiece);
        m_moves.push_back( move );
    }

    // =================================
    // == Normal moves (non-captures) ==
    // =================================
    Bitboard normalMoves = possibleMoves & ~enemyPieces;
    if(piece != KING) {
        normalMoves &= board.m_pushMask;
    }
    while(normalMoves) {
        int toSq = ResetLsb(normalMoves);
        Move move = Move(fromSq, toSq, piece, MOVE_TYPE::NORMAL);
        m_moves.push_back( move );
    }
    
}
void MoveGenerator::AddPawnPushMoves(Board &board, int fromSq, Bitboard possibleMoves, MOVE_TYPE moveType) {
    while(possibleMoves) {
        int toSq = ResetLsb(possibleMoves);
        Move move = Move(fromSq, toSq, PIECE_TYPE::PAWN, moveType);
        m_moves.push_back(move);
    }
}
void MoveGenerator::AddPromotionMoves(Board &board, int fromSq, Bitboard promotionMoves) {

    while(promotionMoves) {
        int toSq = ResetLsb(promotionMoves);
        PIECE_TYPE capturedPiece = board.GetPieceAtSquare( board.InactivePlayer(), toSq);

        Move move;
        if(capturedPiece) { //PROMOTION_CAPTURE
            move = Move(fromSq, toSq, PIECE_TYPE::PAWN, MOVE_TYPE::PROMOTION_CAPTURE);
            move.SetCapturedType(capturedPiece);
        } else { //PROMOTION
            move = Move(fromSq, toSq, PIECE_TYPE::PAWN, MOVE_TYPE::PROMOTION);
        }

        move.SetPromotionFlag(PROMOTION_QUEEN);
        m_moves.push_back(move);

        move.SetPromotionFlag(PROMOTION_KNIGHT);
        m_moves.push_back(move);

        move.SetPromotionFlag(PROMOTION_ROOK);
        m_moves.push_back(move);

        move.SetPromotionFlag(PROMOTION_BISHOP);
        m_moves.push_back(move);
    }
}
void MoveGenerator::AddCastlingMoves(Board &board) {
    COLORS color = board.ActivePlayer();
    U8 castlingRights = board.CastlingRights();
    int fromSq = 0, toSq = 0;

    if(color == WHITE) {
        //WHITE CASTLING KING
        if(castlingRights & CASTLING_K 
            && !(board.m_allpieces & ( (ONE << F1) | (ONE << G1) ) )
            && !board.IsAttacked(color, E1)
            && !board.IsAttacked(color, F1)
            && !board.IsAttacked(color, G1)
        ) {
            fromSq = E1; toSq = G1;
            Move move = Move(fromSq, toSq, PIECE_TYPE::KING, MOVE_TYPE::CASTLING);
            m_moves.push_back(move);
        }
        //WHITE CASTLING QUEEN
        if(castlingRights & CASTLING_Q 
            && !(board.m_allpieces & ( (ONE << B1) | (ONE << C1) | (ONE << D1) ) )
            && !board.IsAttacked(color, E1)
            && !board.IsAttacked(color, D1)
            && !board.IsAttacked(color, C1)
        ) {
            fromSq = E1; toSq = C1;
            Move move = Move(fromSq, toSq, PIECE_TYPE::KING, MOVE_TYPE::CASTLING);
            m_moves.push_back(move);
        }
    } else {
        //BLACK CASTLING KING
        if(castlingRights & CASTLING_k 
            && !(board.m_allpieces & ( (ONE << F8) | (ONE << G8) ) )
            && !board.IsAttacked(color, E8)
            && !board.IsAttacked(color, F8)
            && !board.IsAttacked(color, G8)
        ) {
            fromSq = E8; toSq = G8;
            Move move = Move(fromSq, toSq, PIECE_TYPE::KING, MOVE_TYPE::CASTLING);
            m_moves.push_back(move);
        }
        //BLACK CASTLING QUEEN
        if(castlingRights & CASTLING_q 
            && !(board.m_allpieces & ( (ONE << B8) | (ONE << C8) | (ONE << D8) ) )
            && !board.IsAttacked(color, E8)
            && !board.IsAttacked(color, D8)
            && !board.IsAttacked(color, C8)
        ) {
            fromSq = E8; toSq = C8;
            Move move = Move(fromSq, toSq, PIECE_TYPE::KING, MOVE_TYPE::CASTLING);
            m_moves.push_back(move);
        }
    }
}

Bitboard MoveGenerator::GenerateAttacks(Board &board, COLORS color) {
    Bitboard ownPieces = board.Piece(color,ALL_PIECES);
    return GenerateKingDangerAttacks(board, color) & ~ownPieces;
}

Bitboard MoveGenerator::GenerateKingDangerAttacks(Board &board, COLORS color) {
    Bitboard attacks = ZERO;
    Bitboard blockers = board.m_allpieces ^ board.Piece((COLORS)!color, KING);
    for(PIECE_TYPE pieceType = PAWN; pieceType <= KING; ++pieceType) {
        Bitboard bb = board.Piece(color, pieceType);
        while(bb) {
            int square = ResetLsb(bb);
            switch(pieceType) {
                case PAWN: attacks |= AttacksPawns(color, square); break;
                case KNIGHT: attacks |= AttacksKnights(square); break;
                case KING: attacks |= AttacksKing(square); break;
                case BISHOP: attacks |= AttacksSliding(BISHOP, square, blockers); break;
                case ROOK: attacks |= AttacksSliding(ROOK, square, blockers); break;
                case QUEEN: attacks |= AttacksSliding(QUEEN, square, blockers); break;
                default: assert(false);
            };
        }
    }
    return attacks;
}

//Step 1: Generate enemy sliding attacks, in the same direction as our king
//Step 2: Generate attacks from our king (as the same sliding pieceType)
//Step 3: A pinned piece would appear in both bitboards
Bitboard MoveGenerator::PinnedPieces(Board &board, COLORS color) {
    Bitboard pinned = ZERO;
    int kingSquare = BitscanForward( board.Piece(color, KING) );

    for(PIECE_TYPE pieceType = BISHOP; pieceType <= QUEEN; ++pieceType) {
        Bitboard bb = board.Piece((COLORS)!color, pieceType);
        while(bb) {
            int square = ResetLsb(bb);
            switch(pieceType) {
                case BISHOP: {
                    pinned |= FillPinned(board, color, BISHOP, square, kingSquare);
                } break;
                case ROOK: {
                    pinned |= FillPinned(board, color, ROOK, square, kingSquare);
                } break;
                case QUEEN: {
                    pinned |= FillPinned(board, color, BISHOP, square, kingSquare);
                    pinned |= FillPinned(board, color, ROOK, square, kingSquare);
                } break;
                default: break;
            };
        }
    }
    return pinned;
}

Bitboard MoveGenerator::FillPinned(Board& board, COLORS color, PIECE_TYPE slidingType, int square, int kingSquare) {
    assert(slidingType == BISHOP || slidingType == ROOK);

    Bitboard pinned = ZERO;
    DIRECTIONS direction;
    if(Attacks::IsInDirection(slidingType, square, kingSquare, direction)) {
        Bitboard inBetween = Attacks::Between(square, kingSquare);
        Bitboard attacksFromEnemy = Attacks::AttacksSliding(slidingType, square, board.m_allpieces);
        Bitboard attacksFromKing = Attacks::AttacksSliding(slidingType, kingSquare, board.m_allpieces);
        pinned = attacksFromEnemy & attacksFromKing & board.Piece(color, ALL_PIECES);
        if(pinned) {
            assert(PopCount(pinned) == 1);
            board.m_pinnedPushMask[BitscanForward(pinned)] = inBetween;
            board.m_pinnedCaptureMask[BitscanForward(pinned)] = SquareBB(square);
        }
    }
    return pinned;
}
