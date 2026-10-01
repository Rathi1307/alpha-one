#pragma once

#include "Types.hpp"
#include "Move.hpp"
#include "Board.hpp"
#include "TranspositionTable.hpp"
#include <chrono>
#include <atomic>
#include <vector>
#include <string>

namespace alphaone {

struct SearchConfig {
    int max_depth = 6;
    int time_limit_ms = 3000;
    size_t tt_size_mb = 16;
    bool use_tt = true;
};

struct SearchStats {
    int score = 0;
    int depth = 0;
    uint64_t nodes = 0;
    int elapsed_ms = 0;
    uint64_t nodes_per_second = 0;
    uint64_t transposition_hits = 0;
    Move best_move{};
    std::string best_move_uci = "0000";
};

class Search {
public:
    explicit Search(const SearchConfig& config = SearchConfig{});

    SearchStats searchBestMove(Board& board, int target_depth = 0, int time_limit_ms = 0);
    void stop() noexcept { stop_flag_.store(true, std::memory_order_relaxed); }

    const SearchStats& lastStats() const noexcept { return stats_; }
    void clearHistory() noexcept;

private:
    bool shouldStop() noexcept;
    void orderMoves(std::vector<Move>& moves, const Board& board, const Move& tt_move, int ply) const noexcept;

    int quiescence(Board& board, int alpha, int beta, int ply);
    int negamax(Board& board, int depth, int alpha, int beta, int ply, bool is_root);

    SearchConfig config_;
    TranspositionTable tt_;
    SearchStats stats_{};
    std::atomic<bool> stop_flag_{false};

    std::chrono::steady_clock::time_point start_time_;
    int effective_time_limit_ms_ = 0;

    Move root_best_move_{};
    Move killer_moves_[64][2]{};
    int history_[12][64]{};
};

} // namespace alphaone
