/// @file
/// @brief `IteratorRange`: a pair of iterators.

#pragma once

namespace dual {

/// @brief Pair of iterators `[_begin, _end)` usable in range-based `for`.
/// @tparam IteratorT Random-access iterator type.
template<typename IteratorT>
struct IteratorRange {
    /// Iterator type.
    typedef IteratorT iterator;

    /// Range begin.
    iterator _begin;

    /// Range end.
    iterator _end;

    /// @brief Empty range of value-initialised iterators.
    IteratorRange() : _begin(iterator()), _end(iterator()) {}

    /// @brief Range `[begin, end)`.
    IteratorRange(iterator begin, iterator end) : _begin(begin), _end(end) {}

    /// @brief Elements `[from, to)` of @p objects; the bounds are clamped to the container.
    template<typename C>
    IteratorRange(C& objects, int from, int to) {
        if (from < 0)
            from = 0;

        int m = (int) objects.size();
        if (to > m)
            to = m;
        if (from > to)
            from = to;

        _end = _begin = std::begin(objects);
        if (from)
            _begin += from;
        if (to)
            _end += to;
    }

    /// @brief All elements of @p objects.
    template<typename C>
    IteratorRange(C& objects) {
        _begin = std::begin(objects);
        _end = std::end(objects);
    }

    /// @brief Check whether the range is empty.
    bool empty() const {
        return _begin == _end;
    }

    /// @brief Number of elements.
    int size() const {
        if (empty())
            return 0;
        return _end - _begin;
    }

    /// @brief Range begin.
    iterator begin() const {
        return _begin;
    }

    /// @brief Range end.
    iterator end() const {
        return _end;
    }
};

/*template <typename C>
IteratorRange<typename C::iterator> make_iterator_range(C& c)
{
    return IteratorRange<typename C::iterator>(c);
}*/

/// @brief Join two ranges of objects.
///
/// If the ranges are adjacent, the result spans both without copying; otherwise both are gathered,
/// @p r1 first, into @p buffer and the result spans the buffer.
template<typename IteratorRange, typename Buffer>
IteratorRange join_ranges(IteratorRange const& r1, IteratorRange const& r2, Buffer& buffer) {
    if (r1.end() == r2.begin())
        return IteratorRange(r1.begin(), r2.end());

    if (r1.begin() == r2.end())
        return IteratorRange(r2.begin(), r1.end());

    buffer.clear();
    buffer.reserve(r1.size() + r2.size());
    buffer.assign(r1.begin(), r1.end());
    buffer.insert(buffer.end(), r2.begin(), r2.end());

    return IteratorRange(buffer);
}

}  // namespace dual
