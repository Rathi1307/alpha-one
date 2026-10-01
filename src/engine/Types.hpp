#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <array>

namespace alphaone {

// Piece constants matching 0..11 indexing for bitboards
constexpr uint8_t WHITE_PAWN   = 0;
constexpr uint8_t WHITE_KNIGHT = 1;
constexpr uint8_t WHITE_BISHOP = 2;
constexpr uint8_t WHITE_ROOK   = 3;
constexpr uint8_t WHITE_QUEEN  = 4;
constexpr uint8_t WHITE_KING   = 5;
constexpr uint8_t BLACK_PAWN   = 6;
constexpr uint8_t BLACK_KNIGHT = 7;
constexpr uint8_t BLACK_BISHOP = 8;
constexpr uint8_t BLACK_ROOK   = 9;
constexpr uint8_t BLACK_QUEEN  = 10;
constexpr uint8_t BLACK_KING   = 11;
constexpr uint8_t EMPTY        = 12;

constexpr uint8_t WHITE = 13;
constexpr uint8_t BLACK = 14;
constexpr uint8_t DRAW  = 15;

enum class Color : uint8_t {
    White = 0,
    Black = 1,
    None  = 2
};

inline constexpr Color opponentColor(Color c) noexcept {
    return (c == Color::White) ? Color::Black : ((c == Color::Black) ? Color::White : Color::None);
}

inline constexpr uint8_t opponentTurn(uint8_t turn) noexcept {
    return (turn == WHITE) ? BLACK : WHITE;
}

// Coordinate conversions:
// row 0 = rank 8 (Black side, squares 0..7 = a8..h8)
// row 7 = rank 1 (White side, squares 56..63 = a1..h1)
// col 0 = file a, col 7 = file h
// sq = row * 8 + col (0..63)
inline constexpr int makeSquare(int row, int col) noexcept {
    return row * 8 + col;
}

inline constexpr int squareRow(int sq) noexcept {
    return sq / 8;
}

inline constexpr int squareCol(int sq) noexcept {
    return sq % 8;
}

inline constexpr bool isValidSquare(int row, int col) noexcept {
    return row >= 0 && row < 8 && col >= 0 && col < 8;
}

inline constexpr bool isValidSquare(int sq) noexcept {
    return sq >= 0 && sq < 64;
}

// Convert square (0..63) to algebraic "e2" (row 6, col 4 -> 6*8+4 = 52)
inline std::string squareToAlgebraic(int sq) {
    if (!isValidSquare(sq)) return "-";
    int row = squareRow(sq);
    int col = squareCol(sq);
    char file = static_cast<char>('a' + col);
    char rank = static_cast<char>('8' - row);
    return {file, rank};
}

// Convert algebraic "e2" to square (0..63)
inline int algebraicToSquare(std::string_view alg) {
    if (alg.size() < 2) return -1;
    int col = alg[0] - 'a';
    int row = '8' - alg[1];
    if (!isValidSquare(row, col)) return -1;
    return makeSquare(row, col);
}

// Material values (in centipawns)
constexpr int SCORE_CHECKMATE = 100000;
constexpr int SCORE_DRAW      = 0;

} // namespace alphaone
