#pragma once

#include "Types.hpp"
#include <cstdint>
#include <string>
#include <string_view>

namespace alphaone {

struct Move {
    uint8_t from = 0;
    uint8_t to = 0;
    uint8_t promotion = EMPTY;

    constexpr Move() noexcept : from(0), to(0), promotion(EMPTY) {}
    constexpr Move(uint8_t f, uint8_t t, uint8_t prom = EMPTY) noexcept
        : from(f), to(t), promotion(prom) {}

    bool isNull() const noexcept {
        return from == 0 && to == 0 && promotion == EMPTY;
    }

    bool operator==(const Move& rhs) const noexcept {
        return from == rhs.from && to == rhs.to && promotion == rhs.promotion;
    }

    bool operator!=(const Move& rhs) const noexcept {
        return !(*this == rhs);
    }

    // Convert to UCI string (e.g., "e2e4", "e7e8q")
    std::string toUci() const {
        if (isNull()) return "0000";
        std::string uci = squareToAlgebraic(from) + squareToAlgebraic(to);
        if (promotion != EMPTY) {
            if (promotion == WHITE_QUEEN || promotion == BLACK_QUEEN) uci += 'q';
            else if (promotion == WHITE_ROOK || promotion == BLACK_ROOK) uci += 'r';
            else if (promotion == WHITE_BISHOP || promotion == BLACK_BISHOP) uci += 'b';
            else if (promotion == WHITE_KNIGHT || promotion == BLACK_KNIGHT) uci += 'n';
        }
        return uci;
    }

    static Move fromUci(std::string_view uci, uint8_t turn = WHITE) {
        if (uci.size() < 4) return Move{};
        int from_sq = algebraicToSquare(uci.substr(0, 2));
        int to_sq = algebraicToSquare(uci.substr(2, 2));
        if (from_sq < 0 || to_sq < 0) return Move{};

        uint8_t prom = EMPTY;
        if (uci.size() >= 5) {
            char p = uci[4];
            if (turn == WHITE) {
                if (p == 'q' || p == 'Q') prom = WHITE_QUEEN;
                else if (p == 'r' || p == 'R') prom = WHITE_ROOK;
                else if (p == 'b' || p == 'B') prom = WHITE_BISHOP;
                else if (p == 'n' || p == 'N') prom = WHITE_KNIGHT;
            } else {
                if (p == 'q' || p == 'Q') prom = BLACK_QUEEN;
                else if (p == 'r' || p == 'R') prom = BLACK_ROOK;
                else if (p == 'b' || p == 'B') prom = BLACK_BISHOP;
                else if (p == 'n' || p == 'N') prom = BLACK_KNIGHT;
            }
        }
        return Move{static_cast<uint8_t>(from_sq), static_cast<uint8_t>(to_sq), prom};
    }
};

} // namespace alphaone
