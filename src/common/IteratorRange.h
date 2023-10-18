#pragma once

template<typename IteratorT>
struct IteratorRange {
    typedef IteratorT iterator;

    iterator _begin;

    iterator _end;

    IteratorRange() : _begin(iterator()), _end(iterator()) {}

    IteratorRange(iterator begin, iterator end) : _begin(begin), _end(end) {}

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

    template<typename C>
    IteratorRange(C& objects) {
        _begin = std::begin(objects);
        _end = std::end(objects);
    }

    bool empty() const {
        return _begin == _end;
    }

    int size() const {
        if (empty())
            return 0;
        return _end - _begin;
    }

    iterator begin() const {
        return _begin;
    }

    iterator end() const {
        return _end;
    }
};

/*template <typename C>
IteratorRange<typename C::iterator> make_iterator_range(C& c)
{
    return IteratorRange<typename C::iterator>(c);
}*/

// Объединить диапазоны объектов
// Собрать в случае необходимости в одну коллекцию buffer
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
