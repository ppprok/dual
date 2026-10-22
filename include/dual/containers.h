/// @file
/// @brief Small algorithms over STL-like containers.

#pragma once

#include <algorithm>

#include <dual/bit_chunk.h>

namespace dual {

/// @brief Concatenate two containers.
template<typename C>
inline C join(C const& c1, C const& c2) {
    C c3;
    c3.reserve(c1.size() + c2.size());
    c3.insert(c3.end(), c1.begin(), c1.end());
    c3.insert(c3.end(), c2.begin(), c2.end());
    return std::move(c3);
}

/// @brief Concatenate three containers.
template<typename C>
inline C join(C const& c1, C const& c2, C const& c0) {
    C c3;
    c3.reserve(c1.size() + c2.size() + c0.size());
    c3.insert(c3.end(), c1.begin(), c1.end());
    c3.insert(c3.end(), c2.begin(), c2.end());
    c3.insert(c3.end(), c0.begin(), c0.end());
    return std::move(c3);
}

/*template <typename C1, typename C2>
C1& append_with_move(C1& c1, C2& c2)
{
    c1.reserve(c1.size() + c2.size());
    for (auto& v:c2)
    {
        c1.push_back(std::move(v));
    }
    c2.clear();

    return c1;
};*/

/// @brief Check that the index points to an element of the container (`0 <= pos < c.size()`).
template<typename C>
inline bool inside(C const& c, int pos) {
    return pos >= 0 && pos < (int) c.size();
}

/// @brief Remove the elements that satisfy the predicate from the collection.
///
/// Each removed element is replaced by the last one, so the order is not preserved.
template<typename C, typename F>
inline void pop_if(C& c, F cond) {
    int n = c.size();
    for (int i = 0; i < n;) {
        if (!cond(c[i])) {
            ++i;
        } else {
            c[i] = std::move(c.back());
            c.pop_back();
            --n;
        }
    }
}

/// @brief Remove the elements that satisfy the predicate from the collection, keeping the order
///        of the others (erase-remove idiom).
template<typename C, typename F>
inline void remove_if(C& c, F cond) {
    auto end = std::end(c);
    c.erase(std::remove_if(std::begin(c), end, cond), end);
}

/// @brief Index of the first element that satisfies the predicate, or -1.
template<typename C, typename F>
inline int find_if(C& c, F cond) {
    auto end = std::end(c), beg = std::begin(c);
    auto pos = std::find_if(beg, end, cond);
    if (pos == end)
        return -1;
    return pos - beg;
}

/// @brief Remove the elements that convert to `false` from the collection.
///
/// Each removed element is replaced by the last one, so the order is not preserved.
template<typename C>
inline void pop_false(C& c) {
    int n = (int) c.size();
    for (int i = 0; i < n;) {
        if (c[i]) {
            ++i;
        } else {
            c[i] = std::move(c.back());
            c.pop_back();
            --n;
        }
    }
}

/// @brief Check whether the collection contains the element (linear search).
template<typename C, typename V>
inline bool contains(C const& c, V const& v) {
    auto e = c.end();
    return std::find(c.begin(), e, v) != e;
}

/// @brief Remove from @p c the elements contained in @p excl, keeping the order of the others.
template<typename C, typename C1>
inline void exclude(C& c, C1 const& excl) {
    remove_if(c, [&excl](typename C::value_type const& v) { return contains(excl, v); });
}

/// @brief Sort the whole container with `operator<`.
template<typename C>
inline void sort(C& c) {
    std::sort(c.begin(), c.end());
}

/// @brief Sort the whole container with comparator @p cmp.
template<typename C, typename Cmp>
inline void sort(C& c, Cmp cmp) {
    std::sort(c.begin(), c.end(), cmp);
}

/// @brief Remove duplicate elements, leaving the container sorted.
/// @param c Container to process.
/// @param ordered `true` if @p c is already sorted (skips sorting).
template<typename C>
inline void unique(C& c, bool ordered = false) {
    if (!ordered)
        sort(c);

    c.resize(std::unique(c.begin(), c.end()) - c.begin());
}

/// @brief Shuffle the elements of the collection (`std::random_shuffle`).
template<typename C>
inline void random_shuffle(C& c) {
    std::random_shuffle(std::begin(c), std::end(c));
}

/// @brief Shuffle, then sort: equal elements end up in random order.
template<typename C>
inline void random_sort(C& c) {
    random_shuffle(c);
    sort(c);
}

/// @brief Shuffle, then sort with @p cmp: equivalent elements end up in random order.
template<typename C, typename Cmp>
inline void random_sort(C& c, Cmp cmp) {
    random_shuffle(c);
    sort(c, cmp);
}

/// @brief Element-wise comparison of collections (equal sizes and equal elements).
template<typename C1, typename C2>
inline bool equal(C1 const& c1, C2 const& c2) {
    return c1.size() == c2.size() && std::equal(std::begin(c1), std::end(c1), std::begin(c2));
}

/// @brief Comparison in lexicographic order.
template<typename C1, typename C2>
inline bool less(C1 const& c1, C2 const& c2) {
    return std::lexicographical_compare(std::begin(c1), std::end(c1), std::begin(c2), std::end(c2));
}

/// @brief Put the elements of both collections into the first collection, leaving @p c2 empty.
///
/// The larger collection is kept and the elements of the smaller one are moved into it, so the
/// order of elements is not preserved.
template<typename C>
inline void merge(C& c1, C& c2) {
    if (c1.size() < c2.size())
        c1.swap(c2);
    while (!c2.empty()) {
        c1.emplace_back(std::move(c2.back()));
        c2.pop_back();
    }
}

/// @brief Select the required number of "best" elements (the smallest according to @p cmp).
/// @param objects Source collection.
/// @param count Number of elements to select.
/// @param cmp Strict ordering; "best" elements come first.
/// @param[out] most Selected elements, sorted by @p cmp when they are fewer than the source or
///             when @p order is set.
/// @param order Sort the result also when all elements are selected.
/// @param random Shuffle before sorting so that ties are broken randomly (only when all elements are
///        selected).
template<typename Objects, typename Cmp>
inline void most(Objects const& objects, int count, Cmp cmp, Objects& most, bool order = true, bool random = true) {
    most.clear();

    if (count <= 0)
        return;

    if (count >= (int) objects.size()) {
        most.assign(objects.begin(), objects.end());
        if (order) {
            if (random)
                random_shuffle(most);
            sort(most, cmp);
        }
    } else {
        most.resize(count);
        //if (random)
        //  random_shuffle(objects);
        std::partial_sort_copy(objects.begin(), objects.end(), most.begin(), most.end(), cmp);
    }
}

/// @brief Keep the required number of "best" elements (the smallest according to @p cmp), in place.
/// @param objects Collection to shrink.
/// @param count Number of elements to keep.
/// @param cmp Strict ordering; "best" elements come first.
/// @param order Sort the collection also when all elements are kept.
/// @param random Shuffle before selecting so that ties are broken randomly.
template<typename Objects, typename Cmp>
inline void most(Objects& objects, int count, Cmp cmp, bool order = true, bool random = true) {
    if (count <= 0) {
        objects.clear();
        return;
    }

    if (count >= (int) objects.size()) {
        if (order) {
            if (random)
                random_shuffle(objects);

            sort(objects, cmp);
        }
    } else {
        if (random)
            random_shuffle(objects);

        std::partial_sort(objects.begin(), objects.begin() + count, objects.end(), cmp);
        objects.resize(count);
    }
}

}  // namespace dual
