// MoveGenerator.cpp
//
// Legal move generation.
//
// Key concepts:
//   - Pinned pieces: Pieces that cannot move freely because they block an attack on the king.
//     Tracked via `pinnedPieces` bitboard and pin direction.
//   - Check evasions: When in check, only specific moves are legal:
//     - `captureMask`: Squares where capturing resolves the check (the checking piece)
//     - `pushMask`: Squares where blocking resolves the check (between king and slider)
//   - King danger squares: Squares attacked by enemy, where king cannot move.
//
// Three generation modes:
//   - Legal: All legal moves (normal search)
//   - Evasion: Only check evasions (when in check)
//   - Tactical: Captures and promotions only (quiescence search)

#include "MoveGenerator.h"

#include "Attacks.h"
using namespace Attacks;
#include "BitboardUtils.h"
#include "Board.h"
#include "Utils.h" //random

#include <algorithm>
#include <iostream>

namespace {
    enum class GENERATION_TYPE {Legal, Evasion, Tactical};

    // ========================================
    // === Internal state (context) methods ===
    // ========================================

    // Contains the context for a single move generation operation
    struct Context {
        MoveList* moves;
    
        COLOR color;
        COLOR enemyColor;
    
        Bitboard ownPieces;
        Bitboard enemyPieces;
        Bitboard allPieces;

        Bitboard ownKing;
        Bitboard enemyKing;
        int ownKingSquare;
        int enemyKingSquare;
    
        Bitboard kingDangerSquares;
        Bitboard pinnedPieces;
    
        // Squares where capture is allowed. In case of check, the piece giving check
        Bitboard captureMask;
        // Squares where push is allowed. In case of check, squares that block a check
        Bitboard pushMask;
    
        Bitboard pinnedCaptureMask[64];
        Bitboard pinnedPushMask[64];

        bool generateQuiet;

        void AddMove(const Move& move) {
            moves->add(move);
        }
    };

    // Empty squares attacked by the enemy pieces, so our king cannot move there
    Bitboard GenerateKingDangerSquares(const Context &context, const Board &board) {
        Bitboard dangerSquares = ZERO;
        Bitboard blockers = context.allPieces ^ context.ownKing;
        // Loop over enemy pieces
        for(PIECE_TYPE pieceType = PAWN; pieceType <= KING; ++pieceType) {
            Bitboard pieceSquares = board.Piece(context.enemyColor, pieceType);
            for(int square : BitboardIterator(pieceSquares)) {
                switch(pieceType) {
                    case PAWN:   dangerSquares |= AttacksPawns(context.enemyColor, square); break;
                    case KNIGHT: dangerSquares |= AttacksKnights(square); break;
                    case KING:   dangerSquares |= AttacksKing(square); break;
                    case BISHOP: dangerSquares |= AttacksSliding(BISHOP, square, blockers); break;
                    case ROOK:   dangerSquares |= AttacksSliding(ROOK, square, blockers); break;
                    case QUEEN:  dangerSquares |= AttacksSliding(QUEEN, square, blockers); break;
                    default: assert(false);
                };
            }
        }
        return dangerSquares;
    }

    void FillPinnedPiecesAndMasks(
        const Context &context,
        const Board &board,
        Bitboard &pinnedPieces,
        Bitboard pinnedPushMask[64],
        Bitboard pinnedCaptureMask[64]
    ) {
        // First, initialize the state
        pinnedPieces = ZERO;
        for(int i = 0; i < 64; ++i) {
            pinnedPushMask[i] = ALL;
            pinnedCaptureMask[i] = ALL;
        }

        // Generate sliding attacks from our king
        for(PIECE_TYPE slidingType : {ROOK, BISHOP}) {
            Bitboard kingRays = AttacksSliding(slidingType, context.ownKingSquare, context.enemyPieces);
            Bitboard enemySlidings = board.GetPieces(context.enemyColor, slidingType) |
                                     board.GetPieces(context.enemyColor, QUEEN);

            // If overlapping with the enemy slidings, that enemy piece is a potential pinner
            Bitboard potentialPinners = kingRays & enemySlidings;

            for(int pinnerSquare : BitboardIterator(potentialPinners)) {
                Bitboard lineBetween = Attacks::Between(context.ownKingSquare, pinnerSquare);
                
                // Our piece is pinned if lays in the line between our king and the pinner
                Bitboard pinnedCandidates = lineBetween & context.ownPieces;

                // We got a match! Fill the state
                if(OnlyOne(pinnedCandidates)) {
                    int pinnedSquare = BitscanForward(pinnedCandidates);
                    pinnedPieces |= pinnedCandidates;
                    pinnedPushMask[pinnedSquare] = lineBetween;
                    pinnedCaptureMask[pinnedSquare] = SquareBB(pinnerSquare);
                }

            }
        }

    }

    void InitContext(Context &context, const Board& board) {
        context.moves = nullptr;

        context.color = board.ActivePlayer();
        context.enemyColor = board.InactivePlayer();

        context.ownPieces = board.GetPieces(context.color, ALL_PIECES);
        context.enemyPieces = board.GetPieces(context.enemyColor, ALL_PIECES);
        context.allPieces = board.AllPieces();
        
        context.ownKing = board.Piece(context.color, KING);
        context.enemyKing = board.Piece(context.enemyColor, KING);
        context.ownKingSquare = BitscanForward(context.ownKing);
        context.enemyKingSquare = BitscanForward(context.enemyKing);

        context.captureMask = ALL;
        context.pushMask = ALL;

        context.generateQuiet = true;

        // Depend on previous variables
        context.kingDangerSquares = GenerateKingDangerSquares(context, board);
        FillPinnedPiecesAndMasks(context, board,
            context.pinnedPieces,
            context.pinnedPushMask,
            context.pinnedCaptureMask
        );
    }

    // ===============================
    // === Move generation methods ===
    // ===============================

    void AddMoves(Context &context, Board &board, PIECE_TYPE piece, int fromSq, Bitboard possibleMoves) {
        // ===================
        // == Capture moves ==
        // ===================
        Bitboard captureMoves = possibleMoves & context.enemyPieces;
        if(piece != KING) {
            captureMoves &= context.captureMask;
        }

        for(int toSq : BitboardIterator(captureMoves)) {
            PIECE_TYPE capturedPiece = board.GetPieceAtSquare(context.enemyColor, toSq);
            Move move = Move(fromSq, toSq, piece, MOVE_TYPE::CAPTURE);
            move.SetCapturedType(capturedPiece);
            context.AddMove(move);
        }

        // =================================
        // == Normal moves (non-captures) ==
        // =================================
        if(!context.generateQuiet) {
            return;
        }

        Bitboard normalMoves = possibleMoves & ~context.enemyPieces;
        if(piece != KING) {
            normalMoves &= context.pushMask;
        }
        for(int toSq : BitboardIterator(normalMoves)) {
            Move move = Move(fromSq, toSq, piece, MOVE_TYPE::NORMAL);
            context.AddMove(move);
        }
    }

    void AddPromotionMoves(Context &context, Board &board, int fromSq, Bitboard promotionMoves) {
        assert(PopCount(promotionMoves) <= 1);

        bool isCapture = promotionMoves & context.enemyPieces;
        promotionMoves &= isCapture ? context.captureMask
                                    : context.pushMask;

        for(int toSq : BitboardIterator(promotionMoves)) {
            Move move;
            if(isCapture) { //PROMOTION_CAPTURE
                move = Move(fromSq, toSq, PIECE_TYPE::PAWN, MOVE_TYPE::PROMOTION_CAPTURE);

                PIECE_TYPE capturedPiece = board.GetPieceAtSquare(context.enemyColor, toSq);
                move.SetCapturedType(capturedPiece);
            } else { //PROMOTION
                move = Move(fromSq, toSq, PIECE_TYPE::PAWN, MOVE_TYPE::PROMOTION);
            }

            move.SetPromotionFlag(PROMOTION_QUEEN);
            context.AddMove(move);

            if(context.generateQuiet) {
                for(int p = PROMOTION_KNIGHT; p <= PROMOTION_BISHOP; p++) {
                    move.SetPromotionFlag((PROMOTION_TYPE)p);
                    context.AddMove(move);
                }
            }
        }
    }
    void AddCastlingMoves(Context &context, Board &board) {
        u8 castlingRights = board.CastlingRights();

        if(context.color == WHITE) {
            //WHITE CASTLING KING
            if(castlingRights & CASTLING_K 
                && !(context.allPieces & ( (ONE << F1) | (ONE << G1) ) )
                && !board.IsAttacked(context.color, E1)
                && !board.IsAttacked(context.color, F1)
                && !board.IsAttacked(context.color, G1)
            ) {
                Move move = Move(E1, G1, PIECE_TYPE::KING, MOVE_TYPE::CASTLING);
                context.AddMove(move);
            }
            //WHITE CASTLING QUEEN
            if(castlingRights & CASTLING_Q 
                && !(context.allPieces & ( (ONE << B1) | (ONE << C1) | (ONE << D1) ) )
                && !board.IsAttacked(context.color, E1)
                && !board.IsAttacked(context.color, D1)
                && !board.IsAttacked(context.color, C1)
            ) {
                Move move = Move(E1, C1, PIECE_TYPE::KING, MOVE_TYPE::CASTLING);
                context.AddMove(move);
            }
        } else {
            //BLACK CASTLING KING
            if(castlingRights & CASTLING_k 
                && !(context.allPieces & ( (ONE << F8) | (ONE << G8) ) )
                && !board.IsAttacked(context.color, E8)
                && !board.IsAttacked(context.color, F8)
                && !board.IsAttacked(context.color, G8)
            ) {
                Move move = Move(E8, G8, PIECE_TYPE::KING, MOVE_TYPE::CASTLING);
                context.AddMove(move);
            }
            //BLACK CASTLING QUEEN
            if(castlingRights & CASTLING_q 
                && !(context.allPieces & ( (ONE << B8) | (ONE << C8) | (ONE << D8) ) )
                && !board.IsAttacked(context.color, E8)
                && !board.IsAttacked(context.color, D8)
                && !board.IsAttacked(context.color, C8)
            ) {
                Move move = Move(E8, C8, PIECE_TYPE::KING, MOVE_TYPE::CASTLING);
                context.AddMove(move);
            }
        }
    }

    void GeneratePawnMoves(Context &context, Board &board) {
        enum ATTACK_SIDE {LEFT, RIGHT};

        //Relative functions (white/black)
        auto RNorth = [&](const Bitboard& b) -> Bitboard {
            return context.color == WHITE ? North(b) : South(b);
        };
        auto RSouth = [&](const Bitboard& b, int times=1) -> Bitboard {
            return context.color == WHITE ? South(b, times) : North(b, times);
        };
        auto RWest = [&](const Bitboard& b) -> Bitboard {
            return context.color == WHITE ? West(b) : East(b);
        };
        auto REast = [&](const Bitboard& b) -> Bitboard {
            return context.color == WHITE ? East(b) : West(b);
        };
        int relativeRank3 = context.color == WHITE ? RANK3 : RANK6;
        int relativeRank8 = context.color == WHITE ? RANK8 : RANK1;

        PIECE_TYPE piece = PAWN;

        Bitboard thePawns = board.GetPieces(context.color, piece);

        Bitboard singlePush = RNorth(thePawns) & ~context.allPieces;
        Bitboard doublePush = RNorth(singlePush & MaskRank[relativeRank3]) & ~context.allPieces & context.pushMask;
        Bitboard attack[2] = { RWest(RNorth(thePawns)) & context.enemyPieces,
                            REast(RNorth(thePawns)) & context.enemyPieces };
        Bitboard promotionPush = (singlePush & MaskRank[relativeRank8]);
        Bitboard promotionAttack[2] = { attack[LEFT]  & context.enemyPieces & MaskRank[relativeRank8],
                                        attack[RIGHT] & context.enemyPieces & MaskRank[relativeRank8] };
        Bitboard enpassant[2] = {0};
        if(board.EnPassantSquare()) {
            enpassant[LEFT]  = RWest(RNorth(thePawns)) & board.EnPassantSquare();
            enpassant[RIGHT] = REast(RNorth(thePawns)) & board.EnPassantSquare();
        }

        singlePush &= ClearRank[relativeRank8] & context.pushMask;
        attack[LEFT] &= ClearRank[relativeRank8];
        attack[RIGHT] &= ClearRank[relativeRank8];

        singlePush *= context.generateQuiet;
        doublePush *= context.generateQuiet;

        for(int toSq : BitboardIterator(singlePush)) {
            Bitboard toBitboard = SquareBB(toSq);
            int fromSq = BitscanForward( RSouth(toBitboard) );
            toBitboard &= context.pinnedPushMask[fromSq];
            if(toBitboard) {
                Move move = Move(fromSq, toSq, PAWN, MOVE_TYPE::NORMAL);
                context.AddMove(move);
            }
        }
        for(int toSq : BitboardIterator(doublePush)) {
            Bitboard toBitboard = SquareBB(toSq);
            int fromSq = BitscanForward( RSouth(toBitboard,2) );
            toBitboard &= context.pinnedPushMask[fromSq];
            if(toBitboard) {
                Move move = Move(fromSq, toSq, PAWN, MOVE_TYPE::DOUBLE_PUSH);
                context.AddMove(move);
            }
        }
        for(int toSq : BitboardIterator(promotionPush)) {
            Bitboard toBitboard = SquareBB(toSq);
            int fromSq = BitscanForward( RSouth(toBitboard) );
            toBitboard &= context.pinnedPushMask[fromSq];
            AddPromotionMoves(context, board, fromSq, toBitboard);
        }

        for(ATTACK_SIDE side : {LEFT, RIGHT}) {
            auto FromBitboard = [&](const Bitboard& b) -> Bitboard {
                return (side == LEFT) ? REast(RSouth(b))
                                    : RWest(RSouth(b));
            };

            for(int toSq : BitboardIterator(attack[side])) {
                Bitboard toBitboard = SquareBB(toSq);
                int fromSq = BitscanForward( FromBitboard(toBitboard) );
                toBitboard &= context.pinnedCaptureMask[fromSq];
                AddMoves(context, board, piece, fromSq, toBitboard);
            }
            for(int toSq : BitboardIterator(promotionAttack[side])) {
                Bitboard toBitboard = SquareBB(toSq);
                int fromSq = BitscanForward( FromBitboard(toBitboard) );
                toBitboard &= context.pinnedCaptureMask[fromSq];
                AddPromotionMoves(context, board, fromSq, toBitboard);
            }
            for(int toSq : BitboardIterator(enpassant[side])) {
                Bitboard toBitboard = SquareBB(toSq);
                int fromSq = BitscanForward( FromBitboard(toBitboard) );
                Bitboard enemyPawn = RSouth(board.EnPassantSquare());

                // Check legality
                // For blockers: move our pawn and remove the enemy pawn
                Bitboard blockers = (context.allPieces ^ SquareBB(fromSq) ^ enemyPawn) | toBitboard;
                int kingSquare = BitscanForward( board.Piece(context.color,KING) );
                if(board.AttackersTo(context.color, kingSquare, blockers) & ~enemyPawn) //any attackers that are not the enemy pawn?
                    continue;

                if(toBitboard) {
                    Move move = Move(fromSq, toSq, PAWN, MOVE_TYPE::ENPASSANT);
                    move.SetCapturedType(PAWN);
                    context.AddMove(move);
                }
            }
        }
    }
    void GenerateKnightMoves(Context &context, Board &board) {
        PIECE_TYPE piece = KNIGHT;
        Bitboard theKnights = board.GetPieces(context.color, piece);
        theKnights &= ~context.pinnedPieces;

        for(int square : BitboardIterator(theKnights)) {
            Bitboard attacks = AttacksKnights(square) & ~context.ownPieces;
            AddMoves(context, board, piece, square, attacks);
        }
    }
    void GenerateKingMoves(Context &context, Board &board) {
        PIECE_TYPE piece = KING;
        Bitboard theKing = board.GetPieces(context.color, piece);

        //Exit if no king on the board
        assert(theKing);

        //Attacks
        int square = BitscanForward(theKing);
        Bitboard attacks = AttacksKing(square) & ~context.ownPieces;

        //Evade attacked squares
        attacks &= ~context.kingDangerSquares;

        //Moves
        AddMoves(context, board, piece, square, attacks);

        //Castling. Don't generate in evasion
        if(context.generateQuiet && !board.IsCheck())
            AddCastlingMoves(context, board);
    }
    void GenerateSlidingMoves(PIECE_TYPE pieceType, Context &context, Board &board) {
        Bitboard thePieces = board.GetPieces(context.color, pieceType);

        for(int fromSq : BitboardIterator(thePieces)) {
            Bitboard attacks = AttacksSliding(pieceType, fromSq, context.allPieces) & ~context.ownPieces;
            attacks &= context.pinnedPushMask[fromSq] | context.pinnedCaptureMask[fromSq];
            AddMoves(context, board, pieceType, fromSq, attacks);
        }
    }

    void GeneratePseudoMoves(Context &context, Board &board) {
        GenerateKingMoves(context, board);
        GenerateKnightMoves(context, board);
        GeneratePawnMoves(context, board);

        GenerateSlidingMoves(BISHOP, context, board);
        GenerateSlidingMoves(ROOK, context, board);
        GenerateSlidingMoves(QUEEN, context, board);
    }

    MoveList Generate(Board& board, GENERATION_TYPE type) {
        MoveList moves; // Assuming RVO/NRVO optimization by the compiler to avoid the return copy

        Context context;
        InitContext(context, board);

        context.moves = &moves;

        switch(type) {
            case GENERATION_TYPE::Legal:
                break;
            case GENERATION_TYPE::Tactical:
                context.generateQuiet = false;
                break;
            case GENERATION_TYPE::Evasion: {
                Bitboard checkers = board.Checkers();
                int numCheckers = PopCount(checkers);

                if(numCheckers > 1) {
                    // Double-check, only king moves allowed
                    GenerateKingMoves(context, board);
                    return moves;
                }
                else if(numCheckers == 1) {
                    context.captureMask = checkers;
                    int checkerSquare = BitscanForward(checkers);
                    PIECE_TYPE checkerType = board.GetPieceAtSquare(context.enemyColor, checkerSquare);
                    if(checkerType == PAWN || checkerType == KNIGHT) {
                        context.pushMask = ZERO; // No in-between moves are possible
                    } else {
                        assert((checkerType == BISHOP) || (checkerType == ROOK) || (checkerType == QUEEN));
                        int kingSquare = BitscanForward( board.Piece(context.color, KING) );
                        context.pushMask = Attacks::Between(checkerSquare, kingSquare);
                    }
                }
            } break;
        }

        GeneratePseudoMoves(context, board);
        return moves;
    }

} // namespace

MoveList MoveGenerator::GenerateMoves(Board &board) {
    if(board.IsCheck())
        return GenerateEvasionMoves(board);
    return Generate(board, GENERATION_TYPE::Legal);
}

MoveList MoveGenerator::GenerateEvasionMoves(Board &board) {
    return Generate(board, GENERATION_TYPE::Evasion);
}

MoveList MoveGenerator::GenerateTacticalMoves(Board &board) {
    return Generate(board, GENERATION_TYPE::Tactical);
}

Move MoveGenerator::RandomMove(const MoveList& moves) {
    assert( !moves.empty() );

    Utils::PRNG rng;
    int max_index = static_cast<int>( moves.size() ) - 1;
    uint32_t randomIndex = rng.Random32(0, max_index);

    return moves[randomIndex];
}
