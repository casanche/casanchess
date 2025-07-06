#include "MoveGenerator.h"
#include "Attacks.h"
using namespace Attacks;
#include "BitboardUtils.h"
using namespace BitboardUtils;
#include "Board.h"
#include "Utils.h" //random

#include <algorithm>
#include <iostream>

const int MAX_MOVES_RESERVE = 256;
const int MAX_CAPTURES_RESERVE = 64;

//Legal moves
MoveList MoveGenerator::GenerateMoves(Board &board) {
    Init(board);
    m_moves.reserve(MAX_MOVES_RESERVE);
    m_generateQuiet = true;

    //Calculations
    m_kingDangerSquares = GenerateKingDangerAttacks(board);
    m_pinned = PinnedPieces(board, m_color);

    if(board.IsCheck()) {
        GenerateEvasionMoves(board);
    } else {
        GeneratePseudoMoves(board);
    }

    return m_moves;
}

MoveList MoveGenerator::GenerateCaptures(Board &board) {
    Init(board);
    m_moves.reserve(MAX_CAPTURES_RESERVE);
    m_generateQuiet = false;

    //Calculations
    m_kingDangerSquares = GenerateKingDangerAttacks(board);
    m_pinned = PinnedPieces(board, m_color);

    //Don't allow king moves to empty squares
    m_kingDangerSquares |= ~m_allPieces;

    GeneratePseudoMoves(board);

    return m_moves;
}

void MoveGenerator::Init(const Board& board) {
    //Init
    m_color = board.ActivePlayer();
    m_enemyColor = board.InactivePlayer();
    m_ownPieces = board.GetPieces(m_color, ALL_PIECES);
    m_enemyPieces = board.GetPieces(m_enemyColor, ALL_PIECES);
    m_allPieces = board.AllPieces();

    // Initialize the push and capture masks
    m_captureMask = ALL;
    m_pushMask = ALL;
    for(int i = 0; i < 64; ++i) {
        m_pinnedPushMask[i] = ALL;
        m_pinnedCaptureMask[i] = ALL;
    }
}

void MoveGenerator::GeneratePseudoMoves(Board &board) {
    GenerateKingMoves(board);
    GenerateKnightMoves(board);
    GeneratePawnMoves(board);

    GenerateSlidingMoves(BISHOP, board);
    GenerateSlidingMoves(ROOK, board);
    GenerateSlidingMoves(QUEEN, board);
}

void MoveGenerator::GenerateEvasionMoves(Board &board) {
    Bitboard checkers = board.Checkers();
    int numCheckers = PopCount(checkers);

    if(numCheckers > 1) {
        GenerateKingMoves(board);
    }
    else if(numCheckers == 1) {
        //Create the capture mask
        m_captureMask = checkers;

        //Create the push mask (block moves)
        int checkerSquare = BitscanForward(checkers);
        PIECE_TYPE checkerType = board.GetPieceAtSquare(m_enemyColor, checkerSquare);
        switch(checkerType) {
            case PAWN: case KNIGHT: {
                m_pushMask = ZERO; //only capture solves the check
            } break;
            case BISHOP: case ROOK: case QUEEN: {
                int kingSquare = BitscanForward( board.Piece(m_color, KING) );
                m_pushMask = Attacks::Between(checkerSquare, kingSquare);
            } break;
            default: assert(false);
        };

        GeneratePseudoMoves(board);
    }
}

Move MoveGenerator::RandomMove() {
    Utils::PRNG rng;
    int max_index = static_cast<int>(m_moves.size()) - 1;
    uint32_t randomIndex = rng.Random(0, max_index);

    return m_moves[randomIndex];
}

void MoveGenerator::GeneratePawnMoves(Board &board) {
    enum ATTACK_SIDE {LEFT, RIGHT};

    //Relative functions (white/black)
    auto RNorth = [&](const Bitboard& b) -> Bitboard {
        return m_color == WHITE ? North(b) : South(b);
    };
    auto RSouth = [&](const Bitboard& b, int times=1) -> Bitboard {
        return m_color == WHITE ? South(b, times) : North(b, times);
    };
    auto RWest = [&](const Bitboard& b) -> Bitboard {
        return m_color == WHITE ? West(b) : East(b);
    };
    auto REast = [&](const Bitboard& b) -> Bitboard {
        return m_color == WHITE ? East(b) : West(b);
    };
    int relativeRank3 = m_color == WHITE ? RANK3 : RANK6;
    int relativeRank8 = m_color == WHITE ? RANK8 : RANK1;

    PIECE_TYPE piece = PAWN;

    Bitboard thePawns = board.GetPieces(m_color, piece);

    Bitboard singlePush = RNorth(thePawns) & ~m_allPieces;
    Bitboard doublePush = RNorth(singlePush & MaskRank[relativeRank3]) & ~m_allPieces & m_pushMask;
    Bitboard attack[2] = { RWest(RNorth(thePawns)) & m_enemyPieces,
                           REast(RNorth(thePawns)) & m_enemyPieces };
    Bitboard promotionPush = (singlePush & MaskRank[relativeRank8]);
    Bitboard promotionAttack[2] = { attack[LEFT]  & m_enemyPieces & MaskRank[relativeRank8],
                                    attack[RIGHT] & m_enemyPieces & MaskRank[relativeRank8] };
    Bitboard enpassant[2] = {{0}};
    if(board.EnPassantSquare()) {
        enpassant[LEFT]  = RWest(RNorth(thePawns)) & board.EnPassantSquare();
        enpassant[RIGHT] = REast(RNorth(thePawns)) & board.EnPassantSquare();
    }

    singlePush &= ClearRank[relativeRank8] & m_pushMask;
    attack[LEFT] &= ClearRank[relativeRank8];
    attack[RIGHT] &= ClearRank[relativeRank8];

    singlePush *= m_generateQuiet;
    doublePush *= m_generateQuiet;

    while(singlePush) {
        int toSq = ResetLsb(singlePush);
        Bitboard toBitboard = SquareBB(toSq);
        int fromSq = BitscanForward( RSouth(toBitboard) );
        toBitboard &= m_pinnedPushMask[fromSq];
        if(toBitboard) {
            Move move = Move(fromSq, toSq, PAWN, MOVE_TYPE::NORMAL);
            m_moves.push_back(move);
        }
    }
    while(doublePush) {
        int toSq = ResetLsb(doublePush);
        Bitboard toBitboard = SquareBB(toSq);
        int fromSq = BitscanForward( RSouth(toBitboard,2) );
        toBitboard &= m_pinnedPushMask[fromSq];
        if(toBitboard) {
            Move move = Move(fromSq, toSq, PAWN, MOVE_TYPE::DOUBLE_PUSH);
            m_moves.push_back(move);
        }
    }
    while(promotionPush) {
        int toSq = ResetLsb(promotionPush);
        Bitboard toBitboard = SquareBB(toSq);
        int fromSq = BitscanForward( RSouth(toBitboard) );
        toBitboard &= m_pinnedPushMask[fromSq];
        AddPromotionMoves(board, fromSq, toBitboard);
    }

    for(ATTACK_SIDE side : {LEFT, RIGHT}) {
        auto FromBitboard = [&](const Bitboard& b) -> Bitboard {
            return (side == LEFT) ? REast(RSouth(b))
                                  : RWest(RSouth(b));
        };

        while(attack[side]) {
            int toSq = ResetLsb(attack[side]);
            Bitboard toBitboard = SquareBB(toSq);
            int fromSq = BitscanForward( FromBitboard(toBitboard) );
            toBitboard &= m_pinnedCaptureMask[fromSq];
            AddMoves(board, piece, fromSq, toBitboard);
        }
        while(promotionAttack[side]) {
            int toSq = ResetLsb(promotionAttack[side]);
            Bitboard toBitboard = SquareBB(toSq);
            int fromSq = BitscanForward( FromBitboard(toBitboard) );
            toBitboard &= m_pinnedCaptureMask[fromSq];
            AddPromotionMoves(board, fromSq, toBitboard);
        }
        while(enpassant[side]) {
            int toSq = ResetLsb(enpassant[side]);
            Bitboard toBitboard = SquareBB(toSq);
            int fromSq = BitscanForward( FromBitboard(toBitboard) );
            Bitboard enemyPawn = RSouth(board.EnPassantSquare());

            //Check legality
            Bitboard blockers = m_allPieces ^ SquareBB(fromSq) ^ enemyPawn; //remove the own and enemy pawns
            int kingSquare = BitscanForward( board.Piece(m_color,KING) );
            if(board.AttackersTo(m_color, kingSquare, blockers) & ~enemyPawn) //any attackers that are not the enemy pawn?
                continue;

            if(toBitboard) {
                Move move = Move(fromSq, toSq, PAWN, MOVE_TYPE::ENPASSANT);
                m_moves.push_back(move);
            }
        }
    }
}
void MoveGenerator::GenerateKnightMoves(Board &board) {
    PIECE_TYPE piece = KNIGHT;
    Bitboard theKnights = board.GetPieces(m_color, piece);
    theKnights &= ~m_pinned;

    while(theKnights) {
        int square = ResetLsb(theKnights);
        Bitboard attacks = AttacksKnights(square) & ~m_ownPieces;

        AddMoves(board, piece, square, attacks);
    }
}
void MoveGenerator::GenerateKingMoves(Board &board) {
    PIECE_TYPE piece = KING;
    Bitboard theKing = board.GetPieces(m_color, piece);

    //Exit if no king on the board
    assert(theKing);

    //Attacks
    int square = BitscanForward(theKing);
    Bitboard attacks = AttacksKing(square) & ~m_ownPieces;

    //Evade attacked squares
    attacks &= ~m_kingDangerSquares;

    //Moves
    AddMoves(board, piece, square, attacks);

    //Castling. Don't generate in evasion
    if(m_generateQuiet && !board.IsCheck())
        AddCastlingMoves(board);
}
void MoveGenerator::GenerateSlidingMoves(PIECE_TYPE pieceType, Board &board) {
    Bitboard thePieces = board.GetPieces(m_color, pieceType);

    while(thePieces) {
        //Attacks
        int fromSq = ResetLsb(thePieces);
        Bitboard attacks = AttacksSliding(pieceType, fromSq, m_allPieces) & ~m_ownPieces;
        attacks &= m_pinnedPushMask[fromSq] | m_pinnedCaptureMask[fromSq];

        AddMoves(board, pieceType, fromSq, attacks);
    }
}

void MoveGenerator::AddMoves(Board &board, PIECE_TYPE piece, int fromSq, Bitboard possibleMoves) {
    // ===================
    // == Capture moves ==
    // ===================
    Bitboard captureMoves = possibleMoves & m_enemyPieces;
    if(piece != KING) {
        captureMoves &= m_captureMask;
    }

    while(captureMoves) {
        int toSq = ResetLsb(captureMoves);
        PIECE_TYPE capturedPiece = board.GetPieceAtSquare(m_enemyColor, toSq);
        Move move = Move(fromSq, toSq, piece, MOVE_TYPE::CAPTURE);
        move.SetCapturedType(capturedPiece);
        m_moves.push_back(move);
    }

    // =================================
    // == Normal moves (non-captures) ==
    // =================================
    Bitboard normalMoves = possibleMoves & ~m_enemyPieces;
    if(piece != KING) {
        normalMoves &= m_pushMask;
    }
    while(normalMoves && m_generateQuiet) {
        int toSq = ResetLsb(normalMoves);
        Move move = Move(fromSq, toSq, piece, MOVE_TYPE::NORMAL);
        m_moves.push_back(move);
    }

}

void MoveGenerator::AddPromotionMoves(Board &board, int fromSq, Bitboard promotionMoves) {
    assert(PopCount(promotionMoves) <= 1);

    bool isCapture = promotionMoves & m_enemyPieces;
    promotionMoves &= isCapture ? m_captureMask
                                : m_pushMask;

    while(promotionMoves) {
        int toSq = ResetLsb(promotionMoves);

        Move move;
        if(isCapture) { //PROMOTION_CAPTURE
            move = Move(fromSq, toSq, PIECE_TYPE::PAWN, MOVE_TYPE::PROMOTION_CAPTURE);
            
            PIECE_TYPE capturedPiece = board.GetPieceAtSquare(m_enemyColor, toSq);
            move.SetCapturedType(capturedPiece);
        } else { //PROMOTION
            move = Move(fromSq, toSq, PIECE_TYPE::PAWN, MOVE_TYPE::PROMOTION);
        }

        move.SetPromotionFlag(PROMOTION_QUEEN);
        m_moves.push_back(move);

        if(m_generateQuiet) {
            for(int p = PROMOTION_KNIGHT; p <= PROMOTION_BISHOP; p++) {
                move.SetPromotionFlag((PROMOTION_TYPE)p);
                m_moves.push_back(move);
            }
        }

    }
}
void MoveGenerator::AddCastlingMoves(Board &board) {
    u8 castlingRights = board.CastlingRights();

    if(m_color == WHITE) {
        //WHITE CASTLING KING
        if(castlingRights & CASTLING_K 
            && !(m_allPieces & ( (ONE << F1) | (ONE << G1) ) )
            && !board.IsAttacked(m_color, E1)
            && !board.IsAttacked(m_color, F1)
            && !board.IsAttacked(m_color, G1)
        ) {
            Move move = Move(E1, G1, PIECE_TYPE::KING, MOVE_TYPE::CASTLING);
            m_moves.push_back(move);
        }
        //WHITE CASTLING QUEEN
        if(castlingRights & CASTLING_Q 
            && !(m_allPieces & ( (ONE << B1) | (ONE << C1) | (ONE << D1) ) )
            && !board.IsAttacked(m_color, E1)
            && !board.IsAttacked(m_color, D1)
            && !board.IsAttacked(m_color, C1)
        ) {
            Move move = Move(E1, C1, PIECE_TYPE::KING, MOVE_TYPE::CASTLING);
            m_moves.push_back(move);
        }
    } else {
        //BLACK CASTLING KING
        if(castlingRights & CASTLING_k 
            && !(m_allPieces & ( (ONE << F8) | (ONE << G8) ) )
            && !board.IsAttacked(m_color, E8)
            && !board.IsAttacked(m_color, F8)
            && !board.IsAttacked(m_color, G8)
        ) {
            Move move = Move(E8, G8, PIECE_TYPE::KING, MOVE_TYPE::CASTLING);
            m_moves.push_back(move);
        }
        //BLACK CASTLING QUEEN
        if(castlingRights & CASTLING_q 
            && !(m_allPieces & ( (ONE << B8) | (ONE << C8) | (ONE << D8) ) )
            && !board.IsAttacked(m_color, E8)
            && !board.IsAttacked(m_color, D8)
            && !board.IsAttacked(m_color, C8)
        ) {
            Move move = Move(E8, C8, PIECE_TYPE::KING, MOVE_TYPE::CASTLING);
            m_moves.push_back(move);
        }
    }
}

Bitboard MoveGenerator::GenerateKingDangerAttacks(Board &board) {
    Bitboard attacks = ZERO;
    Bitboard blockers = m_allPieces ^ board.Piece(m_color, KING);
    for(PIECE_TYPE pieceType = PAWN; pieceType <= KING; ++pieceType) {
        Bitboard enemyPieces = board.Piece(m_enemyColor, pieceType);
        while(enemyPieces) {
            int square = ResetLsb(enemyPieces);
            switch(pieceType) {
                case PAWN: attacks |= AttacksPawns(m_enemyColor, square); break;
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
//Step 2: Generate attacks from our king (as the same sliding piece)
//Step 3: A pinned piece would appear in both bitboards
Bitboard MoveGenerator::PinnedPieces(Board &board, COLOR color) {
    Bitboard pinned = ZERO;
    int kingSquare = BitscanForward( board.GetPieces(color, KING) );

    for(PIECE_TYPE pieceType : {BISHOP, ROOK, QUEEN}) {
        Bitboard enemySlidings = board.GetPieces(m_enemyColor, pieceType);
        while(enemySlidings) {
            int slidingSquare = ResetLsb(enemySlidings);
            switch(pieceType) {
                case BISHOP: {
                    pinned |= FillPinned(BISHOP, slidingSquare, kingSquare);
                } break;
                case ROOK: {
                    pinned |= FillPinned(ROOK, slidingSquare, kingSquare);
                } break;
                case QUEEN: {
                    pinned |= FillPinned(BISHOP, slidingSquare, kingSquare);
                    pinned |= FillPinned(ROOK, slidingSquare, kingSquare);
                } break;
                default: break;
            };
        }
    }
    return pinned;
}

Bitboard MoveGenerator::FillPinned(PIECE_TYPE slidingType, int slidingSquare, int kingSquare) {
    assert(slidingType == BISHOP || slidingType == ROOK);

    Bitboard pinned = ZERO;
    if(Attacks::IsInDirection(slidingType, slidingSquare, kingSquare)) {
        Bitboard inBetween = Between(slidingSquare, kingSquare);
        Bitboard attacksFromEnemy = AttacksSliding(slidingType, slidingSquare, m_allPieces);
        Bitboard attacksFromKing = AttacksSliding(slidingType, kingSquare, m_allPieces);
        pinned = attacksFromEnemy & attacksFromKing & m_ownPieces;
        if(pinned) {
            assert(PopCount(pinned) == 1);
            m_pinnedPushMask[BitscanForward(pinned)] = inBetween;
            m_pinnedCaptureMask[BitscanForward(pinned)] = SquareBB(slidingSquare);
        }
    }
    return pinned;
}
