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

/*
Magic bitboards:
https://rhysre.net/2019/01/15/magic-bitboards.html (didactic)
*/

//#define PERFT_VERBOSE

MoveGenerator::MoveGenerator() { }

int count;
std::map<std::string, int> countMap;

//Legal moves
MoveList MoveGenerator::GenerateMoves(Board &board) {
    // return GeneratePseudoMoves(board); //uncomment for testing

    COLORS color = board.ActivePlayer();

    // board.UpdateKingAttackers(color);
    bool isCheck = board.IsCheck(color);

    // ===========================================
    // == Initialize the capture and push masks ==
    // ===========================================
    board.m_captureMask = ALL;
    board.m_pushMask = ALL;

    if(isCheck) {
        //TODO: Evasion moves
        GenerateEvasionMoves(board);
    } else {
        GeneratePseudoMoves(board);
    }

    m_moves.erase( std::remove_if(
        m_moves.begin(),
        m_moves.end(),
        [&](const Move theMove)-> bool {
            return !board.IsMoveLegal(theMove, isCheck);
        }
    ), m_moves.end() );

    #ifdef PERFT_VERBOSE
    for(auto move : m_moves) {
        if(move.MoveType() == CAPTURE) //move.MoveType() == PROMOTION_CAPTURE
            countMap["captures"]++;
        if(move.IsPromotion() ) { countMap["promotions"]++; }
        if(move.MoveType() == ENPASSANT) countMap["enpassants"]++;
        if(move.MoveType() == CASTLING) countMap["castlings"]++;
        if(true) {
            board.MakeMove(move);
            COLORS color = board.ActivePlayer();
            // board.UpdateKingAttackers(color);
            bool isCheck = board.IsCheck(color);
            if(isCheck) {
                countMap["checks"]++;
            }
            board.TakeMove(move);
        }
        if(true) {
            board.MakeMove(move);
            COLORS color = board.ActivePlayer();
            // board.UpdateKingAttackers(color);
            bool isCheck = board.IsCheck(color);
            MoveGenerator gen;
            if(isCheck && gen.GenerateMoves(board).size() == 0) {
                countMap["checkmates"]++;
            }
            board.TakeMove(move);
        }
    }
    #endif

    return m_moves;
}

MoveList MoveGenerator::GeneratePseudoMoves(Board &board) {
    m_moves.reserve(MAX_MOVES_RESERVE);

    GenerateKingMoves(board);
    GenerateKnightMoves(board);
    GeneratePawnMoves(board);

    GenerateSlidingMoves(BISHOP, board);
    GenerateSlidingMoves(ROOK, board);
    GenerateSlidingMoves(QUEEN, board);

    return m_moves;
}

MoveList MoveGenerator::GenerateEvasionMoves(Board &board) {
    m_moves.reserve(MAX_MOVES_RESERVE);

    COLORS color = board.ActivePlayer();
    Bitboard attackers = board.m_kingAttackers[color];

    int numAttackers = PopCount(attackers);
    if(numAttackers == 1) {
        //Create the capture mask
        board.m_captureMask = attackers;

        int attackerSq = BitscanForward(attackers);
        PIECE_TYPE checkerType = board.GetPieceAtSquare(board.InactivePlayer(), attackerSq);
        switch(checkerType) {
            case PAWN: case KNIGHT: {
                board.m_pushMask = ZERO; //only capture solves the check
                GeneratePseudoMoves(board);
            } break;
            case BISHOP: case ROOK: case QUEEN: {
                //Create the block mask
                Bitboard theKing = board.GetPieces(color, KING);
                int kingSq = BitscanForward(theKing);
                board.m_pushMask = Attacks::Between(attackerSq, kingSq);

                // board.m_pushMask = ALL; //oponent slider rays to square... (board)

                GeneratePseudoMoves(board);
            } break;
            default: assert(false);
        };
    } else if(numAttackers > 1) {
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

    Bitboard thePieces = board.GetPieces( board.ActivePlayer(), piece );
    Bitboard enemyPieces = board.GetPieces( board.InactivePlayer(), ALL_PIECES );
    Bitboard allPieces = board.m_allpieces;

    Bitboard oneStep =  North(thePieces) & ~allPieces;
    Bitboard twoSteps = North(oneStep & MaskRank[RANK3]) & ~allPieces;
    Bitboard attackLeft = ( (thePieces & ClearFile[FILEA]) << 7) & enemyPieces & board.m_captureMask;
    Bitboard attackRight = ( (thePieces & ClearFile[FILEH]) << 9) & enemyPieces & board.m_captureMask;
    Bitboard promotions = (oneStep & MaskRank[RANK8]);
    Bitboard promotionsLeft = attackLeft & MaskRank[RANK8];
    Bitboard promotionsRight = attackRight & MaskRank[RANK8];
    Bitboard enpassantLeft = ( (thePieces & ClearFile[FILEA]) << 7) & board.m_enPassantSquare; //fix
    Bitboard enpassantRight = ( (thePieces & ClearFile[FILEH]) << 9) & board.m_enPassantSquare; //fix

    oneStep &= ClearRank[RANK8];
    attackLeft &= ClearRank[RANK8];
    attackRight &= ClearRank[RANK8];

    while(oneStep) {
        int toSq = ResetLsb(oneStep);
        int fromSq = toSq - 8;
        Bitboard toBitboard = ONE << toSq;
        AddPawnPushMoves(board, fromSq, toBitboard, MOVE_TYPE::NORMAL);
    }
    while(twoSteps) {
        int toSq = ResetLsb(twoSteps);
        int fromSq = toSq - 16;
        Bitboard toBitboard = ONE << toSq;
        AddPawnPushMoves(board, fromSq, toBitboard, MOVE_TYPE::DOUBLE_PUSH);
    }
    while(attackLeft) {
        int toSq = ResetLsb(attackLeft);
        int fromSq = toSq - 7;
        Bitboard toBitboard = ONE << toSq;
        AddMoves(board, piece, fromSq, toBitboard, enemyPieces);
    }
    while(attackRight) {
        int toSq = ResetLsb(attackRight);
        int fromSq = toSq - 9;
        Bitboard toBitboard = ONE << toSq;
        AddMoves(board, piece, fromSq, toBitboard, enemyPieces);
    }
    while(promotions) {
        int toSq = ResetLsb(promotions);
        int fromSq = toSq - 8;
        Bitboard toBitboard = ONE << toSq;
        AddPromotionMoves(board, fromSq, toBitboard);
    }
    while(promotionsLeft) {
        int toSq = ResetLsb(promotionsLeft);
        int fromSq = toSq - 7;
        Bitboard toBitboard = ONE << toSq;
        AddPromotionMoves(board, fromSq, toBitboard);
    }
    while(promotionsRight) {
        int toSq = ResetLsb(promotionsRight);
        int fromSq = toSq - 9;
        Bitboard toBitboard = ONE << toSq;
        AddPromotionMoves(board, fromSq, toBitboard);
    }
    while(enpassantLeft) {
        int toSq = ResetLsb(enpassantLeft);
        int fromSq = toSq - 7;
        Bitboard toBitboard = ONE << toSq;
        AddPawnPushMoves(board, fromSq, toBitboard, MOVE_TYPE::ENPASSANT);
    }
    while(enpassantRight) {
        int toSq = ResetLsb(enpassantRight);
        int fromSq = toSq - 9;
        Bitboard toBitboard = ONE << toSq;
        AddPawnPushMoves(board, fromSq, toBitboard, MOVE_TYPE::ENPASSANT);
    }
}

void MoveGenerator::GenerateBlackPawnMoves(Board &board) {
    PIECE_TYPE piece = PAWN;
    
    Bitboard thePieces = board.GetPieces( board.ActivePlayer(), piece );
    Bitboard enemyPieces = board.GetPieces( board.InactivePlayer(), ALL_PIECES );
    Bitboard allPieces = board.m_allpieces;

    Bitboard oneStep = South(thePieces) & ~allPieces;
    Bitboard twoSteps = South(oneStep & MaskRank[RANK6]) & ~allPieces;
    Bitboard attackLeft = ( (thePieces & ClearFile[FILEH]) >> 7) & enemyPieces & board.m_captureMask;
    Bitboard attackRight = ( (thePieces & ClearFile[FILEA]) >> 9) & enemyPieces & board.m_captureMask;
    Bitboard promotions = (oneStep & MaskRank[RANK1]);
    Bitboard promotionsLeft = attackLeft & MaskRank[RANK1];
    Bitboard promotionsRight = attackRight & MaskRank[RANK1];
    Bitboard enpassantLeft = ( (thePieces & ClearFile[FILEH]) >> 7) & board.m_enPassantSquare;
    Bitboard enpassantRight = ( (thePieces & ClearFile[FILEA]) >> 9) & board.m_enPassantSquare;

    oneStep &= ClearRank[RANK1];
    attackLeft &= ClearRank[RANK1];
    attackRight &= ClearRank[RANK1];

    while(oneStep) {
        int toSq = ResetLsb(oneStep);
        int fromSq = toSq + 8;
        Bitboard toBitboard = ONE << toSq;
        AddPawnPushMoves(board, fromSq, toBitboard, MOVE_TYPE::NORMAL);
    }
    while(twoSteps) {
        int toSq = ResetLsb(twoSteps);
        int fromSq = toSq + 16;
        Bitboard toBitboard = ONE << toSq;
        AddPawnPushMoves(board, fromSq, toBitboard, MOVE_TYPE::DOUBLE_PUSH);
    }
    while(attackLeft) {
        int toSq = ResetLsb(attackLeft);
        int fromSq = toSq + 7;
        Bitboard toBitboard = ONE << toSq;
        AddMoves(board, piece, fromSq, toBitboard, enemyPieces);
    }
    while(attackRight) {
        int toSq = ResetLsb(attackRight);
        int fromSq = toSq + 9;
        Bitboard toBitboard = ONE << toSq;
        AddMoves(board, piece, fromSq, toBitboard, enemyPieces);
    }
    while(promotions) {
        int toSq = ResetLsb(promotions);
        int fromSq = toSq + 8;
        Bitboard toBitboard = ONE << toSq;
        AddPromotionMoves(board, fromSq, toBitboard);
    }
    while(promotionsLeft) {
        int toSq = ResetLsb(promotionsLeft);
        int fromSq = toSq + 7;
        Bitboard toBitboard = ONE << toSq;
        AddPromotionMoves(board, fromSq, toBitboard);
    }
    while(promotionsRight) {
        int toSq = ResetLsb(promotionsRight);
        int fromSq = toSq + 9;
        Bitboard toBitboard = ONE << toSq;
        AddPromotionMoves(board, fromSq, toBitboard);
    }
    while(enpassantLeft) {
        int toSq = ResetLsb(enpassantLeft);
        int fromSq = toSq + 7;
        Bitboard toBitboard = ONE << toSq;
        AddPawnPushMoves(board, fromSq, toBitboard, MOVE_TYPE::ENPASSANT);
    }
    while(enpassantRight) {
        int toSq = ResetLsb(enpassantRight);
        int fromSq = toSq + 9;
        Bitboard toBitboard = ONE << toSq;
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
        //Moves
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

    //Moves
    AddMoves(board, piece, fromSq, attacks, enemyPieces);

    //Castling
    AddCastlingMoves(board);
}
void MoveGenerator::GenerateSlidingMoves(PIECE_TYPE pieceType, Board &board) {
    Bitboard thePieces = board.GetPieces( board.ActivePlayer(), pieceType );
    Bitboard ownPieces = board.GetPieces( board.ActivePlayer(), ALL_PIECES );
    Bitboard enemyPieces = board.GetPieces( board.InactivePlayer(), ALL_PIECES );

    while(thePieces) {
        //Attacks
        int fromSq = ResetLsb(thePieces);
        Bitboard attacks = AttacksSliding(pieceType, fromSq, board.m_allpieces);
        attacks &= ~ownPieces;

        // board.m_attackedSquares[board.ActivePlayer()] &= attacks;

        AddMoves(board, pieceType, fromSq, attacks, enemyPieces);
    }
}

void MoveGenerator::AddMoves(Board &board, PIECE_TYPE piece, int fromSq, Bitboard possibleMoves, Bitboard enemyPieces) {

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
            // P(toSq << " " << capturedPiece);
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
            && !board.AttackersTo(color, E1)
            && !board.AttackersTo(color, F1)
            && !board.AttackersTo(color, G1)
        ) {
            fromSq = E1; toSq = G1;
            Move move = Move(fromSq, toSq, PIECE_TYPE::KING, MOVE_TYPE::CASTLING);
            m_moves.push_back(move);
        }
        //WHITE CASTLING QUEEN
        if(castlingRights & CASTLING_Q 
            && !(board.m_allpieces & ( (ONE << B1) | (ONE << C1) | (ONE << D1) ) )
            && !board.AttackersTo(color, E1)
            && !board.AttackersTo(color, D1)
            && !board.AttackersTo(color, C1)
        ) {
            fromSq = E1; toSq = C1;
            Move move = Move(fromSq, toSq, PIECE_TYPE::KING, MOVE_TYPE::CASTLING);
            m_moves.push_back(move);
        }
    } else {
        //BLACK CASTLING KING
        if(castlingRights & CASTLING_k 
            && !(board.m_allpieces & ( (ONE << F8) | (ONE << G8) ) )
            && !board.AttackersTo(color, E8)
            && !board.AttackersTo(color, F8)
            && !board.AttackersTo(color, G8)
        ) {
            fromSq = E8; toSq = G8;
            Move move = Move(fromSq, toSq, PIECE_TYPE::KING, MOVE_TYPE::CASTLING);
            m_moves.push_back(move);
        }
        //BLACK CASTLING QUEEN
        if(castlingRights & CASTLING_q 
            && !(board.m_allpieces & ( (ONE << B8) | (ONE << C8) | (ONE << D8) ) )
            && !board.AttackersTo(color, E8)
            && !board.AttackersTo(color, D8)
            && !board.AttackersTo(color, C8)
        ) {
            fromSq = E8; toSq = C8;
            Move move = Move(fromSq, toSq, PIECE_TYPE::KING, MOVE_TYPE::CASTLING);
            m_moves.push_back(move);
        }
    }
}

// Bitboard MoveGenerator::KingDangerSquares(Board &board) {
//     // Bitboard attackedSquares = ZERO;
//     // Bitboard piecesNoKing = board.m_allpieces & !board.GetPieces(board.ActivePlayer(), KING);
    
//     // attackedSquares |= AttacksSliding(BISHOP, --, piecesNoKing);
//     return ZERO;
// }

// void SquaresUnderAttack() {
//     //Non-sliding pieces
//     AttacksKnights(fromSq);
// }

