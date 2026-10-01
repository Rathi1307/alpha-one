#pragma once

#include <cstdint>
#include <cstring>
#ifdef _MSC_VER
#include <intrin.h>
#endif

namespace alphaone {

inline uint8_t lsbIndex(uint64_t x) noexcept {
#if defined(_MSC_VER)
    unsigned long index;
    _BitScanForward64(&index, x);
    return static_cast<uint8_t>(index);
#elif defined(__GNUC__) || defined(__clang__)
    return static_cast<uint8_t>(__builtin_ctzll(x));
#else
    static const uint8_t index[64] = {
         0,  1, 48,  2, 57, 49, 28,  3,
        61, 58, 50, 42, 38, 29, 17,  4,
        62, 55, 59, 36, 53, 51, 43, 22,
        45, 39, 33, 30, 24, 18, 12,  5,
        63, 47, 56, 27, 60, 41, 37, 16,
        54, 35, 52, 21, 44, 32, 23, 11,
        46, 26, 40, 15, 34, 20, 31, 10,
        25, 14, 19,  9, 13,  8,  7,  6
    };
    static const uint64_t debruijn = 0x03f79d7b4cb0a89ULL;
    return index[((x & -static_cast<int64_t>(x)) * debruijn) >> 58];
#endif
}

inline uint8_t msbIndex(uint64_t x) noexcept {
#if defined(_MSC_VER)
    unsigned long index;
    _BitScanReverse64(&index, x);
    return static_cast<uint8_t>(index);
#elif defined(__GNUC__) || defined(__clang__)
    return static_cast<uint8_t>(63 - __builtin_clzll(x));
#else
    double d = static_cast<double>(x);
    uint64_t bits;
    std::memcpy(&bits, &d, sizeof(bits));
    return static_cast<uint8_t>(((bits >> 52) & 0x7ff) - 1023);
#endif
}

inline uint8_t popcount(uint64_t x) noexcept {
#if defined(_MSC_VER)
    return static_cast<uint8_t>(__popcnt64(x));
#elif defined(__GNUC__) || defined(__clang__)
    return static_cast<uint8_t>(__builtin_popcountll(x));
#else
    uint8_t count = 0;
    while (x) {
        x &= x - 1;
        ++count;
    }
    return count;
#endif
}

} // namespace alphaone
