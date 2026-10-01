#pragma once

#include "Types.hpp"
#include "Board.hpp"
#include <array>

namespace alphaone {

class Evaluation {
public:
    // Centipawn piece values for move ordering / heuristics
    static constexpr int PAWN_VAL   = 100;
    static constexpr int KNIGHT_VAL = 320;
    static constexpr int BISHOP_VAL = 330;
    static constexpr int ROOK_VAL   = 500;
    static constexpr int QUEEN_VAL  = 950;
    static constexpr int KING_VAL   = 20000;

    static int pieceValue(uint8_t piece) noexcept;
    static int pieceSquareScore(uint8_t piece, uint8_t sq, bool endgame) noexcept;

    // Evaluate static position in centipawns (positive = White advantage)
    static int evaluate(const Board& board) noexcept;

private:
    static const std::array<int, 64> MG_PAWN_TABLE;
    static const std::array<int, 64> EG_PAWN_TABLE;
    static const std::array<int, 64> MG_KNIGHT_TABLE;
    static const std::array<int, 64> EG_KNIGHT_TABLE;
    static const std::array<int, 64> MG_BISHOP_TABLE;
    static const std::array<int, 64> EG_BISHOP_TABLE;
    static const std::array<int, 64> MG_ROOK_TABLE;
    static const std::array<int, 64> EG_ROOK_TABLE;
    static const std::array<int, 64> MG_QUEEN_TABLE;
    static const std::array<int, 64> EG_QUEEN_TABLE;
    static const std::array<int, 64> MG_KING_TABLE;
    static const std::array<int, 64> EG_KING_TABLE;
};

} // namespace alphaone
