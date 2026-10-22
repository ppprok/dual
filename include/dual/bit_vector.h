/// @file
/// @brief Owning bit array `bit_vector`.

#pragma once

#include <cstdlib>
#include <new>

#include <dual/bit_chunk.h>

namespace dual {

/// @brief Bit array that allocates memory for its blocks.
///
/// An owning `bit_chunk`: the blocks are allocated with `malloc`/`realloc` and freed in the
/// destructor. The capacity (in blocks) is fixed by the constructor or `zero`; the inherited
/// `bit_chunk` operations never reallocate, so bits must stay below `capacity * bits_per_block`.
/// Move-only.
class bit_vector : public bit_chunk {
private:
    int _capacity;

    inline void free() {
        if (_capacity > 0 && begin)
            std::free(begin);

        _capacity = 0;
        begin = end = nullptr;
    }

public:
    /*Block* begin()
    {
        return bit_chunk::begin;
    }
    
    Block const* begin() const
    {
        return bit_chunk::begin;
    }

    Block* end()
    {
        return bit_chunk::end;
    }

    Block const* end() const
    {
        return bit_chunk::end;
    }*/

    /// @brief Free own memory and take over the memory of @p v, leaving @p v empty.
    inline bit_vector& operator=(bit_vector&& v) noexcept {
        free();

        bit_chunk::begin = v.begin;
        bit_chunk::end = v.end;
        _capacity = v._capacity;

        v._capacity = 0;
        v.free();

        return *this;
    }

    /// @brief Take over the memory of @p v, leaving @p v empty.
    inline bit_vector(bit_vector&& v) noexcept : _capacity(0) {
        bit_chunk::begin = v.begin;
        bit_chunk::end = v.end;
        _capacity = v._capacity;

        v._capacity = 0;
        v.free();
    }

    /// @brief Empty (all-zero) vector with capacity for @p n bits; no memory is allocated if `n <= 0`.
    /// @throws std::bad_alloc If the memory cannot be allocated.
    inline bit_vector(int n) {
        if (n > 0) {
            auto* p = static_cast<Block*>(std::calloc(block_for_bits(n), sizeof(Block)));
            if (!p)
                throw std::bad_alloc();
            _capacity = block_for_bits(n);
            bit_chunk::begin = bit_chunk::end = p;
        } else {
            _capacity = 0;
        }
    }

    /// @brief Empty vector without memory.
    inline bit_vector() : _capacity(0) {}

    /*inline bit_vector::bit_vector( bit_chunk bits ) :bit_chunk(bits), _capacity(0)
    {
    }*/

    /// @brief Free the memory.
    inline ~bit_vector() {
        free();
    }

    /// @brief Zero the bit vector, making room for @p n bits.
    ///
    /// Grows the capacity to at least `block_for_bits(n)` blocks if needed (existing bits are
    /// discarded) and makes the vector empty, i.e. all bits zero.
    /// @throws std::bad_alloc If the memory cannot be allocated; the vector is unchanged then.
    void zero(int n) {
        if (n > 0) {
            n = block_for_bits(n);
            if (_capacity < n) {
                // The old bits are discarded, so allocate anew instead of realloc; on failure the
                // vector keeps its old memory (realloc's result used to overwrite it and leak it).
                auto* p = static_cast<Block*>(std::malloc(n * sizeof(Block)));
                if (!p)
                    throw std::bad_alloc();
                free();
                bit_chunk::begin = bit_chunk::end = p;
                _capacity = n;
            } else {
                bit_chunk::end = bit_chunk::begin;
            }
        } else {
            if (_capacity > 0)
                clear();
        }
    }
};

}  // namespace dual
