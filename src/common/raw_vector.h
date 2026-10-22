/// @file
/// @brief Growable vector of trivially copyable elements `raw_vector`.

#pragma once

#include <cstring>
#include <memory>
#include <stdlib.h>

/// @brief Growable vector that does not call constructors of its elements.
///
/// Memory is managed with `malloc`/`realloc`/`free`, so `T` must be trivially copyable and
/// destructible. Growth reserves 1.5x of the requested size. Pointers into the vector are
/// invalidated by any growth. Index preconditions are checked with `assert` only.
/// @tparam T Trivially copyable element type.
template<typename T>
class raw_vector {

    // Initialised here, so that every constructor starts from an empty vector without memory.
    T* _begin = nullptr;
    T* _end = nullptr;
    int _capacity = 0;

public:
    /// Iterator type.
    typedef T* iterator;
    /// Constant iterator type.
    typedef T const* const_iterator;
    /// Element type.
    typedef T value_type;

    /// @brief Empty vector without memory.
    raw_vector() : _begin(0), _end(0), _capacity(0) {}

    /// @brief Free the memory.
    ~raw_vector() {
        reset();
    }

    /// @brief Number of elements that fit without reallocation.
    inline int capacity() const {
        return _capacity;
    }

    /// @brief Free the memory and make the vector empty.
    void reset() {
        if (_begin) {
            free(_begin);
            _begin = 0;
        }
        _capacity = 0;
        _end = 0;
    }

    /// @brief Take over the memory of @p b, leaving it empty.
    raw_vector(raw_vector&& b) {
        _begin = b._begin;
        _end = b._end;
        _capacity = b._capacity;

        b._begin = 0;
        b._end = 0;
        b._capacity = 0;
    }

    /// @brief Byte copy of @p vec.
    raw_vector(raw_vector const& vec) {
        if (!vec.empty())
            assign(vec._begin, vec._end);
    }

    /// @brief Replace the contents with a byte copy of @p vec.
    raw_vector& operator=(raw_vector const& vec) {
        if (!vec.empty())
            assign(vec._begin, vec._end);
        else
            clear();

        return *this;
    }

    /// @brief Free own memory and take over the memory of @p b, leaving it empty.
    raw_vector& operator=(raw_vector&& b) {
        if (this == &b)
            return *this;

        reset();

        _begin = b._begin;
        _end = b._end;
        _capacity = b._capacity;

        b._begin = 0;
        b._end = 0;
        b._capacity = 0;

        return *this;
    }

    /// @brief Check that memory is allocated.
    bool valid() const {
        return _begin && size() >= 0;
    }

    /// @brief Number of elements.
    int size() const {
        return _end - _begin;
    }

    /// @brief Check whether the vector has no elements.
    bool empty() const {
        return size() == 0;
    }

    /// @brief Pointer to the first element.
    T* begin() {
        return _begin;
    }

    /// @brief Pointer past the last element.
    T* end() {
        return _end;
    }

    /// @brief Pointer to the first element.
    T const* begin() const {
        return _begin;
    }

    /// @brief Pointer past the last element.
    T const* end() const {
        return _end;
    }

    /*T* rbegin()
	{
		return _end - 1;
	}

	T* rend()
	{
		return _begin - 1;
	}

	T const* rbegin() const
	{
		return _end - 1;
	}

	T const* rend() const
	{
		return _begin - 1;
	}*/

    /// @brief Copy of element @p i (`0 <= i < size()`).
    T operator[](int i) const {
        assert(i >= 0 && i < size());
        return _begin[i];
    }

    /// @brief Element @p i (`0 <= i < size()`).
    T& operator[](int i) {
        assert(i >= 0 && i < size());
        return _begin[i];
    }

    /// @brief Make the capacity at least @p s elements (exactly @p s if it grows).
    void reserve(int s) {
        if (_capacity >= s)
            return;

        if (!_begin) {
            _begin = _end = (T*) malloc(s * sizeof(T));
            _capacity = s;
            return;
        }

        int cs = size();
        _begin = (T*) realloc(_begin, s * sizeof(T));
        _end = _begin + cs;
        _capacity = s;
    }

    /// @brief If @p s exceeds the capacity, grow the capacity to `s + capacity() / 2`.
    void reserve_extra(int s) {
        if (s > capacity())
            reserve(s + capacity() / 2);
    }

    /// @brief Remove all elements (the memory is kept).
    void clear() {
        _end = _begin;
    }

    /// @brief Make room for @p s more elements, so that `push_back_raw` can be called @p s times.
    void prepare_push_back(int s = 1) {
        reserve_extra(s + size());
    }

    /// @brief Append @p a without growing; the capacity must suffice (see `prepare_push_back`).
    void push_back_raw(T a) {
        assert(valid() && size() < capacity());
        *_end++ = a;
    }

    /// @brief Append @p a, growing the vector if needed.
    void push_back(T a) {
        prepare_push_back();
        push_back_raw(a);
    }

    /// @brief Copy of the last element of a non-empty vector.
    T back() const {
        assert(!empty());
        return *(_end - 1);
    }

    /// @brief Last element of a non-empty vector.
    T& back() {
        assert(!empty());
        return *(_end - 1);
    }

    /// @brief Copy of the first element of a non-empty vector.
    T front() const {
        assert(!empty());
        return *_begin;
    }

    /// @brief First element of a non-empty vector.
    T& front() {
        assert(!empty());
        return *_begin;
    }

    /// @brief Remove the last element; no-op for an empty vector.
    void pop_back() {
        if (!empty())
            --_end;
    }

    /// @brief Change the size to @p s (negative means 0); added elements are uninitialised.
    void resize_raw(int s) {
        if (s < 0)
            s = 0;
        reserve_extra(s);
        _end = _begin + s;
    }

    /// @brief Change the size to @p s; added elements are set to @p val.
    void resize(int s, T val = T()) {
        if (size() >= s)
            return resize_raw(s);

        reserve_extra(s);
        memset((void*) _end, 0, sizeof(T));

        if (*_end != val) {
            for (int c = size(); c < s; ++c)
                push_back_raw(val);
        } else {
            auto p = _end + 1;
            _end = _begin + s;
            if (p != _end)
                memset((void*) p, 0, (_end - p) * sizeof(T));
        }
    }

    /// @brief Remove a single element at the given index, shifting the following ones.
    void erase(int i) {
        erase(_begin + i);
    }

    /// @brief Remove a single element, shifting the following ones.
    void erase(T* b) {
        erase(b, b + 1);
    }

    /// @brief Remove several elements `[b, e)`, shifting the following ones.
    void erase(T* b, T* e) {
        int s = e - b;
        if (s == 0)
            return;
        assert(_begin <= b && s > 0 && e <= _end);
        memmove(b, e, (_end - e) * sizeof(T));
        _end -= s;
    }

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

    /// @brief Vector with a copy of `[b, e)` (random-access iterators).
    template<typename Iter>
    raw_vector(Iter b, Iter e) {
        assign_range(b, e);
    }

    /// @brief Vector with a byte copy of `[b, e)`.
    raw_vector(T const* b, T const* e) {
        assign(b, e);
    }
};
