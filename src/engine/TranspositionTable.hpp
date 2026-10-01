#pragma once

#include "Types.hpp"
#include "Move.hpp"
#include <vector>
#include <cstdint>

namespace alphaone {

enum class BoundType : uint8_t {
    Exact      = 0,
    LowerBound = 1, // Beta cutoff / fail-high
    UpperBound = 2  // Alpha / fail-low
};

struct TTEntry {
    uint64_t key = 0ULL;
    int16_t score = 0;
    uint8_t depth = 0;
    BoundType bound = BoundType::Exact;
    Move best_move{};
};

class TranspositionTable {
public:
    explicit TranspositionTable(size_t size_mb = 16);

    void resize(size_t size_mb);
    void clear();

    bool probe(uint64_t key, int depth, int alpha, int beta, int& out_score, Move& out_move, int ply = 0) const noexcept;
    void store(uint64_t key, int depth, int score, BoundType bound, Move best_move, int ply = 0) noexcept;

    size_t size() const noexcept { return entries_.size(); }
    uint64_t hits() const noexcept { return hits_; }
    void resetStats() noexcept { hits_ = 0; }

private:
    std::vector<TTEntry> entries_;
    size_t mask_ = 0;
    mutable uint64_t hits_ = 0;
};

} // namespace alphaone
