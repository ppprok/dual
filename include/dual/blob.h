/// @file
/// @brief Fixed-size container of trivially copyable elements `blob`.

#pragma once

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <new>

namespace dual {

/// @brief Fixed-size container that does not call constructors of its elements.
///
/// Memory is managed with `malloc`/`realloc`/`free`, so `T` must be trivially copyable and
/// destructible. Unlike `raw_vector`, there is no spare capacity: the size is the allocated size.
/// Move-only. Index preconditions are checked with `assert` only.
/// @tparam T Trivially copyable element type.
template<typename T>
class blob {

    // Initialised here, so that every constructor starts from an empty blob without memory.
    T* _begin = nullptr;
    T* _end = nullptr;
    //int _capacity;

public:
    /// Iterator type.
    typedef T* iterator;
    /// Constant iterator type.
    typedef T const* const_iterator;
    /// Element type.
    typedef T value_type;

    /// @brief Empty blob without memory.
    inline blob() :
            _begin(nullptr),
            _end(nullptr)  //, _capacity(0)
    {}

    /// @brief Blob of @p n uninitialised elements (`n >= 0`).
    /// @throws std::bad_alloc If the memory cannot be allocated.
    inline blob(int n)
    //:_capacity(n)
    {
        assert(n >= 0);
        if (n > 0) {
            _begin = static_cast<T*>(std::malloc(n * sizeof(T)));
            if (!_begin)
                throw std::bad_alloc();
            _end = _begin + n;
        } else {
            _begin = _end = nullptr;
        }
    }

    /// @brief Free the memory.
    inline ~blob() {
        reset();
    }

    /// @brief Free the memory and make the blob empty.
    inline void reset() {
        if (_begin) {
            free(_begin);
            _begin = nullptr;
        }
        _end = nullptr;
    }

    /// @brief Take over the memory of @p b, leaving it empty.
    inline blob(blob&& b) noexcept {
        _begin = b._begin;
        _end = b._end;

        b._begin = nullptr;
        b._end = nullptr;
    }

    /// @brief Free own memory and take over the memory of @p b, leaving it empty.
    inline blob& operator=(blob&& b) noexcept {
        if (this == &b)
            return *this;

        reset();

        _begin = b._begin;
        _end = b._end;

        b._begin = nullptr;
        b._end = nullptr;

        return *this;
    }

    /// @brief Number of elements.
    inline int size() const {
        return _end - _begin;
    }

    /// @brief Check whether the blob has no elements.
    inline bool empty() const {
        return _begin >= _end;
    }

    /// @brief Pointer to the first element.
    inline T* begin() {
        return _begin;
    }

    /// @brief Pointer past the last element.
    inline T* end() {
        return _end;
    }

    /// @brief Pointer to the first element.
    inline T const* begin() const {
        return _begin;
    }

    /// @brief Pointer past the last element.
    inline T const* end() const {
        return _end;
    }

    /// @brief Copy of element @p i.
    inline T get(int i) const {
        assert(i >= 0 && i < size());
        return _begin[i];
    }

    /// @brief Element @p i (`0 <= i < size()`).
    inline T const& operator[](int i) const {
        assert(i >= 0 && i < size());
        return _begin[i];
    }

    /// @brief Element @p i (`0 <= i < size()`).
    inline T& operator[](int i) {
        assert(i >= 0 && i < size());
        return _begin[i];
    }

    /*inline bool push_back(T a)
    {
        if (size() >= capacity())
            return false;
        *_end++ = a;
        return true;
    }
    
    inline bool push_back_cref(T const& a)
    {
        if (size() >= capacity())
            return false;
        *_end++ = a;
        return true;
    }*/

    /// @brief Remove the last element (the memory is kept).
    inline void pop_back() {
        assert(!empty());
        --_end;
    }

    /// @brief Copy of the last element of a non-empty blob.
    inline T back() const {
        assert(!empty());
        return *(_end - 1);
    }

    /// @brief Last element of a non-empty blob.
    inline T& back() {
        assert(!empty());
        return *(_end - 1);
    }

    /// @brief Copy of the first element of a non-empty blob.
    inline T front() const {
        assert(!empty());
        return *_begin;
    }

    /// @brief First element of a non-empty blob.
    inline T& front() {
        assert(!empty());
        return *_begin;
    }

    /// @brief Change the size to @p s elements.
    ///
    /// Shrinking only moves the end. Growing reallocates: a blob without memory gets zeroed
    /// elements (`calloc`), otherwise the added elements are uninitialised (`realloc`).
    /// @throws std::bad_alloc If the memory cannot be allocated; the blob is unchanged then.
    inline void resize(int s) {
        if (s <= 0) {
            _end = _begin;
        } else if (s < size()) {
            _end = _begin + s;
        } else if (s > size()) {
            // On failure the blob keeps its memory and size (no leak through realloc's null result)
            auto* p = static_cast<T*>(_begin ? std::realloc(_begin, s * sizeof(*_begin))
                                             : std::calloc(s, sizeof(*_begin)));
            if (!p)
                throw std::bad_alloc();
            _begin = p;
            _end = _begin + s;
        }
    }

    // Remove a single element at the given index
    /*inline void erase(int i)
    {
        erase(_begin + i);
    }
    
    // Remove a single element
    void erase(T* b)
    {
        erase(b, b+1);
    }

    // Remove several elements
    void erase(T* b, T*e)
    {
        int s = b - e;
        if (s == 0)
            return;
        assert(_begin <= b && s > 0 && e <= _end);
		memmove(b, e, (_end - e) * sizeof(T));
        _end -= s;
    }
    */

    /// @brief Replace the contents with a byte copy of `[b, e)`.
    void assign(T const* b, T const* e) {
        resize(e - b);
        if (!empty())
            memcpy(_begin, b, (e - b) * sizeof(T));
    }

    /// @brief Replace the contents with the elements of `[b, e)` (random-access iterators).
    template<typename Iter>
    void assign_range(Iter b, Iter e) {
        resize(e - b);
        for (auto i = _begin; b != e;)
            *(i++) = *(b++);
    }

    /// @brief Replace the contents with the elements of container @p c.
    template<typename C>
    void assign(C& c) {
        assign_range(std::begin(c), std::end(c));
    }

    /// @brief Blob with a copy of `[b, e)` (random-access iterators).
    template<typename Iter>
    blob(Iter b, Iter e) {
        assign_range(b, e);
    }

    /// @brief Blob with a byte copy of `[b, e)`.
    blob(T const* b, T const* e) {
        assign(b, e);
    }
};

}  // namespace dual
