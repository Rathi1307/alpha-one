#include "../src/engine/Board.hpp"
#include <iostream>
#include <cassert>

using namespace alphaone;

void testInitialMoves() {
    Board board;
    auto moves = board.generateLegalMoves();
    assert(moves.size() == 20);
    std::cout << "[PASS] testInitialMoves: 20 legal moves generated.\n";
}

void testPinsAndChecks() {
    // Scholar's Mate checkmate position:
    Board board("r1bqkb1r/pppp1Qpp/2n2n2/4p3/2B1P3/8/PPPP1PPP/RNB1K1NR b KQkq - 0 4");
    auto moves = board.generateLegalMoves();

    assert(moves.empty());
    assert(board.isCheckmate() == true);
    assert(board.isInCheck() == true);
    std::cout << "[PASS] testPinsAndChecks: Scholar's mate detected correctly (0 moves, checkmate=true).\n";
}

void testEnPassant() {
    // White pawn on e5, Black plays d7-d5
    Board board("rnbqkbnr/ppp1pppp/8/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 3");
    auto moves = board.generateLegalMoves();

    bool found_ep = false;
    for (const auto& m : moves) {
        if (m.toUci() == "e5d6") {
            found_ep = true;
        }
    }
    assert(found_ep);
    std::cout << "[PASS] testEnPassant: En passant capture e5xd6 generated.\n";
}

void testCastling() {
    Board board("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
    auto moves = board.generateLegalMoves();

    bool found_kingside = false;
    bool found_queenside = false;
    for (const auto& m : moves) {
        if (m.toUci() == "e1g1") found_kingside = true;
        if (m.toUci() == "e1c1") found_queenside = true;
    }
    assert(found_kingside);
    assert(found_queenside);
    std::cout << "[PASS] testCastling: Both kingside (e1g1) and queenside (e1c1) castling generated.\n";
}

void testStalemate() {
    // Stalemate position: Black king on a8, White king on c7, White queen on b6
    Board board("k7/2K5/1Q6/8/8/8/8/8 b - - 0 1");
    auto moves = board.generateLegalMoves();
    assert(moves.empty());
    assert(board.isStalemate() == true);
    assert(board.isCheckmate() == false);
    std::cout << "[PASS] testStalemate: Stalemate correctly identified (0 moves, stalemate=true).\n";
}

int main() {
    std::cout << "Running MoveGenerator Tests...\n";
    testInitialMoves();
    testPinsAndChecks();
    testEnPassant();
    testCastling();
    testStalemate();
    std::cout << "All MoveGenerator Tests Passed!\n";
    return 0;
}
