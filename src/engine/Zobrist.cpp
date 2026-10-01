#include "Zobrist.hpp"
#include <random>

namespace alphaone {

bool Zobrist::initialized_ = false;
std::array<std::array<uint64_t, 64>, 12> Zobrist::board_hash_{};
std::array<uint64_t, 64> Zobrist::passant_hash_{};
uint64_t Zobrist::turn_hash_ = 0;
uint64_t Zobrist::wkc_hash_ = 0;
uint64_t Zobrist::wqc_hash_ = 0;
uint64_t Zobrist::bkc_hash_ = 0;
uint64_t Zobrist::bqc_hash_ = 0;

void Zobrist::init() noexcept {
    if (initialized_) return;

    // Use deterministic 64-bit Mersenne Twister seed for consistent hashing
    std::mt19937_64 rng(1070372ULL);

    for (int p = 0; p < 12; ++p) {
        for (int sq = 0; sq < 64; ++sq) {
            board_hash_[p][sq] = rng();
        }
    }

    for (int sq = 0; sq < 64; ++sq) {
        passant_hash_[sq] = rng();
    }

    turn_hash_ = rng();
    wkc_hash_  = rng();
    wqc_hash_  = rng();
    bkc_hash_  = rng();
    bqc_hash_  = rng();

    initialized_ = true;
}

} // namespace alphaone
