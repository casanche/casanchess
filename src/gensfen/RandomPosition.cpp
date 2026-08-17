#include "gensfen/RandomPosition.h"

#include "Attacks.h"
#include "BitboardUtils.h"

#include <iterator>

namespace {
    constexpr Bitboard LIGHT_SQUARES = 0x55AA55AA55AA55AA;
    constexpr Bitboard DARK_SQUARES = 0xAA55AA55AA55AA55;
}

RandomPositionGenerator::RandomPositionGenerator(uint64_t seed)
    : m_rng(seed) {
}

std::string RandomPositionGenerator::GenerateV2(Board& board, const RandomPositionConfig& config) {
    constexpr int MAX_PIECES[8] = {0, 8, 2, 2, 2, 1, 1, 0};  // -, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, -

    constexpr Bitboard WHITE_HALF = MaskRank[RANK1] | MaskRank[RANK2] | MaskRank[RANK3] | MaskRank[RANK4];
    constexpr Bitboard BLACK_HALF = MaskRank[RANK5] | MaskRank[RANK6] | MaskRank[RANK7] | MaskRank[RANK8];

    bool validPosition = false;

    do {
        board.ClearBits();
        Bitboard emptySquares = ALL;

        // === Kings ===
        int kingSquare_w = PopBiasedSquare(emptySquares, WHITE_HALF, 66);
        board.PutPiece(WHITE, KING, kingSquare_w);

        Bitboard kingMask_b = emptySquares & ~Attacks::AttacksKing(kingSquare_w);
        int kingSquare_b = PopBiasedSquare(kingMask_b, BLACK_HALF, 66);
        board.PutPiece(BLACK, KING, kingSquare_b);

        emptySquares ^= SquareBB(kingSquare_b);

        // === Pawns ===
        for(int c = WHITE; c <= BLACK; c++) {
            COLOR color = (COLOR)c;
            int pawnCount = m_rng.Random32(0, MAX_PIECES[PAWN]);

            Bitboard validPawnMask = emptySquares & ClearRank[RANK1] & ClearRank[RANK8];
            int pawnsOnFile[8] = {0};

            for(int i = 0; i < pawnCount; i++) {
                Bitboard biasMask = validPawnMask;

                // Bias: pawns on 7th
                if(m_rng.Random32(0, 100) > config.advancedPawnBias)
                    biasMask &= ~RelativeMaskRank(color, RANK7);

                // Bias: doubled, tripled pawns
                for(int file = FILEA; file <= FILEH; file++) {
                    int howMany = pawnsOnFile[file];
                    if(howMany == 1 && m_rng.Random32(0, 100) > config.doubledPawnBias)
                        biasMask &= ClearFile[file];
                    if(howMany >= 2 && m_rng.Random32(0,100) > config.tripledPawnBias)
                        biasMask &= ClearFile[file];
                }

                int square = PopRandomSquare(biasMask);
                if(square == -1) break;

                board.PutPiece(color, PAWN, square);
                validPawnMask ^= SquareBB(square);
                emptySquares ^= SquareBB(square);

                pawnsOnFile[File(square)]++;
            }
        }

        // === Pieces ===
        for(int c = WHITE; c <= BLACK; c++) {
            COLOR color = (COLOR)c;

            for(PIECE_TYPE piece = KNIGHT; piece <= QUEEN; ++piece) {
                int pieceCount = m_rng.Random32(0, MAX_PIECES[piece]);

                for(int i = 0; i < pieceCount; i++) {
                    Bitboard biasMask = emptySquares;

                    // Bias: bishop on same-color squares
                    if(piece == BISHOP) {
                        Bitboard existingBishop = board.GetPieces(color, BISHOP);
                        if(existingBishop && m_rng.Random32(0, 100) > config.sameBishopsBias) {
                            bool existingLightSquares = (existingBishop & LIGHT_SQUARES) ? true : false;
                            biasMask &= existingLightSquares ? DARK_SQUARES : LIGHT_SQUARES;
                        }
                    }

                    int square = PopRandomSquare(biasMask);
                    if (square == -1) break;

                    board.PutPiece(color, piece, square);
                    emptySquares ^= SquareBB(square);
                }
            }
        }

        board.m_activePlayer = m_rng.Random32(0, 1) ? WHITE : BLACK;

        // === Validate ===
        Bitboard inactiveKing = board.GetPieces(board.InactivePlayer(), KING);
        if(!board.IsAttacked(board.InactivePlayer(), BitscanForward(inactiveKing))) {
            validPosition = true;
        }

    } while(!validPosition);

    board.InitStateAndHistory();

    return Fen::GetFen(board);
}

int RandomPositionGenerator::PopRandomSquare(Bitboard& mask) {
    if (!mask) return -1;
    
    int r = m_rng.Random32(0, PopCount(mask) - 1);

    // Count 'r' times until reaching the square position
    Bitboard candidates = mask;
    for(int i = 0; i < r; i++)
        RemoveLsb(candidates);

    int square = BitscanForward(candidates);
    
    mask ^= SquareBB(square); // Turn-off the bit
    return square;
}

int RandomPositionGenerator::PopBiasedSquare(Bitboard& globalMask, Bitboard preferredMask, uint biasPercent) {
    Bitboard preferred = globalMask & preferredMask;
    Bitboard notPreferred = globalMask & ~preferredMask;

    bool usePreferred = false;
    if (preferred && notPreferred) {
        usePreferred = (m_rng.Random32(0, 99) < biasPercent);
    } else if (preferred) {
        usePreferred = true;
    } else if (notPreferred) {
        usePreferred = false;
    } else {
        return -1;
    }

    int square = usePreferred ? PopRandomSquare(preferred) : PopRandomSquare(notPreferred);
    globalMask ^= SquareBB(square);
    return square;
}
