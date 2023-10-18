#pragma once

// Интервал элементов в контейнере
struct range {
    int begin;
    int end;

    range(int b = 0, int e = -1) : begin(b), end(e) {}

    template<typename C, typename Iter>
    void set_range(C const& c, Iter b, Iter e) {
        begin = b - c.begin();
        end = e - c.begin();
    }

    template<typename C>
    void set_range(C const& c) {
        begin = 0;
        end = c.size();
    }

    template<typename C, typename Iter>
    void set_begin(C const& c, Iter b) {
        begin = b - c.begin();
    }

    template<typename C, typename Iter>
    void set_end(C const& c, Iter e) {
        end = e - c.begin();
    }

    bool empty() const {
        return begin >= end;
    }

    void clear() {
        end = begin;
    }

    int size() const {
        return end - begin;
    }

    bool valid() const {
        return begin >= 0 && end >= begin;
    }

    template<typename C>
    auto begin_in(C& c) -> decltype(c.begin() + begin) {
        return c.begin() + begin;
    }

    template<typename C>
    auto end_in(C& c) -> decltype(c.begin() + end) {
        return c.begin() + end;
    }
};
