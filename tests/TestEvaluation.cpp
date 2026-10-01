#include "../src/engine/Board.hpp"
#include "../src/engine/Evaluation.hpp"
#include <iostream>
#include <cassert>

using namespace alphaone;

void testInitialEvaluation() {
    Board board;
    int eval = Evaluation::evaluate(board);
    // Initial chess position is completely symmetric: evaluation must be 0
    std::cout << "Initial position evaluation: " << eval << "\n";
    assert(eval == 0);
    std::cout << "[PASS] testInitialEvaluation: Exactly 0\n";
}

void testMaterialDeltas() {
    Board board;
    // Remove Black Queen at square d8 (sq = 3)
    board.setPiece(EMPTY, 3);
    int eval = Evaluation::evaluate(board);
    std::cout << "White +Q advantage eval: " << eval << "\n";
    assert(eval > 900);
    std::cout << "[PASS] testMaterialDeltas\n";
}

int main() {
    std::cout << "Running Evaluation Tests...\n";
    testInitialEvaluation();
    testMaterialDeltas();
    std::cout << "All Evaluation Tests Passed!\n";
    return 0;
}
