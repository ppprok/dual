/// @file
/// @brief Non-owning memory range view `chunk`.

#pragma once

#include <cassert>
#include <cstddef>

/// @brief Chunk defined by bounds: a non-owning view of the memory range `[begin, end)`.
/// @tparam T Element type.
template<typename T>
struct chunk {
    /// Element type.
    typedef T value_type;

    /// Chunk begin.
    T* begin;
    /// Chunk end (one past the last element).
    T* end;

    /// @brief View the range `[b, e)`; by default an empty view with null bounds (not `valid()`).
    chunk(T* b = 0, T* e = 0) : begin(b), end(e) {}

    /// @brief Set the size to @p s elements by moving `end`; no memory is allocated or initialised.
    void resize(int s) {
        assert(valid());
        assert(s >= 0);
        end = begin + s;
    }

    /// @brief Set the size to @p s elements, filling elements added past the old `end` with @p value.
    /// @warning Writes past the old `end`; the memory must be available.
    void resize(int s, T const& value) {
        assert(s >= 0);
        assert(valid());
        auto c = end;
        end = begin + s;
        if (c >= end)
            return;

        for (; c != end; ++c)
            *c = value;
    }

    /// @brief Make the chunk empty (`end = begin`).
    void clear() {
        end = begin;
    }

    /// @brief Check whether the chunk is empty.
    bool empty() const {
        assert(valid());
        return begin == end;
    }

    /// @brief Get the size of the chunk (number of elements).
    int size() const {
        return end - begin;
    }

    /// @brief Rebase: treat `begin`/`end` as offsets from a null pointer and turn them into
    ///        pointers into the memory starting at @p base.
    chunk& operator+=(T* base) {
        std::ptrdiff_t offset = (base - (T*) 0);
        begin += offset;
        end += offset;
        assert(valid());
        return *this;
    }

    /// @brief Inverse of `operator+=`: turn pointers into the memory at @p base into offsets.
    chunk& operator-=(T* base) {
        assert(valid());
        std::ptrdiff_t offset = (base - (T*) 0);
        begin -= offset;
        end -= offset;
        return *this;
    }

    /// @brief Check that the chunk specifies a valid memory range (non-null `begin`, `begin <= end`).
    bool valid() const {
        return begin && begin <= end;
    }
};
