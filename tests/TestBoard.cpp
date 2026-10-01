#include "../src/engine/Board.hpp"
#include <iostream>
#include <cassert>

using namespace alphaone;

void testInitialBoard() {
    Board board;
    assert(board.whiteToMove() == true);
    assert(board.whiteKingSquare() == 60);
    assert(board.blackKingSquare() == 4);

    // Check corners
    assert(board.getPiece(56) == WHITE_ROOK);
    assert(board.getPiece(63) == WHITE_ROOK);
    assert(board.getPiece(0) == BLACK_ROOK);
    assert(board.getPiece(7) == BLACK_ROOK);

    // Check pawns
    for (int c = 0; c < 8; ++c) {
        assert(board.getPiece(48 + c) == WHITE_PAWN);
        assert(board.getPiece(8 + c) == BLACK_PAWN);
    }
    std::cout << "[PASS] testInitialBoard\n";
}

void testMakeUndo() {
    Board board;
    uint64_t initial_hash = board.zobristHash();

    // Make e2-e4: 52 -> 36
    Move e2e4{52, 36, EMPTY};
    board.move(e2e4);

    assert(board.getPiece(52) == EMPTY);
    assert(board.getPiece(36) == WHITE_PAWN);
    assert(board.whiteToMove() == false);
    assert(board.zobristHash() != initial_hash);

    // Undo e2-e4
    board.undo();
    assert(board.getPiece(52) == WHITE_PAWN);
    assert(board.getPiece(36) == EMPTY);
    assert(board.whiteToMove() == true);
    assert(board.zobristHash() == initial_hash);

    std::cout << "[PASS] testMakeUndo\n";
}

void testFen() {
    Board board;
    std::string start_fen = board.toFen();
    assert(start_fen.find("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1") != std::string::npos);

    // Set custom FEN
    std::string custom_fen = "r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 2 3";
    bool ok = board.setPosition(custom_fen);
    assert(ok);
    assert(board.whiteToMove() == true);
    assert(board.getPiece(45) == WHITE_KNIGHT); // f3 = 5*8+5 = 45
    assert(board.getPiece(36) == WHITE_PAWN);   // e4 = 4*8+4 = 36
    assert(board.getPiece(28) == BLACK_PAWN);   // e5 = 3*8+4 = 28
    assert(board.getPiece(18) == BLACK_KNIGHT); // c6 = 2*8+2 = 18

    std::cout << "[PASS] testFen\n";
}

int main() {
    std::cout << "Running Board Tests...\n";
    testInitialBoard();
    testMakeUndo();
    testFen();
    std::cout << "All Board Tests Passed!\n";
    return 0;
}
