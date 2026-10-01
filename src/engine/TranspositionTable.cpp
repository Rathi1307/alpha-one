#include "TranspositionTable.hpp"
#include <algorithm>

namespace alphaone {

TranspositionTable::TranspositionTable(size_t size_mb) {
    resize(size_mb);
}

void TranspositionTable::resize(size_t size_mb) {
    if (size_mb < 1) size_mb = 1;
    if (size_mb > 512) size_mb = 512;

    size_t num_entries = (size_mb * 1024 * 1024) / sizeof(TTEntry);
    size_t power_of_two = 1;
    while (power_of_two * 2 <= num_entries) {
        power_of_two *= 2;
    }

    entries_.resize(power_of_two);
    mask_ = power_of_two - 1;
    clear();
}

void TranspositionTable::clear() {
    std::fill(entries_.begin(), entries_.end(), TTEntry{});
    hits_ = 0;
}

bool TranspositionTable::probe(uint64_t key, int depth, int alpha, int beta, int& out_score, Move& out_move, int ply) const noexcept {
    const auto& entry = entries_[key & mask_];
    if (entry.key != key) {
        return false;
    }

    out_move = entry.best_move;

    if (entry.depth >= depth) {
        int score = entry.score;
        // Normalize mate score for current ply
        if (score > SCORE_CHECKMATE - 1000) {
            score -= ply;
        } else if (score < -SCORE_CHECKMATE + 1000) {
            score += ply;
        }

        if (entry.bound == BoundType::Exact) {
            out_score = score;
            ++hits_;
            return true;
        }
        if (entry.bound == BoundType::LowerBound && score >= beta) {
            out_score = score;
            ++hits_;
            return true;
        }
        if (entry.bound == BoundType::UpperBound && score <= alpha) {
            out_score = score;
            ++hits_;
            return true;
        }
    }

    return false;
}

void TranspositionTable::store(uint64_t key, int depth, int score, BoundType bound, Move best_move, int ply) noexcept {
    auto& entry = entries_[key & mask_];

    // Normalize mate score relative to root
    if (score > SCORE_CHECKMATE - 1000) {
        score += ply;
    } else if (score < -SCORE_CHECKMATE + 1000) {
        score -= ply;
    }

    if (entry.key != key || depth >= entry.depth) {
        entry.key = key;
        entry.depth = static_cast<uint8_t>(std::clamp(depth, 0, 255));
        entry.score = static_cast<int16_t>(std::clamp(score, -32767, 32767));
        entry.bound = bound;
        if (!best_move.isNull() || entry.key != key) {
            entry.best_move = best_move;
        }
    }
}

} // namespace alphaone
