#include "MoveMaker.h"
#include "Board.h"

void MoveMaker::MakeMove(Board& board, Move move) {
    // Get move information
    COLORS color = board.ActivePlayer();
    int fromSq = move.FromSq();
    int toSq = move.ToSq();
    PIECE_TYPE pieceType = move.PieceType();
    MOVE_TYPE moveType = move.MoveType();

    //Increase ply
    ++board.m_ply;
    if(color == BLACK) {
        ++board.m_moveNumber;
    }
    //Fifty-rule
    if(pieceType == PAWN || moveType == CAPTURE) {
        board.m_fiftyrule = 0;
    } else {
        ++board.m_fiftyrule;
    }

    //Reset en-passant square
    if(board.m_enPassantSquare) {
        board.m_zobristKey.UpdateEnpassant(board.m_enPassantSquare);
        board.m_enPassantSquare = ZERO;
    }

    // Move the active piece
    MovePiece(board, fromSq, toSq, color, pieceType);

    //Remove captured piece
    if(moveType == CAPTURE || moveType == PROMOTION_CAPTURE) {
        RemovePiece(board, toSq, board.InactivePlayer(), move.CapturedType());
    }
    else if(moveType == DOUBLE_PUSH) {
        int squareShift = -8 + 16*color;
        board.m_enPassantSquare = SquareBB(toSq + squareShift);
        board.m_zobristKey.UpdateEnpassant(board.m_enPassantSquare);
    }
    else if(moveType == ENPASSANT) {
        COLORS enemyColor = board.InactivePlayer();
        int squareShift = -8 + 16*color;
        RemovePiece(board, toSq + squareShift, enemyColor, PAWN);
    }
    if(moveType == PROMOTION || moveType == PROMOTION_CAPTURE) {
        RemovePiece(board, toSq, color, PAWN);
        PROMOTION_TYPE promotionType = move.PromotionType();
        switch(promotionType) {
            case(PROMOTION_QUEEN):  AddPiece(board, toSq, color, QUEEN);   break;
            case(PROMOTION_KNIGHT): AddPiece(board, toSq, color, KNIGHT);  break;
            case(PROMOTION_ROOK):   AddPiece(board, toSq, color, ROOK);    break;
            case(PROMOTION_BISHOP): AddPiece(board, toSq, color, BISHOP);  break;
            default: assert(false);
        };
    }

    //Castling rights
    UpdateCastlingRights(board, move);

    //Change the active player
    board.m_activePlayer = board.InactivePlayer();
    board.m_zobristKey.UpdateColor();

    //Store irreversible information (to help a later TakeMove)
    assert(board.m_ply >= 0 && board.m_ply <= MAX_PLIES);
    board.m_history[board.m_ply].fiftyrule = board.m_fiftyrule;
    board.m_history[board.m_ply].castling = board.m_castlingRights;
    board.m_history[board.m_ply].zkey = board.ZKey();
    board.m_history[board.m_ply].enpassant = board.m_enPassantSquare;
    board.m_history[board.m_ply].move = move;

    //Update helper bitboards
    board.UpdateBitboards();

    //Reset check calculation
    board.m_checkCalculated = false;

    //Asserts
    assert(board.CheckIntegrity());
    assert(~board.m_allpieces & SquareBB(fromSq) );
    assert( board.m_allpieces & SquareBB(toSq) );
}

void MoveMaker::TakeMove(Board& board, Move move) {
    // Get move information
    int fromSq = move.FromSq();
    int toSq = move.ToSq();
    PIECE_TYPE pieceType = move.PieceType();
    MOVE_TYPE moveType = move.MoveType();

    //Change the active player
    board.m_activePlayer = board.InactivePlayer();
    COLORS color = board.m_activePlayer;
    board.m_zobristKey.UpdateColor();

    //Decrease the move number if black made the move
    --board.m_ply;
    if(color == BLACK) {
        --board.m_moveNumber;
    }

    //Promotions before moving piece
    if(moveType == PROMOTION || moveType == PROMOTION_CAPTURE) {
        AddPiece(board, toSq, color, PAWN);
        PROMOTION_TYPE promotionType = move.PromotionType();
        switch(promotionType) {
            case(PROMOTION_QUEEN):  RemovePiece(board, toSq, color, QUEEN);   break;
            case(PROMOTION_KNIGHT): RemovePiece(board, toSq, color, KNIGHT);  break;
            case(PROMOTION_ROOK):   RemovePiece(board, toSq, color, ROOK);    break;
            case(PROMOTION_BISHOP): RemovePiece(board, toSq, color, BISHOP);  break;
            default: assert(false);
        };
    }

    // Move the active piece
    MovePiece(board, toSq, fromSq, color, pieceType);

    //Remove captured piece
    if(moveType == CAPTURE || moveType == PROMOTION_CAPTURE) {
        AddPiece(board, toSq, board.InactivePlayer(), move.CapturedType());
    }
    else if(moveType == ENPASSANT) {
        COLORS enemyColor = board.InactivePlayer();
        int squareShift = -8 + 16*color;
        AddPiece(board, toSq + squareShift, enemyColor, PAWN);
    }

    //Retrieve castling rights
    RewindCastlingRights(board, move);

    //Retrieve 50-move counter
    board.m_fiftyrule = board.m_history[board.m_ply].fiftyrule;

    //Retrieve enpassant
    Bitboard changedBits = board.m_enPassantSquare ^ board.m_history[board.m_ply].enpassant;
    if(changedBits) {
        if(board.m_enPassantSquare)
            board.m_zobristKey.UpdateEnpassant(board.m_enPassantSquare);
        if(board.m_history[board.m_ply].enpassant)
            board.m_zobristKey.UpdateEnpassant(board.m_history[board.m_ply].enpassant);
        board.m_enPassantSquare = board.m_history[board.m_ply].enpassant;
    }

    //Update helper bitboards
    board.UpdateBitboards();

    //Reset check calculation
    board.m_checkCalculated = false;

    //Asserts
    assert(board.CheckIntegrity());
    assert(board.m_allpieces & SquareBB(fromSq));
}

void MoveMaker::MakeMove(Board& board, std::string input) {
    MakeMove(board, StringToMove(board, input));
}
void MoveMaker::TakeMove(Board& board) {
    TakeMove(board, board.m_history[board.m_ply].move);
}

void MoveMaker::MakeNull(Board& board) {
    Move move = Move();

    board.m_ply++;
    assert(board.m_ply <= MAX_PLIES);

    //Reset en-passant square
    if(board.m_enPassantSquare) {
        board.m_zobristKey.UpdateEnpassant(board.m_enPassantSquare);
        board.m_enPassantSquare = ZERO;
    }

    //Change the active player
    board.m_activePlayer = board.InactivePlayer();
    board.m_zobristKey.UpdateColor();

    //Store irreversible information (to help a later TakeMove)
    board.m_history[board.m_ply].fiftyrule = board.m_fiftyrule;
    board.m_history[board.m_ply].castling = board.m_castlingRights;
    board.m_history[board.m_ply].zkey = board.ZKey();
    board.m_history[board.m_ply].enpassant = board.m_enPassantSquare;
    board.m_history[board.m_ply].move = move;

    //Reset check calculation
    board.m_checkCalculated = false;

    //Asserts
    assert(move.MoveType() == NULLMOVE);
}

void MoveMaker::TakeNull(Board& board) {
    //Change the active player
    board.m_activePlayer = board.InactivePlayer();

    board.m_ply--;
    assert(board.m_ply >= 0);

    //Retrieve state
    board.m_fiftyrule = board.m_history[board.m_ply].fiftyrule;
    board.m_castlingRights = board.m_history[board.m_ply].castling;
    board.m_zobristKey.SetKey( board.m_history[board.m_ply].zkey );
    board.m_enPassantSquare = board.m_history[board.m_ply].enpassant;

    //Reset check calculation
    board.m_checkCalculated = false;
}

void MoveMaker::AddPiece(Board& board, int square, COLORS color, PIECE_TYPE pieceType) {
    Bitboard &bb = board.m_pieces[color][pieceType];
    bb |= SquareBB(square); //add bit, OR
    board.m_zobristKey.UpdatePiece(color, pieceType, square); //modify the Zobrist key (XOR)
    if(pieceType == PAWN) {
        board.m_pawnKey.UpdatePiece(color, PAWN, square);
    }
}
void MoveMaker::RemovePiece(Board& board, int square, COLORS color, PIECE_TYPE pieceType) {
    Bitboard &bb = board.m_pieces[color][pieceType];
    bb ^= SquareBB(square); //remove bit, XOR
    board.m_zobristKey.UpdatePiece(color, pieceType, square); //modify the Zobrist key (XOR)
    if(pieceType == PAWN) {
        board.m_pawnKey.UpdatePiece(color, PAWN, square);
    }
}
void MoveMaker::MovePiece(Board& board, int fromSq, int toSq, COLORS color, PIECE_TYPE pieceType) {
    AddPiece(board, toSq, color, pieceType);
    RemovePiece(board, fromSq, color, pieceType);
}

void MoveMaker::UpdateCastlingRights(Board& board, const Move& move) {
    if( !board.CastlingRights() || move.MoveType() == NULLMOVE ) {
        assert(move.MoveType() != CASTLING);
        return;
    }

    COLORS color = board.ActivePlayer();
    int fromSq = move.FromSq();
    int toSq = move.ToSq();
    PIECE_TYPE pieceType = move.PieceType();
    MOVE_TYPE moveType = move.MoveType();
    PIECE_TYPE capturedType = move.CapturedType();

    //King castles
    if(moveType == CASTLING) {
        switch(toSq) {
        case G1: {
            MovePiece(board, H1, F1, color, ROOK);
            RemoveCastlingRights(board,  CASTLING_K);
            RemoveCastlingRights(board,  CASTLING_Q);
        } break;
        case C1: {
            MovePiece(board, A1, D1, color, ROOK);
            RemoveCastlingRights(board, CASTLING_K);
            RemoveCastlingRights(board, CASTLING_Q);
        } break;
        case G8: {
            MovePiece(board, H8, F8, color, ROOK);
            RemoveCastlingRights(board, CASTLING_k);
            RemoveCastlingRights(board, CASTLING_q);
        } break;
        case C8: {
            MovePiece(board, A8, D8, color, ROOK);
            RemoveCastlingRights(board, CASTLING_k);
            RemoveCastlingRights(board, CASTLING_q);
        } break;
        default: assert(false);
        };
    }

    //Rook moves
    if(pieceType == ROOK) {
        switch(fromSq) {
            case H1: RemoveCastlingRights(board, CASTLING_K); break;
            case A1: RemoveCastlingRights(board, CASTLING_Q); break;
            case H8: RemoveCastlingRights(board, CASTLING_k); break;
            case A8: RemoveCastlingRights(board, CASTLING_q); break;
            default: break;
        };
    }
    //King moves
    else if(pieceType == KING && moveType != CASTLING) {
        if(color == WHITE) {
            RemoveCastlingRights(board, CASTLING_K);
            RemoveCastlingRights(board, CASTLING_Q);
        } else if(color == BLACK) {
            RemoveCastlingRights(board, CASTLING_k);
            RemoveCastlingRights(board, CASTLING_q);
        }
    }

    //Rook is captured
    if(capturedType == ROOK) {
        switch(toSq) {
            case H1: RemoveCastlingRights(board, CASTLING_K); break;
            case A1: RemoveCastlingRights(board, CASTLING_Q); break;
            case H8: RemoveCastlingRights(board, CASTLING_k); break;
            case A8: RemoveCastlingRights(board, CASTLING_q); break;
            default: break;
        };
    }
}

void MoveMaker::RewindCastlingRights(Board& board, const Move& move) {

    if(move.MoveType() == CASTLING) {
        COLORS color = board.ActivePlayer();
        switch(move.ToSq()) {
            case G1: MovePiece(board, F1, H1, color, ROOK); break;
            case C1: MovePiece(board, D1, A1, color, ROOK); break;
            case G8: MovePiece(board, F8, H8, color, ROOK); break;
            case C8: MovePiece(board, D8, A8, color, ROOK); break;
            default: assert(false);
        };
    }

    //Update the zkeys
    U8 changedBits = board.m_castlingRights ^ board.m_history[board.m_ply].castling;
    if(changedBits & 0b0001) board.m_zobristKey.UpdateCastling(CASTLING_K);
    if(changedBits & 0b0010) board.m_zobristKey.UpdateCastling(CASTLING_Q);
    if(changedBits & 0b0100) board.m_zobristKey.UpdateCastling(CASTLING_k);
    if(changedBits & 0b1000) board.m_zobristKey.UpdateCastling(CASTLING_q);

    //Retrieve castling rights
    board.m_castlingRights = board.m_history[board.m_ply].castling;
}

void MoveMaker::AddCastlingRights(Board& board, CASTLING_TYPE castlingType) {
    if(~board.CastlingRights() & castlingType) //if the bit is 0
        ToggleCastlingRights(board, castlingType);
}
void MoveMaker::RemoveCastlingRights(Board& board, CASTLING_TYPE castlingType) {
    if(board.CastlingRights() & castlingType) //if the bit is 1
        ToggleCastlingRights(board, castlingType);
}
void MoveMaker::ToggleCastlingRights(Board& board, CASTLING_TYPE castlingType) {
    board.m_castlingRights ^= castlingType;
    board.m_zobristKey.UpdateCastling(castlingType);
}

Move MoveMaker::StringToMove(const Board& board, std::string input) {

    bool incorrectMoveSize = input.size() < 4 || input.size() > 5;
    if(incorrectMoveSize) {
        std::cout << "Not a move: " << input << " (incorrect move size)" << std::endl;
        assert(false);
    }

    int fromSq = board.SquareToIndex( input.substr(0, 2) );
    int toSq = board.SquareToIndex( input.substr(2, 2) );

    char promLet = '\0';
    if(input.size() == 5)
        promLet = input[4];

    return DescriptiveToMove(board, (SQUARES)fromSq, (SQUARES)toSq, promLet);
}
Move MoveMaker::DescriptiveToMove(const Board& board, SQUARES fromSq, SQUARES toSq, char promLet) {
    Move move;

    PIECE_TYPE activePiece = board.GetPieceAtSquare(board.ActivePlayer(), fromSq);
    PIECE_TYPE capturedPiece = board.GetPieceAtSquare(board.InactivePlayer(), toSq);

    int sign = (board.m_activePlayer == WHITE) ? +1 : -1;

    bool isCastling = (activePiece == KING)
        && (fromSq == E1 || fromSq == E8)
        && (toSq == G8 || toSq == G1 || toSq == C8 || toSq == C1);
    bool isDoublePush = (activePiece == PAWN)
        && (abs(Rank(fromSq) - Rank(toSq)) == 2);
    bool isEnpassant = (activePiece == PAWN)
        && ~board.Piece(board.InactivePlayer(),ALL_PIECES) & SquareBB(toSq) //no piece in the destination square
        && board.Piece(board.InactivePlayer(),PAWN) & ( SquareBB(toSq - 8 * sign) ); //enemy pawn behind the destination square
    bool isPromotion = (activePiece == PAWN)
        && ((Rank(toSq) == RANK8 && board.m_activePlayer == WHITE)
        || (Rank(toSq) == RANK1 && board.m_activePlayer == BLACK));
    
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

    return move;
}
