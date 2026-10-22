/// @file
/// @brief Non-owning view of a bit set stored in a range of machine words (`bit_chunk`).

#pragma once

#include <assert.h>
#include <vector>

#include <dual/bits.h>
#include <dual/chunk.h>
#include <dual/raw_vector.h>

namespace dual {

/// @cond INTERNAL
namespace details {

template<int v>
struct log2 {
    static const int value = log2<v / 2>::value + 1;
};

template<>
struct log2<1> {
    static const int value = 0;
};
}  // namespace details
/// @endcond

/// @brief Block with access to individual bits: a non-owning view of a bit set.
///
/// The bits are stored in the blocks `[begin, end)`; bit `i` is bit `i % bits_per_block` of block
/// `i / bits_per_block`. Bits past `end` are treated as zeros, so two chunks that differ only in
/// trailing zero blocks represent the same set. Most operations that write a whole chunk
/// (`assign*`, `flip`, `trim`) shrink `end` to the last non-zero block.
///
/// @warning A `bit_chunk` does not own or allocate memory. Operations that may move `end`
/// forward (`set`, `pad_blocks`, `pad_bits`, `assign*`) write past the current `end`; the caller
/// must guarantee that enough memory is available there (see `bit_vector` and `bit_matrix`,
/// which allocate the storage). Preconditions are checked with `assert` only.
//template <typename BaseChunk>
struct bit_chunk : chunk<unsigned int> {
    /// Byte type.
    typedef unsigned char byte;

    /// Storage word type.
    typedef value_type Block;

    //typedef raw_vector<Block> Blocks;

    /// Number of bits in one block.
    static const int bits_per_block = sizeof(Block) * 8;
    /// Number of low bits of a bit index that address a bit inside a block (log2 of `bits_per_block`).
    static const int low_index_bit_count = details::log2<bits_per_block>::value;
    /// Mask selecting the in-block part of a bit index.
    static const int low_index_bit_mask = bits_per_block - 1;

    /// @brief Block number and mask of a bit, precomputed for repeated `get` calls.
    struct get_bit_iterator {
        /// Index of the block containing the bit.
        int blockNum;
        /// Mask of the bit inside the block (0 for a negative bit index).
        Block bitMask;

        /// @brief Precompute the position of bit @p i; a negative @p i addresses no bit.
        inline get_bit_iterator(int i) {
            if (i < 0) {
                bitMask = 0;
                blockNum = 0;
            } else {
                blockNum = i >> low_index_bit_count;
                bitMask = Block(1) << (i & low_index_bit_mask);
            }
        }
    };

    /// @brief Number of blocks needed to hold @p n bits (rounded up).
    static inline int block_for_bits(int n) {
        int b = n >> low_index_bit_count;
        if (n & low_index_bit_mask)
            return b + 1;
        return b;
    }

    /// @brief Trim to the last set bit: drop trailing zero blocks.
    inline void trim() {
        assert(valid());
        for (auto d = end - 1; begin != end && !*d; --d, --end) {};
    }

    /// @brief If the chunk has fewer than @p s blocks, append blocks filled with @p bit.
    /// @param s Required number of blocks.
    /// @param bit Value of the bits in the appended blocks.
    /// @warning Writes past `end`; memory for @p s blocks must be available.
    inline void pad_blocks(int s, bool bit = false) {
        assert(valid());

        auto end1 = begin + s;
        if (end1 <= end)
            return;
        if (bit)
            do_pad_ones(end1);
        else
            do_pad_zeros(end1);
    }

private:
    inline void do_pad_zeros(Block* end1) {
        assert(end < end1);
        for (; end != end1; ++end)
            *end = 0;
    }

    inline void do_pad_ones(Block* end1) {
        assert(end < end1);
        for (; end != end1; ++end)
            *end = -1;
    }

public:
    /// @brief Check whether no bit is set.
    inline bool none() const {
        assert(valid());
        for (auto i = end; i != begin; --i)
            if (*(i - 1))
                return false;
        return true;
    }

    /// @brief Assign the intersection `r1 & r2`.
    /// @note Writes up to `min(r1.size(), r2.size())` blocks, then trims trailing zero blocks.
    inline void assign_and(bit_chunk const& r1, bit_chunk const& r2) {
        assert(valid());
        assert(r1.valid());
        assert(r2.valid());

        auto endd = begin;
        auto e1 = r1.begin;
        auto e2 = r2.begin;

        for (auto d = begin; e1 != r1.end && e2 != r2.end; ++d, ++e1, ++e2) {
            if (*d = *e1 & *e2)
                endd = d + 1;
        }

        end = endd;
    }

    /// @brief Assign the union `r1 | r2`.
    /// @note Writes up to `max(r1.size(), r2.size())` blocks, then trims trailing zero blocks.
    inline void assign_or(bit_chunk const& r1, bit_chunk const& r2) {
        assert(valid());
        assert(r1.valid());
        assert(r2.valid());

        auto d = begin;
        auto endd = begin;
        auto e1 = r1.begin;
        auto e2 = r2.begin;

        for (; e1 != r1.end && e2 != r2.end; ++d, ++e1, ++e2) {
            if (*d = *e1 | *e2)
                endd = d + 1;
        }

        for (; e1 != r1.end; ++e1, ++d) {
            if (*d = *e1)
                endd = d + 1;
        }

        for (; e2 != r2.end; ++e2, ++d) {
            if (*d = *e2)
                endd = d + 1;
        }
        end = endd;
    }

    /// @brief Assign the difference `r1 & ~r2`.
    /// @note Writes up to `r1.size()` blocks, then trims trailing zero blocks.
    inline void assign_sub(bit_chunk const& r1, bit_chunk const& r2) {
        assert(valid());
        assert(r1.valid());
        assert(r2.valid());

        auto d = begin;
        auto endd = begin;
        auto e1 = r1.begin;
        auto e2 = r2.begin;

        for (; e1 != r1.end && e2 != r2.end; ++d, ++e1, ++e2) {
            if (*d = *e1 & ~*e2)
                endd = d + 1;
        }

        for (; e1 != r1.end; ++e1, ++d) {
            if (*d = *e1)
                endd = d + 1;
        }

        end = endd;
    }

    /// @brief Copy the bits of @p r into this chunk's memory.
    /// @note Writes up to `r.size()` blocks, then trims trailing zero blocks.
    inline void assign(bit_chunk const& r) {
        assert(valid());
        assert(r.valid());

        auto endd = begin;
        for (auto d = begin, e = r.begin; e != r.end; ++e, ++d) {
            if (*d = *e)
                endd = d + 1;
        }
        end = endd;
    }

    /// @brief Invert the bits of the existing blocks `[begin, end)`, then trim trailing zero blocks.
    /// @note Bits past `end` are not affected (they stay zero).
    inline void flip() {
        assert(valid());

        auto cend = end;
        for (auto i = end = begin; i != cend; ++i) {
            if (*i = ~*i)
                end = i + 1;
        }
    }

    /// @brief Get bit @p i.
    /// @return The bit value; `false` for a negative index or an index past `end`.
    inline bool get(int i) const {
        assert(valid());

        if (i < 0)
            return false;
        int h = i >> low_index_bit_count;
        auto d = begin + h;
        if (d >= end)
            return false;

        return (*d & (Block(1) << (i & low_index_bit_mask))) != 0;
    }

    /// @brief Get the bit addressed by a precomputed iterator.
    /// @return The bit value; `false` if the bit lies past `end`.
    inline bool get(get_bit_iterator it) const {
        assert(valid());

        auto d = begin + it.blockNum;
        if (d >= end)
            return false;
        return (*d & it.bitMask) != 0;
    }

    /// @brief Set bit @p i (`i >= 0`).
    ///
    /// If the bit lies past `end`, the chunk is first extended with zero blocks up to and including
    /// the block of bit @p i.
    /// @warning The memory for the extension must be available (no allocation happens here).
    inline void set(int i) {
        assert(i >= 0 && valid());

        int h = i >> low_index_bit_count;

        auto d = begin + h;

        if (d >= end)
            do_pad_zeros(d + 1);

        *d |= get_bit_mask(i);
    }

    /// @brief Clear bit @p i; no-op for a negative index or an index past `end`.
    /// @note Does not trim trailing zero blocks.
    inline void reset(int i) {
        assert(valid());

        if (i < 0)
            return;

        int h = i >> low_index_bit_count;

        auto d = begin + h;

        if (d >= end)
            return;

        *d &= get_bit_inv_mask(i);
    }

    /// @brief Set bit @p i to @p value: `set(i)` if `value`, otherwise `reset(i)`.
    inline void set(int i, bool value) {
        if (value)
            set(i);
        else
            reset(i);
    }

protected:
    /// @brief Index of the lowest set bit of a non-zero block.
    static inline int get_first(Block b) {
        auto b1 = b & 0xff;
        if (b1)
            return BitOpTables::low_bit_number[b1];

        auto b2 = b & 0xff00;
        if (b2)
            return BitOpTables::low_bit_number[b2 >> 8] + 8;

        auto b3 = b & 0xff0000;
        if (b3)
            return BitOpTables::low_bit_number[b3 >> 16] + 16;

        return BitOpTables::low_bit_number[b >> 24] + 24;
    }

    /// @brief Index of the highest set bit of a non-zero block.
    static inline int get_last(Block b) {
        auto b1 = b & 0xff000000;
        if (b1)
            return BitOpTables::high_bit_number[b1 >> 24] + 24;

        auto b2 = b & 0xff0000;
        if (b2)
            return BitOpTables::high_bit_number[b2 >> 16] + 16;

        auto b3 = b & 0xff00;
        if (b3)
            return BitOpTables::high_bit_number[b3 >> 8] + 8;

        return BitOpTables::high_bit_number[b];
    }

public:
    /// @brief Index of the lowest set bit, or -1 if no bit is set.
    inline int find_first() const {
        assert(valid());
        int i = 0;
        for (auto d = begin; d != end; ++d, i += bits_per_block) {
            auto b = *d;
            if (b)
                return i + get_first(b);
        }
        return -1;
    }

    /// @brief Index of the lowest set bit greater than @p k, or -1 if there is none.
    /// @param k Previous position; -1 starts from the beginning (same as `find_first`).
    /// @note Typical loop: `for (int k = find_first(); k != -1; k = find_next(k))`.
    inline int find_next(int k) const {
        assert(valid());
        assert(k >= -1);

        int h = (k + 1) >> low_index_bit_count;
        auto d = begin + h;
        assert(d <= end);
        if (d == end)
            return -1;

        int i = h << low_index_bit_count;
        int r = (k + 1) & low_index_bit_mask;
        auto b = *d >> r;
        if (b)
            return i + r + get_first(b);
        i += bits_per_block;
        ++d;

        for (; d != end; ++d, i += bits_per_block) {
            auto b = *d;
            if (b)
                return i + get_first(b);
        }

        return -1;
    }

    /// @brief Index of the highest set bit, or -1 if no bit is set.
    inline int find_last() const {
        assert(valid());

        for (auto d = end; d != begin;) {
            --d;
            auto b = *d;
            if (b)
                return get_last(b) + ((d - begin) << low_index_bit_count);
        }

        return -1;
    }

    /// @brief Set several bits.
    /// @tparam Iter Iterator over non-negative bit indices.
    template<typename Iter>
    inline void set_bits(Iter b, Iter e) {
        for (; b != e; ++b)
            set(*b);
    }

    /// @brief Set several bits.
    /// @tparam C Container of non-negative bit indices.
    template<typename C>
    inline void set_bits(C const& items) {
        set_bits(std::begin(items), std::end(items));
    }

    /// @brief Mark with bits the objects that satisfy a predicate: bit `j` is set if
    ///        `pred(items[j])` holds.
    /// @note Bits of objects that do not satisfy @p pred are left unchanged.
    template<typename C, typename Pred>
    void set_if(C const& items, Pred pred) {
        int j = 0;
        for (auto const& item : items) {
            if (pred(item))
                set(j);
            ++j;
        }
    }

    /// @brief Get the indices of the set bits in increasing order, appending them to @p bits.
    template<typename C>
    inline void get_bits(C& bits) const {
        for (int k = find_first(); k != -1; k = find_next(k))
            bits.push_back(k);
    }

    /// @brief Select the objects marked with bits: append `items[j]` to @p selected for every set
    ///        bit `j < items.size()`.
    template<typename C1, typename C2>
    void get_selected(C1 const& items, C2& selected) {
        for (int j = find_first(), n = items.size(); j != -1 && j < n; j = find_next(j)) {
            selected.push_back(items[j]);
        }
    }

    /// @brief Strict ordering of bit sets: blocks are compared as unsigned numbers starting from
    ///        the first block; trailing zero blocks are ignored.
    inline bool operator<(bit_chunk const& r) const {
        assert(valid() && r.valid());

        auto q = begin;
        auto e = r.begin;
        for (; q != end && e != r.end; ++q, ++e) {
            if (*q > *e)
                return false;
            if (*q < *e)
                return true;
        }

        for (; q != end; ++q)
            if (*q)
                return false;

        for (; e != r.end; ++e)
            if (*e)
                return true;

        return false;
    }

    /// @brief Equality of bit sets; trailing zero blocks are ignored.
    bool operator==(bit_chunk const& r) const {
        assert(valid() && r.valid());

        auto q = begin;
        auto e = r.begin;
        for (; q != end && e != r.end; ++q, ++e) {
            if (*q != *e)
                return false;
        }

        for (; q != end; ++q)
            if (*q)
                return false;

        for (; e != r.end; ++e)
            if (*e)
                return false;

        return true;
    }

    /// @brief Check whether every bit set here is also set in @p b.
    inline bool is_subset_of(bit_chunk const& b) const {
        assert(valid());
        assert(b.valid());

        auto b1 = begin;
        auto b2 = b.begin;

        for (; b1 != end && b2 != b.end; ++b1, ++b2)
            if ((*b1 & *b2) != *b1)
                return false;

        for (; b1 != end; ++b1)
            if (*b1)
                return false;

        return true;
    }

    /// @brief Compare two bit sets by inclusion.
    /// @return -1 if `b1` is a subset of `b2` (including `b1 == b2`), 1 if `b2` is a proper subset
    ///         of `b1`, 0 if the sets are incomparable.
    static inline int subset_cmp(bit_chunk const& b1, bit_chunk const& b2) {
        assert(b1.valid());
        assert(b2.valid());

        auto d1 = b1.begin, d2 = b2.begin;
        int r = 0;

        for (; d1 != b1.end && d2 != b2.end; ++d1, ++d2) {
            auto a1 = *d1;
            auto a2 = *d2;

            if (a1 == a2)
                continue;

            auto a = a1 & a2;
            if (a == a1) {
                if (r == 1)
                    return 0;
                r = -1;
                continue;
            }

            if (a == a2) {
                if (r == -1)
                    return 0;
                r = 1;
                continue;
            }

            return 0;
        }

        if (r != 1)
            for (; d1 != b1.end; ++d1)
                if (*d1)
                    return r + 1;

        if (r != -1)
            for (; d2 != b2.end; ++d2)
                if (*d2)
                    return r - 1;

        if (r)
            return r;

        // If the two sets are equal, the first one is considered a subset of the second
        return -1;
    }

    /// @brief Check whether this set and @p b have a common bit.
    inline bool intersects(bit_chunk const& b) const {
        assert(valid());
        assert(b.valid());

        for (auto d = begin, e = b.begin; d != end && e != b.end; ++d, ++e)
            if (*d & *e)
                return true;

        return false;
    }

public:
    /// @brief Number of set bits.
    inline int count() const {
        assert(valid());

        int c = 0;
        for (auto d = begin; d != end; ++d) {
            auto b = *d;
            if (b)
                c += bits_count(b);
        }
        return c;
    }

    /// @brief Number of set bits, with early exit.
    /// @return The exact count if it is at most @p maxc; otherwise some value greater than @p maxc.
    inline int count(int maxc) const {
        assert(valid());

        int c = 0;
        for (auto d = begin; d != end; ++d) {
            auto b = *d;
            if (b) {
                c += bits_count(b);
                if (c > maxc)
                    return c;
            }
        }
        return c;
    }

    /// @brief Number of bits in the blocks `[begin, end)` (`size() * bits_per_block`).
    inline int bit_size() const {
        return size() << low_index_bit_count;
    }

    /// @brief Block with all bits set except the in-block position of bit @p pos.
    inline Block get_bit_inv_mask(int pos) {
        return ~(Block(1) << (pos & low_index_bit_mask));
    }

    /// @brief Block with only the in-block position of bit @p pos set.
    inline Block get_bit_mask(int pos) {
        return Block(1) << (pos & low_index_bit_mask);
    }

    /// @brief View the blocks `[b, e)`.
    inline bit_chunk(Block* b, Block* e) : chunk(b, e) {}

    /// @brief Empty view with null bounds (not `valid()`).
    inline bit_chunk() {}

    /// @brief Check whether at least one bit is set.
    inline bool any() const {
        return !none();
    }

    /// @brief Extend the chunk to hold at least @p n bits, filling new blocks with @p bit.
    /// @warning Writes past `end`; see `pad_blocks`.
    inline void pad_bits(int n, bool bit /*= false*/) {
        pad_blocks(block_for_bits(n), bit);
    }
};

/// Vector of bit chunk views.
typedef std::vector<bit_chunk> bit_chunks;

/// @brief Bit set with an assigned weight.
/// @tparam Weight Weight type.
template<typename Weight>
struct weighted_bit_chunk : bit_chunk {
    /// Weight of the bit set.
    Weight weight;

    /// @brief View the blocks `[b, e)` with weight @p w.
    weighted_bit_chunk(Block* b = 0, Block* e = 0, Weight w = 0) : bit_chunk(b, e), weight(w) {}

    /// @brief View the same blocks as @p bits with weight @p w.
    weighted_bit_chunk(bit_chunk const& bits, Weight w = 0) : bit_chunk(bits), weight(w) {}

    /// @brief Copy the bits (see `bit_chunk::assign`) and the weight of @p r.
    void assign(weighted_bit_chunk const& r) {
        bit_chunk::assign(r);
        weight = r.weight;
    }
};

}  // namespace dual
