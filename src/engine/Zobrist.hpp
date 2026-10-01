#pragma once

#include "Types.hpp"
#include <cstdint>
#include <array>

namespace alphaone {

class Zobrist {
public:
    static void init() noexcept;

    static uint64_t pieceSquare(uint8_t piece, uint8_t square) noexcept {
        return board_hash_[piece % 12][square & 63];
    }

    static uint64_t enPassant(uint8_t square) noexcept {
        return passant_hash_[square & 63];
    }

    static uint64_t sideToMove() noexcept { return turn_hash_; }
    static uint64_t whiteKingCastle() noexcept { return wkc_hash_; }
    static uint64_t whiteQueenCastle() noexcept { return wqc_hash_; }
    static uint64_t blackKingCastle() noexcept { return bkc_hash_; }
    static uint64_t blackQueenCastle() noexcept { return bqc_hash_; }

private:
    static bool initialized_;
    static std::array<std::array<uint64_t, 64>, 12> board_hash_;
    static std::array<uint64_t, 64> passant_hash_;
    static uint64_t turn_hash_;
    static uint64_t wkc_hash_;
    static uint64_t wqc_hash_;
    static uint64_t bkc_hash_;
    static uint64_t bqc_hash_;
};

} // namespace alphaone
