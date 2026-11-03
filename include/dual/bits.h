/// @file
/// @brief Bit-counting helpers on machine words: number of set bits, lowest and highest set bit.
///
/// The helpers use compiler builtins where they map to one instruction (`__builtin_popcountll` with
/// POPCNT or on ARM, `__builtin_ctzll`/`__builtin_clzll` on GCC and Clang, `_BitScanForward64` and
/// `_BitScanReverse64` on 64-bit MSVC) and portable branch-free fallbacks (`portable_*` in
/// `dual::details`) everywhere else. Without POPCNT (e.g. x86-64 without `-march`) GCC turns
/// `__builtin_popcount*` into a library call, so the SWAR fallback is used instead; build with
/// `DUAL_NATIVE_ARCH=ON` (or `-mpopcnt`) to get the instruction.

#pragma once

#include <cstdint>
#include <type_traits>

#if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_ARM64))
#    include <intrin.h>
#endif

/// @brief Everything in the library: bit containers, matrix IO, dualization callbacks and the
///        algorithms. Algorithm-specific state lives in nested namespaces named after the
///        algorithm: `dual::Runc` (RUNC, RUNC-M, OPT), `dual::AOT1` and `dual::PUNC`. The C ABI of
///        `librunc` (`<dual/runc_executor.h>`) is the only part outside the namespace.
namespace dual {

/// 32-bit word of bits.
typedef uint32_t bits32;

/// 64-bit word of bits.
typedef uint64_t bits64;

/// Byte type.
typedef unsigned char byte;

/// @cond INTERNAL
namespace details {

// Portable fallbacks; also used directly by the tests to check them against the builtins.

constexpr int portable_popcount(uint64_t x) {
    x = x - ((x >> 1) & 0x5555555555555555ULL);
    x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL);
    x = (x + (x >> 4)) & 0x0f0f0f0f0f0f0f0fULL;
    return int((x * 0x0101010101010101ULL) >> 56);
}

// Index of the lowest set bit of a non-zero word: the bits below it, counted.
constexpr int portable_lowest(uint64_t x) {
    return portable_popcount((x & (~x + 1)) - 1);
}

// Index of the highest set bit of a non-zero word: smear it down, count, subtract one.
constexpr int portable_highest(uint64_t x) {
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x |= x >> 32;
    return portable_popcount(x) - 1;
}

inline int popcount64(uint64_t x) {
#if defined(__GNUC__) && (defined(__POPCNT__) || defined(__aarch64__) || defined(__ARM_NEON))
    return __builtin_popcountll(x);
#elif defined(_MSC_VER) && defined(_M_X64) && defined(__AVX__)
    // /arch:AVX and later imply POPCNT
    return int(__popcnt64(x));
#else
    return portable_popcount(x);
#endif
}

inline int lowest64(uint64_t x) {
#if defined(__GNUC__)
    return __builtin_ctzll(x);
#elif defined(_MSC_VER) && (defined(_M_X64) || defined(_M_ARM64))
    unsigned long i = 0;
    _BitScanForward64(&i, x);
    return int(i);
#else
    return portable_lowest(x);
#endif
}

inline int highest64(uint64_t x) {
#if defined(__GNUC__)
    return 63 - __builtin_clzll(x);
#elif defined(_MSC_VER) && (defined(_M_X64) || defined(_M_ARM64))
    unsigned long i = 0;
    _BitScanReverse64(&i, x);
    return int(i);
#else
    return portable_highest(x);
#endif
}

template<typename T>
using enable_if_word = std::enable_if_t<std::is_unsigned<T>::value && sizeof(T) <= sizeof(uint64_t), int>;

}  // namespace details
/// @endcond

/// @brief Number of set bits in an unsigned word (at most 64 bits).
template<typename T, details::enable_if_word<T> = 0>
inline int bits_count(T bits) {
    return details::popcount64(uint64_t(bits));
}

/// @brief Index of the lowest set bit of a **non-zero** unsigned word (at most 64 bits).
template<typename T, details::enable_if_word<T> = 0>
inline int lowest_bit_index(T bits) {
    return details::lowest64(uint64_t(bits));
}

/// @brief Index of the highest set bit of a **non-zero** unsigned word (at most 64 bits).
template<typename T, details::enable_if_word<T> = 0>
inline int highest_bit_index(T bits) {
    return details::highest64(uint64_t(bits));
}

/// @brief Iterator over the set bits of an integer word, from the lowest one.
///
/// Converts to `true` while positioned on a set bit; `int` conversion gives the bit index.
/// Typical loop: `for (low_bit<bits32> lb(bits); lb; ++lb) use(int(lb));`.
/// @tparam T Unsigned integer type (at most 64 bits).
template<typename T>
struct low_bit {
    /// Index of the current bit, or -1 when there are no more set bits.
    int bit;
    /// Bits that remain to be visited, the current one excluded.
    T rest_bits;

    /// @brief Position on the lowest set bit of @p bits.
    low_bit(T bits) {
        find_bit(bits);
    }

    /// @brief `true` while positioned on a set bit.
    operator bool() const {
        return bit != -1;
    }

    /// @brief Index of the current bit, or -1.
    operator int() const {
        return bit;
    }

    /// @brief Move to the next set bit.
    low_bit& operator++() {
        find_bit(rest_bits);
        return *this;
    }

private:
    void find_bit(T bits) {
        if (!bits) {
            bit = -1;
            rest_bits = 0;
            return;
        }
        bit = lowest_bit_index(bits);
        rest_bits = T(bits & (bits - 1));
    }
};

/// @brief Iterator positioned on the lowest set bit of @p bits.
template<typename T>
low_bit<T> find_first(T const& bits) {
    return bits;
}

}  // namespace dual
