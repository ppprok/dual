/// @file
/// @brief Bit-counting helpers and per-byte lookup tables.

#pragma once

#include <cstdint>

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

/// @brief Lookup tables for per-byte bit operations (defined in `bits.cpp`).
struct BitOpTables {
    /// Table with the number of set bits in each byte.
    static const int bit_count[256];

    /// Table with the index of the lowest set bit in each byte.
    static const int low_bit_number[256];

    /// Table with each byte after clearing its lowest set bit.
    static const byte reset_low_bit[256];

    /// Table with the index of the highest set bit in each byte.
    static const int high_bit_number[256];

    /// Table with each byte after clearing its highest set bit.
    static const byte reset_high_bit[256];
};

/// @brief Number of set bits in a 32-bit word.
inline int bits_count(bits32 bits) {
    return BitOpTables::bit_count[bits & 0xff] + BitOpTables::bit_count[(bits >> 8) & 0xff]
           + BitOpTables::bit_count[(bits >> 16) & 0xff] + BitOpTables::bit_count[bits >> 24];
}

/// @brief Number of set bits in a 64-bit word.
inline int bits_count(bits64 bits) {
    return BitOpTables::bit_count[bits & 0xff] + BitOpTables::bit_count[(bits >> 8) & 0xff]
           + BitOpTables::bit_count[(bits >> 16) & 0xff] + BitOpTables::bit_count[(bits >> 24) & 0xff]
           + BitOpTables::bit_count[(bits >> 32) & 0xff] + BitOpTables::bit_count[(bits >> 40) & 0xff]
           + BitOpTables::bit_count[(bits >> 48) & 0xff] + BitOpTables::bit_count[bits >> 56];
}

/// @brief Iterator over the set bits of an integer word, from the lowest one.
///
/// Converts to `true` while positioned on a set bit; `int` conversion gives the bit index.
/// Typical loop: `for (low_bit<bits32> lb(bits); lb; ++lb) use(int(lb));`.
/// @tparam T Unsigned integer type.
template<typename T>
struct low_bit {
    /// Index of the current bit, or -1 when there are no more set bits.
    int bit;
    /// Bits that remain to be visited, relative to the byte holding `bit`.
    T rest_bits;

    /// @brief Position on the lowest set bit of @p bits.
    low_bit(T bits) {
        find_bit(bits, 0);
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
        find_bit(rest_bits, bit ^ (bit & 0x07));
        return *this;
    }

private:
    void find_bit(T bits, int from) {
        if (!bits) {
            bit = -1;
            rest_bits = 0;
            return;
        }

        bit = from;
        while (!(bits & 0xff)) {
            bits >>= 8;
            bit += 8;
        }

        auto low = bits & 0xff;
        bit += BitOpTables::low_bit_number[low];
        rest_bits = bits ^ low;
        rest_bits |= BitOpTables::reset_low_bit[low];
    }
};

/// @brief Iterator positioned on the lowest set bit of @p bits.
template<typename T>
low_bit<T> find_first(T const& bits) {
    return bits;
}

//template <typename BitMatrix>
//void read_bit_matrix(std::string const& filename, BitMatrix& bitmatrix)
//{
//    CSVLoader csv;
//    csv.Load(filename);
//    //TODO:
//}

}  // namespace dual
