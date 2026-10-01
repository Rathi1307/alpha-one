#include "Engine.hpp"

namespace alphaone {

Engine::Engine(const SearchConfig& config)
    : config_(config), search_(std::make_unique<Search>(config)) {
    board_.newGame();
}

void Engine::newGame() {
    board_.newGame();
    search_ = std::make_unique<Search>(config_);
}

void Engine::reset() {
    newGame();
}

bool Engine::setPosition(std::string_view fen) {
    bool ok = board_.setPosition(fen);
    search_->clearHistory();
    return ok;
}

std::string Engine::getPosition() const {
    return board_.toFen();
}

std::vector<std::string> Engine::getLegalMoves() {
    auto legalMoves = board_.generateLegalMoves();
    std::vector<std::string> uciMoves;
    uciMoves.reserve(legalMoves.size());
    for (const auto& m : legalMoves) {
        uciMoves.push_back(m.toUci());
    }
    return uciMoves;
}

bool Engine::makeMove(std::string_view uci_move) {
    uint8_t currentTurn = board_.turn();
    Move m = Move::fromUci(uci_move, currentTurn);
    if (m.isNull()) return false;

    if (!board_.isLegal(m)) {
        return false;
    }

    board_.move(m);
    return true;
}

bool Engine::undoMove() {
    if (board_.moveCount() <= 0) return false;
    board_.undo();
    return true;
}

std::string Engine::getBestMove(int time_limit_ms, int max_depth) {
    auto stats = search_->searchBestMove(board_, max_depth, time_limit_ms);
    return stats.best_move_uci;
}

} // namespace alphaone
