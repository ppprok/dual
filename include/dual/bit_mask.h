/// @file
/// @brief `bit_mask`: operations on bit sets restricted to a mask.

#pragma once

#include <dual/bit_chunk.h>

namespace dual {

/// @brief Mask applied to bit arrays when performing operations on them.
///
/// Unless stated otherwise, every operation works on `bits & mask` without materialising it,
/// e.g. `count(bits)` is the number of set bits of `bits` that are also set in the mask.
struct bit_mask : bit_chunk {
    /// @brief Use the bits of @p b (a view, not a copy) as the mask.
    inline bit_mask(bit_chunk b) : bit_chunk(b) {}

    /// @brief Shrink `bits.end` to the last block that has a bit in common with the mask.
    /// @note The blocks themselves are not changed; an empty mask makes @p bits empty.
    inline void trim(bit_chunk& bits) {
        assert(valid());
        assert(bits.valid());

        if (bits.empty())
            return;

        if (empty()) {
            bits.end = bits.begin;
            return;
        }

        int s = bits.size();
        int s1 = size();
        if (s > s1)
            s = s1;

        for (auto d = bits.begin + s, e = begin + s; e != begin;) {
            --d;
            --e;
            if (*d & *e) {
                bits.end = d + 1;
                return;
            }
        }

        bits.end = bits.begin;
    }

    /// @brief Check whether `bits & mask` is empty.
    inline bool none(bit_chunk const& bits) const {
        assert(valid());
        assert(bits.valid());

        const int n = min_size(size(), bits.size());
        for (int k = 0; k < n; ++k)
            if (begin[k] & bits.begin[k])
                return false;
        return true;
    }

    /// @brief Index of the highest bit of `bits & mask`, or -1 if it is empty.
    inline int find_last(bit_chunk const& bits) const {
        assert(valid());
        assert(bits.valid());

        int s = size();
        int s1 = bits.size();
        if (s > s1)
            s = s1;

        for (auto d = begin + s, e = bits.begin + s; d != begin;) {
            --d;
            --e;
            auto b = *d & *e;
            if (b)
                return get_last(b) + ((d - begin) << low_index_bit_count);
        }

        return -1;
    }

    /// @brief Compare the number of set bits after applying the mask.
    /// @return `true` if `count(r1) < count(r2)`.
    inline bool less_count(bit_chunk const& r1, bit_chunk const& r2) const {
        assert(valid());
        assert(r1.valid());
        assert(r2.valid());

        int c2 = count(r2);
        return c2 && count(r1, c2) < c2;
    }

    /// @brief `bit_chunk::operator<` applied to `r1 & mask` and `r2 & mask`.
    inline bool less_bits(bit_chunk const& r1, bit_chunk const& r2) const {
        assert(valid());
        assert(r1.valid());
        assert(r2.valid());

        const int s1 = min_size(size(), r1.size());
        const int s2 = min_size(size(), r2.size());
        const int n = min_size(s1, s2);
        for (int k = 0; k < n; ++k) {
            auto b1 = begin[k] & r1.begin[k];
            auto b2 = begin[k] & r2.begin[k];
            if (b1 != b2)
                return b1 < b2;
        }

        for (int k = n; k < s1; ++k)
            if (begin[k] & r1.begin[k])
                return false;

        for (int k = n; k < s2; ++k)
            if (begin[k] & r2.begin[k])
                return true;

        return false;
    }

    /// @brief Check whether `r1 & mask == r2 & mask`.
    inline bool equal_bits(bit_chunk const& r1, bit_chunk const& r2) const {
        assert(valid());
        assert(r1.valid());
        assert(r2.valid());

        const int s1 = min_size(size(), r1.size());
        const int s2 = min_size(size(), r2.size());
        const int n = min_size(s1, s2);
        for (int k = 0; k < n; ++k)
            if ((begin[k] & r1.begin[k]) != (begin[k] & r2.begin[k]))
                return false;

        for (int k = n; k < s1; ++k)
            if (begin[k] & r1.begin[k])
                return false;

        for (int k = n; k < s2; ++k)
            if (begin[k] & r2.begin[k])
                return false;

        return true;
    }

    /// @brief Compare `b1 & mask` and `b2 & mask` by inclusion.
    /// @return -1 if `b1 & mask` is a proper subset of `b2 & mask`, 1 if `b2 & mask` is a subset of
    ///         `b1 & mask` (including equality), 0 if they are incomparable.
    /// @note Unlike `bit_chunk::subset_cmp`, equal sets give 1.
    inline int subset_cmp(bit_chunk const& b1, bit_chunk const& b2) const {
        assert(valid());
        assert(b1.valid());
        assert(b2.valid());

        const int s1 = min_size(size(), b1.size());
        const int s2 = min_size(size(), b2.size());
        const int n = min_size(s1, s2);
        int r = 0;

        for (int k = 0; k < n; ++k) {
            auto a1 = b1.begin[k] & begin[k];
            auto a2 = b2.begin[k] & begin[k];
            if (a1 == a2)
                continue;

            if ((a2 & a1) == a1) {
                if (r == 1)
                    return 0;
                r = -1;
                continue;
            }

            if ((a1 & a2) == a2) {
                if (r == -1)
                    return 0;
                r = 1;
                continue;
            }

            return 0;
        }

        if (r != 1)
            for (int k = n; k < s1; ++k)
                if (b1.begin[k] & begin[k])
                    return r + 1;

        if (r != -1)
            for (int k = n; k < s2; ++k)
                if (b2.begin[k] & begin[k])
                    return r - 1;

        if (r == 0)
            return 1;

        return r;
    }

    /// @brief Check whether the mask, @p b1 and @p b2 have a common bit.
    inline bool intersects(bit_chunk const& b1, bit_chunk const& b2) const {
        assert(valid());
        assert(b1.valid());
        assert(b2.valid());

        const int n = min_size(size(), min_size(b1.size(), b2.size()));
        for (int k = 0; k < n; ++k)
            if (begin[k] & b1.begin[k] & b2.begin[k])
                return true;
        return false;
    }

    /// @brief Number of set bits of `bits & mask`.
    inline int count(bit_chunk const& bits) const {
        assert(valid());
        assert(bits.valid());

        const int n = min_size(size(), bits.size());
        int c = 0;
        for (int k = 0; k < n; ++k)
            c += bits_count(begin[k] & bits.begin[k]);
        return c;
    }

    /// @brief Number of set bits of `bits & mask`, with early exit.
    /// @return The exact count if it is at most @p maxc; otherwise some value greater than @p maxc.
    inline int count(bit_chunk const& bits, int maxc) const {
        assert(valid());
        assert(bits.valid());

        const int n = min_size(size(), bits.size());
        int c = 0;
        for (int k = 0; k < n; ++k) {
            c += bits_count(begin[k] & bits.begin[k]);
            if (c > maxc)
                break;
        }
        return c;
    }

    /// @brief Check whether `bits & mask` is not empty.
    inline bool any(bit_chunk const& bits) const {
        return !none(bits);
    }

    /// @brief Index of the lowest bit of `bits & mask` greater than @p k (`k >= -1`), or -1.
    inline int find_next(bit_chunk const& bits, int k) const {
        assert(valid());
        assert(bits.valid());
        assert(k >= -1);

        const int n = min_size(size(), bits.size());
        int h = (k + 1) >> low_index_bit_count;
        if (h >= n)
            return -1;

        int r = (k + 1) & low_index_bit_mask;
        auto b = (begin[h] & bits.begin[h]) >> r;
        if (b)
            return (h << low_index_bit_count) + r + get_first(b);

        for (++h; h < n; ++h) {
            b = begin[h] & bits.begin[h];
            if (b)
                return (h << low_index_bit_count) + get_first(b);
        }

        return -1;
    }

    /// @brief Index of the lowest bit of `bits & mask`, or -1 if it is empty.
    inline int find_first(bit_chunk const& bits) const {
        assert(valid());
        assert(bits.valid());

        const int n = min_size(size(), bits.size());
        for (int h = 0; h < n; ++h) {
            auto b = begin[h] & bits.begin[h];
            if (b)
                return (h << low_index_bit_count) + get_first(b);
        }

        return -1;
    }

    /// @brief Find the first difference: the lowest bit of `mask ^ bits` (the mask itself is
    ///        compared, not applied), or -1 if they are equal.
    inline int find_first_diff(bit_chunk const& bits) const {
        assert(valid());
        assert(bits.valid());

        int i = 0;
        auto d = begin, e = bits.begin;
        for (; d != end && e != bits.end; ++d, ++e, i += bits_per_block) {
            auto b = *d ^ *e;
            if (b)
                return get_first(b) + i;
        }

        for (; d != end; ++d, i += bits_per_block) {
            auto b = *d;
            if (b)
                return get_first(b) + i;
        }

        for (; e != bits.end; ++e, i += bits_per_block) {
            auto b = *e;
            if (b)
                return get_first(b) + i;
        }

        return -1;
    }

    /// @brief The lowest bit of `mask ^ bits` greater than @p k (`k >= -1`), or -1.
    inline int find_next_diff(bit_chunk const& bits, int k) const {
        assert(valid());
        assert(bits.valid());
        assert(k >= -1);

        int h = (k + 1) >> low_index_bit_count;
        auto d = begin + h;
        auto e = bits.begin + h;
        //assert(d <= end || e <= bits.end);

        if (d >= end) {
            if (e >= bits.end)
                return -1;
            return bits.find_next(k);
        }

        if (e >= bits.end)
            return bit_chunk::find_next(k);

        int i = h << low_index_bit_count;
        int r = (k + 1) & low_index_bit_mask;
        auto b = (*d ^ *e) >> r;
        if (b)
            return i + r + get_first(b);
        i += bits_per_block;
        ++d;
        ++e;

        for (; d != end && e != bits.end; ++e, ++d, i += bits_per_block) {
            auto b = *d ^ *e;
            if (b)
                return i + get_first(b);
        }

        for (; d != end; ++d, i += bits_per_block) {
            auto b = *d;
            if (b)
                return i + get_first(b);
        }

        for (; e != bits.end; ++e, i += bits_per_block) {
            auto b = *e;
            if (b)
                return i + get_first(b);
        }

        return -1;
    }
};

}  // namespace dual
