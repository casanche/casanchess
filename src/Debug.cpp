#include "Board.h"
#include "Constants.h"
#include "Debug.h"

#include <iostream>

bool BoardIdentity::operator==(const BoardIdentity& rhs) const {
    if( activePlayer != rhs.activePlayer ||
        zkey != rhs.zkey ||
        pawnKey != rhs.pawnKey ||
        castlingRights != rhs.castlingRights ||
        enPassantSquare != rhs.enPassantSquare ||
        allPieces != rhs.allPieces ||
        moveNumber != rhs.moveNumber ||
        ply != rhs.ply ||
        fiftyRule != rhs.fiftyRule
    ) {
        return false;
    }
    for(COLOR color : {WHITE, BLACK}) {
        for(PIECE_TYPE piece = NO_PIECE; piece <= KING; ++piece) {
            if(pieces[color][piece] != rhs.pieces[color][piece]) {
                return false;
            }
        }
    }
    return true;
}

std::ostream& operator<<(std::ostream& os, const BoardIdentity& boardIdentity) {
    os << "BoardIdentity: {"
       << "activePlayer=" << (boardIdentity.activePlayer ? "WHITE" : "BLACK") << ", "
       << "zkey=" << boardIdentity.zkey << ", "
       << "pawnKey=" << boardIdentity.pawnKey << ", "
       << "castlingRights=" << (int)boardIdentity.castlingRights << ", "
       << "enPassantSquare=" << boardIdentity.enPassantSquare << ", "
       << "allPieces=" << boardIdentity.allPieces << ", "
       << "moveNumber=" << boardIdentity.moveNumber << ", "
       << "ply=" << boardIdentity.ply << ", "
       << "fiftyRule=" << (int)boardIdentity.fiftyRule << "}, {";
    for(COLOR c : {WHITE, BLACK}) {
        for(PIECE_TYPE pt = NO_PIECE; pt <= KING; ++pt) {
            os << "p[" << c << "][" << pt << "]=" << boardIdentity.pieces[c][pt] << ", ";
        }
    }
    os << "}";
    return os;
}

bool BoardIntegrityChecker::CheckIntegrity(const Board& board) {
    // Bitboard data consistency
    bool bitboardIntegrity = true;

    Bitboard allPiecesFromLists = ZERO;
    for (COLOR c : {WHITE, BLACK}) {
        for (PIECE_TYPE pt = PAWN; pt <= KING; ++pt) {
            const Bitboard piece_bb = board.Piece(c, pt);
            assert((allPiecesFromLists & piece_bb) == ZERO); // Check that the pieces do not overlap
            allPiecesFromLists |= piece_bb;
        }
    }
    bitboardIntegrity &= (allPiecesFromLists == board.AllPieces()); // m_allpieces should match piece-lists
 
    // Chess rules consistency
    return bitboardIntegrity &&
        (board.Piece(WHITE, PAWN) & (MaskRank[RANK1] | MaskRank[RANK8])) == 0 &&
        (board.Piece(BLACK, PAWN) & (MaskRank[RANK1] | MaskRank[RANK8])) == 0 &&
        OnlyOne( board.Piece(WHITE, KING) ) &&
        OnlyOne( board.Piece(BLACK, KING) ) &&
        PopCount( board.Piece(WHITE, PAWN) ) <= 8 &&
        PopCount( board.Piece(BLACK, PAWN) ) <= 8 &&
        PopCount( board.EnPassantSquare() ) <= 1 &&
        board.Ply() < MAX_PLY_HISTORY &&
        board.ActivePlayer() != NO_COLOR;
}

BoardIdentity BoardIntegrityChecker::GenerateBoardIdentity(const Board& board) {
    BoardIdentity boardIdentity;

    boardIdentity.activePlayer = board.ActivePlayer();
    boardIdentity.zkey = board.ZKey();
    boardIdentity.pawnKey = board.PawnKey();
    boardIdentity.castlingRights = board.CastlingRights();
    boardIdentity.enPassantSquare = board.EnPassantSquare();

    boardIdentity.allPieces = board.AllPieces();
    for (COLOR c : {WHITE, BLACK}) {
        for (PIECE_TYPE pt = NO_PIECE; pt <= KING; ++pt) {
            boardIdentity.pieces[c][pt] = board.Piece(c, pt);
        }
    }

    boardIdentity.moveNumber = board.MoveNumber();
    boardIdentity.ply = board.Ply();
    boardIdentity.fiftyRule = board.FiftyRule();

    return boardIdentity;
}

void SearchDebug::Increment(const std::string& theVariable) {
    debugVariables[theVariable]++;
};

void SearchDebug::Print() {
    for(auto variable : debugVariables) {
        P("\t " << variable.first << " " << variable.second);
    }
}
