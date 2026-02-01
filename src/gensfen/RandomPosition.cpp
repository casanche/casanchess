#include "gensfen/RandomPosition.h"

#include "BitboardUtils.h"

namespace {
    const Bitboard LIGHT_SQUARES = 0x55AA55AA55AA55AA;
    const Bitboard DARK_SQUARES = 0xAA55AA55AA55AA55;

    bool HasBishopOnSameColor(const Board& board, COLOR c, Bitboard newBishop) {
        Bitboard existingBishops = board.Piece(c, BISHOP);
        return ((existingBishops & LIGHT_SQUARES) && (newBishop & LIGHT_SQUARES)) 
            || ((existingBishops & DARK_SQUARES) && (newBishop & DARK_SQUARES));
    }
}

RandomPositionGenerator::RandomPositionGenerator(uint64_t seed) 
    : m_rng(seed) {
}

std::string RandomPositionGenerator::Generate(Board& board, const RandomPositionConfig& config) {
    const int MAX_PIECES[8] = {0, 8, 2, 2, 2, 1, 1, 0};  // Per piece type
    
    do {
        board.ClearBits();

        for (int c = WHITE; c <= BLACK; c++) {
            for (int p = PAWN; p <= KING; p++) {
                const int maxCount = (p == KING) ? 1 : m_rng.Random(0, MAX_PIECES[p]);

                for (int count = 0; count < maxCount; count++) {
                    int square;
                    bool valid;
                    int tries = 0;
                    
                    do {
                        tries++;
                        square = RandomSquareForColor(static_cast<COLOR>(c));
                        
                        if (p == PAWN)
                            valid = IsValidPawnSquare(board, static_cast<COLOR>(c), square, config);
                        else
                            valid = IsValidPieceSquare(board, static_cast<COLOR>(c), p, square);
                            
                    } while (!valid && tries < 100);
                    
                    if (tries >= 100)
                        continue;  // Skip this piece if can't find valid square
                    
                    board.m_pieces[c][p] |= SquareBB(square);
                    board.m_allpieces |= SquareBB(square);
                }
            }
        }
        
    } while (board.IsCheckAnyColor() 
             || !board.GetPieces(WHITE, KING) 
             || !board.GetPieces(BLACK, KING));

    // Random side to move
    board.m_activePlayer = m_rng.Random(0, 1) ? WHITE : BLACK;
    board.InitStateAndHistory();

    return Fen::GetSimplifiedFen(board);
}

bool RandomPositionGenerator::IsValidPawnSquare(const Board& board, COLOR color, int square,
                                                 const RandomPositionConfig& config) {
    Bitboard squareBB = SquareBB(square);
    
    // Basic validity: not occupied
    if (squareBB & board.m_allpieces)
        return false;
    
    // Pawns cannot be on ranks 1 or 8
    int rank = Rank(square);
    if (rank == RANK1 || rank == RANK8)
        return false;
    
    // Check 7th rank (advanced pawn)
    if (RelativeRank(color, square) == RANK7) {
        int pawnsOn7th = PopCount(board.Piece(color, PAWN) & RelativeMaskRank(color, RANK7));
        if (pawnsOn7th >= 1 && m_rng.Random(0, 100) > config.advancedPawnBias * 100)
            return false;
    }
    
    // Check doubled/tripled pawns
    int pawnsOnFile = PopCount(board.Piece(color, PAWN) & MaskFile[File(square)]);
    if (pawnsOnFile >= 2 && m_rng.Random(0, 100) > config.tripledPawnBias * 100)
        return false;
    if (pawnsOnFile >= 1 && m_rng.Random(0, 100) > config.doubledPawnBias * 100)
        return false;
    
    return true;
}

bool RandomPositionGenerator::IsValidPieceSquare(const Board& board, COLOR color, int piece, int square) {
    Bitboard squareBB = SquareBB(square);
    
    // Basic validity: not occupied
    if (squareBB & board.m_allpieces)
        return false;
    
    // Bishop: check for same-color squares (currently strict, no bias)
    if (piece == BISHOP && HasBishopOnSameColor(board, color, squareBB))
        return false;
    
    // King position based on game phase (approximated by piece count)
    if (piece == KING) {
        int heavyPieces = PopCount(board.m_allpieces 
                                   ^ board.Piece(WHITE, PAWN) 
                                   ^ board.Piece(BLACK, PAWN));
        int relRank = RelativeRank(color, square);
        
        // Early game (13+ heavy pieces): king on ranks 1-2
        if (heavyPieces >= 13 && relRank > RANK2)
            return false;
        // Middle game (9-12 heavy pieces): king on ranks 1-4
        if (heavyPieces >= 9 && heavyPieces <= 12 && relRank > RANK4)
            return false;
        // End game: king can be anywhere
    }
    
    return true;
}

int RandomPositionGenerator::RandomSquareForColor(COLOR color) {
    // 2/3 chance: piece on own half of board
    // 1/3 chance: piece on opponent's half
    int random = m_rng.Random(1, 3);
    
    if (random == 3) {
        // 1/3: piece on opponent's half
        return (color == WHITE) ? m_rng.Random(32, 63) : m_rng.Random(0, 31);
    } else {
        // 2/3: piece on own half
        return (color == WHITE) ? m_rng.Random(0, 31) : m_rng.Random(32, 63);
    }
}
