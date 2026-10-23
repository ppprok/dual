/// @file
/// @brief `range`: an index range in a container.

#pragma once

namespace dual {

/// @brief Range of elements in a container, as indices `[begin, end)`.
struct range {
    /// Index of the first element.
    int begin;
    /// Index past the last element.
    int end;

    /// @brief Range `[b, e)`; the default `[0, -1)` is empty and not `valid()`.
    range(int b = 0, int e = -1) : begin(b), end(e) {}

    /// @brief Set the range to the iterators `[b, e)` into container @p c.
    template<typename C, typename Iter>
    void set_range(C const& c, Iter b, Iter e) {
        begin = b - c.begin();
        end = e - c.begin();
    }

    /// @brief Set the range to the whole container @p c.
    template<typename C>
    void set_range(C const& c) {
        begin = 0;
        end = c.size();
    }

    /// @brief Set the begin to iterator @p b into container @p c.
    template<typename C, typename Iter>
    void set_begin(C const& c, Iter b) {
        begin = b - c.begin();
    }

    /// @brief Set the end to iterator @p e into container @p c.
    template<typename C, typename Iter>
    void set_end(C const& c, Iter e) {
        end = e - c.begin();
    }

    /// @brief Check whether the range is empty (`begin >= end`).
    bool empty() const {
        return begin >= end;
    }

    /// @brief Make the range empty (`end = begin`).
    void clear() {
        end = begin;
    }

    /// @brief Number of elements (`end - begin`).
    int size() const {
        return end - begin;
    }

    /// @brief Check that `0 <= begin <= end`.
    bool valid() const {
        return begin >= 0 && end >= begin;
    }

    /// @brief Iterator to the first element of the range in container @p c.
    template<typename C>
    auto begin_in(C& c) -> decltype(c.begin() + begin) {
        return c.begin() + begin;
    }

    /// @brief Iterator past the last element of the range in container @p c.
    template<typename C>
    auto end_in(C& c) -> decltype(c.begin() + end) {
        return c.begin() + end;
    }
};

}  // namespace dual
