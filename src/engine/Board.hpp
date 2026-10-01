#pragma once

#include "Types.hpp"
#include "Move.hpp"
#include "Bitboard.hpp"
#include "Zobrist.hpp"
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <array>

namespace alphaone {

struct BoardState {
    uint64_t bitboards[12];
    bool WKC;
    bool BKC;
    bool WQC;
    bool BQC;
    uint64_t passantTarget;
    uint8_t turn;
    uint8_t halfMoves;
    uint64_t hash;
};

class Board {
public:
    Board();
    explicit Board(std::string_view fen);
    ~Board();

    // Copy / move
    Board(const Board& other);
    Board& operator=(const Board& other);

    // Lifecycle
    void newGame();
    bool setPosition(std::string_view fen);
    std::string toFen() const;

    // Piece access
    uint8_t getPiece(uint8_t square) const noexcept;
    void setPiece(uint8_t piece, uint8_t square) noexcept;

    // State inspection
    uint8_t turn() const noexcept { return stateStack_[stackIndex_].turn; }
    bool whiteToMove() const noexcept { return turn() == WHITE; }
    Color sideToMove() const noexcept { return whiteToMove() ? Color::White : Color::Black; }
    uint64_t zobristHash() const noexcept { return stateStack_[stackIndex_].hash; }
    uint8_t halfMoves() const noexcept { return stateStack_[stackIndex_].halfMoves; }
    int moveCount() const noexcept { return stackIndex_; }

    uint8_t whiteKingSquare() const noexcept { return lsbIndex(stateStack_[stackIndex_].bitboards[WHITE_KING]); }
    uint8_t blackKingSquare() const noexcept { return lsbIndex(stateStack_[stackIndex_].bitboards[BLACK_KING]); }
    uint8_t kingSquare(Color c) const noexcept { return (c == Color::White) ? whiteKingSquare() : blackKingSquare(); }

    const BoardState* getState() const noexcept { return &stateStack_[stackIndex_]; }
    BoardState* getState() noexcept { return &stateStack_[stackIndex_]; }

    // Moves
    void move(const Move& m) noexcept;
    void undo() noexcept;

    void pseudoMoves(Move* moves, int& numMoves) const noexcept;
    std::vector<Move> generateLegalMoves() const;
    bool isLegal(const Move& m) const noexcept;
    bool isAttacked(uint8_t square, uint8_t attackerColor) const noexcept;

    // Game end / checks
    bool isInCheck() const noexcept;
    bool isCheckmate() const noexcept;
    bool isStalemate() const noexcept;
    bool softDraw() const noexcept;
    uint8_t isTerminal() const noexcept;

    // Debugging
    void print() const;

    // Hash computation from scratch
    uint64_t computeHash() const noexcept;

private:
    static void initAttackTables() noexcept;

    static bool tables_initialized_;
    static uint64_t knightAttacks_[64];
    static uint64_t kingAttacks_[64];
    static uint64_t diagonalRays_[64][4];
    static uint64_t cardinalRays_[64][4];

    BoardState* stateStack_ = nullptr;
    int stackIndex_ = 0;
    static constexpr int MAX_STACK = 1024;
};

} // namespace alphaone
