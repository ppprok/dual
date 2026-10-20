#pragma once

// Chunk defined by bounds
template<typename T>
struct chunk {
    typedef T value_type;

    // Chunk begin
    T* begin;
    // Chunk end
    T* end;

    chunk(T* b = 0, T* e = 0) : begin(b), end(e) {}

    void resize(int s) {
        assert(valid());
        assert(s >= 0);
        end = begin + s;
    }

    void resize(int s, T const& value) {
        assert(s >= 0);
        assert(valid());
        auto c = end, end = begin + s;
        if (c >= end)
            return;

        for (; c != end; ++c)
            *c = value;
    }

    void clear() {
        end = begin;
    }

    // Check whether the chunk is empty
    bool empty() const {
        assert(valid());
        return begin == end;
    }

    // Get the size of the chunk
    int size() const {
        return end - begin;
    }

    chunk& operator+=(T* base) {
        int offset = (base - (T*) 0);
        begin += offset;
        end += offset;
        assert(valid());
        return *this;
    }

    chunk& operator-=(T* base) {
        assert(valid());
        int offset = (base - (T*) 0);
        begin -= offset;
        end -= offset;
        return *this;
    }

    // Check that the chunk specifies a valid memory range
    bool valid() const {
        return begin && begin <= end;
    }
};
