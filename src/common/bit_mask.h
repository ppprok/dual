#pragma once

#include "bit_chunk.h"

// ћаска, накладываема€ на битовые массивы при выполнении операций с ними
struct bit_mask : bit_chunk {
    inline bit_mask(bit_chunk b) : bit_chunk(b) {}

    inline void trim(bit_chunk& bits) {
        assert(valid());
        assert(bits.valid());

        if (bits.empty())
            return;

        if (empty()) {
            bits.end = bits.begin;
            return;
        }

        int s = bits.size();
        int s1 = size();
        if (s > s1)
            s = s1;

        for (auto d = bits.begin + s, e = begin + s; e != begin;) {
            --d;
            --e;
            if (*d & *e) {
                bits.end = d + 1;
                return;
            }
        }

        bits.end = bits.begin;
    }

    inline bool none(bit_chunk const& bits) const {
        assert(valid());
        assert(bits.valid());

        if (bits.empty() || empty())
            return true;

        for (auto i = begin, e = bits.begin; i != end && e != bits.end; ++i, ++e)
            if (*i & *e)
                return false;

        return true;
    }

    inline int find_last(bit_chunk const& bits) const {
        assert(valid());
        assert(bits.valid());

        int s = size();
        int s1 = bits.size();
        if (s > s1)
            s = s1;

        for (auto d = begin + s, e = bits.begin + s; d != begin;) {
            --d;
            --e;
            auto b = *d & *e;
            if (b)
                return get_last(b) + ((d - begin) << low_index_bit_count);
        }

        return -1;
    }

    // —равнение числа битов после наложени€ маски
    inline bool less_count(bit_chunk const& r1, bit_chunk const& r2) const {
        assert(valid());
        assert(r1.valid());
        assert(r2.valid());

        int c2 = count(r2);
        return c2 && count(r1, c2) < c2;
    }

    inline bool less_bits(bit_chunk const& r1, bit_chunk const& r2) const {
        assert(valid());
        assert(r1.valid());
        assert(r2.valid());

        auto q = begin;
        auto e1 = r1.begin;
        auto e2 = r2.begin;
        for (; q != end && e1 != r1.end && e2 != r2.end; ++q, ++e1, ++e2) {
            auto b1 = *q & *e1;
            auto b2 = *q & *e2;
            if (b1 == b2)
                continue;
            if (b1 > b2)
                return false;
            return true;
        }

        for (; q != end && e1 != r1.end; ++q, ++e1)
            if (*q & *e1)
                return false;

        for (; q != end && e2 != r2.end; ++q, ++e2)
            if (*q & *e2)
                return true;

        return false;
    }

    inline bool equal_bits(bit_chunk const& r1, bit_chunk const& r2) const {
        assert(valid());
        assert(r1.valid());
        assert(r2.valid());

        auto q = begin;
        auto e1 = r1.begin;
        auto e2 = r2.begin;
        for (; q != end && e1 != r1.end && e2 != r2.end; ++q, ++e1, ++e2) {
            auto b1 = *q & *e1;
            auto b2 = *q & *e2;
            if (b1 != b2)
                return false;
        }

        for (; q != end && e1 != r1.end; ++q, ++e1)
            if (*q & *e1)
                return false;

        for (; q != end && e2 != r2.end; ++q, ++e2)
            if (*q & *e2)
                return false;

        return true;
    }

    inline int subset_cmp(bit_chunk const& b1, bit_chunk const& b2) const {
        assert(valid());
        assert(b1.valid());
        assert(b2.valid());

        auto d0 = begin, d1 = b1.begin, d2 = b2.begin;
        int r = 0;

        for (; d0 != end && d1 != b1.end && d2 != b2.end; ++d0, ++d1, ++d2) {
            auto a = *d1 & *d0;
            if ((*d2 & a) == a) {
                if (r == 1)
                    return 0;
                r = -1;
                continue;
            }

            a = *d2 & *d0;
            if ((*d1 & a) == a) {
                if (r == -1)
                    return 0;
                r = 1;
                continue;
            }

            return 0;
        }

        if (r != 1)
            for (; d0 != end && d1 != b1.end; ++d0, ++d1)
                if (*d1 & *d0)
                    return 0;

        if (r != -1)
            for (; d0 != end && d2 != b2.end; ++d0, ++d2)
                if (*d2 & *d0)
                    return 0;

        if (r == 0)
            return 1;

        return r;
    }

    inline bool intersects(bit_chunk const& b1, bit_chunk const& b2) const {
        assert(valid());
        assert(b1.valid());
        assert(b2.valid());

        for (auto d = begin, e1 = b1.begin, e2 = b2.begin; d != end && e1 != b1.end && e1 != b2.end; ++d, ++e1, ++e2) {
            if (*d & *e1 & *e2)
                return true;
        }
        return false;
    }

    inline int count(bit_chunk const& bits) const {
        assert(valid());
        assert(bits.valid());

        int c = 0;
        for (auto d = begin, e = bits.begin; d != end && e != bits.end; ++d, ++e) {
            auto a = *d & *e;
            if (a)
                c += bits_count(a);
        }

        return c;
    }

    inline int count(bit_chunk const& bits, int maxc) const {
        assert(valid());
        assert(bits.valid());

        int c = 0;
        for (auto d = begin, e = bits.begin; d != end && e != bits.end; ++d, ++e) {
            auto a = *d & *e;
            if (a) {
                c += bits_count(a);
                if (c > maxc)
                    break;
            }
        }
        return c;
    }

    inline bool any(bit_chunk const& bits) const {
        return !none(bits);
    }

    inline int find_next(bit_chunk const& bits, int k) const {
        assert(valid());
        assert(bits.valid());
        assert(k >= -1);

        int h = (k + 1) >> low_index_bit_count;
        auto d = begin + h;
        auto e = bits.begin + h;
        assert(d <= end);
        assert(e <= bits.end);
        if (d == end || e == bits.end)
            return -1;

        int i = h << low_index_bit_count;
        int r = (k + 1) & low_index_bit_mask;
        auto b = (*d & *e) >> r;
        if (b)
            return i + r + get_first(b);
        i += bits_per_block;
        ++d;
        ++e;

        for (; d != end && e != bits.end; ++e, ++d, i += bits_per_block) {
            auto b = *d & *e;
            if (b)
                return i + get_first(b);
        }

        return -1;
    }

    inline int find_first(bit_chunk const& bits) const {
        assert(valid());
        assert(bits.valid());

        int i = 0;
        for (auto d = begin, e = bits.begin; d != end && e != bits.end; ++d, ++e, i += bits_per_block) {
            auto b = *d & *e;
            if (b)
                return get_first(b) + i;
        }

        return -1;
    }

    // Ќайти первое отличие
    inline int find_first_diff(bit_chunk const& bits) const {
        assert(valid());
        assert(bits.valid());

        int i = 0;
        auto d = begin, e = bits.begin;
        for (; d != end && e != bits.end; ++d, ++e, i += bits_per_block) {
            auto b = *d ^ *e;
            if (b)
                return get_first(b) + i;
        }

        for (; d != end; ++d, i += bits_per_block) {
            auto b = *d;
            if (b)
                return get_first(b) + i;
        }

        for (; e != bits.end; ++e, i += bits_per_block) {
            auto b = *e;
            if (b)
                return get_first(b) + i;
        }

        return -1;
    }

    inline int find_next_diff(bit_chunk const& bits, int k) const {
        assert(valid());
        assert(bits.valid());
        assert(k >= -1);

        int h = (k + 1) >> low_index_bit_count;
        auto d = begin + h;
        auto e = bits.begin + h;
        //assert(d <= end || e <= bits.end);

        if (d >= end) {
            if (e >= bits.end)
                return -1;
            return bits.find_next(k);
        }

        if (e >= bits.end)
            return bit_chunk::find_next(k);

        int i = h << low_index_bit_count;
        int r = (k + 1) & low_index_bit_mask;
        auto b = (*d ^ *e) >> r;
        if (b)
            return i + r + get_first(b);
        i += bits_per_block;
        ++d;
        ++e;

        for (; d != end && e != bits.end; ++e, ++d, i += bits_per_block) {
            auto b = *d ^ *e;
            if (b)
                return i + get_first(b);
        }

        for (; d != end; ++d, i += bits_per_block) {
            auto b = *d;
            if (b)
                return i + get_first(b);
        }

        for (; e != bits.end; ++e, i += bits_per_block) {
            auto b = *e;
            if (b)
                return i + get_first(b);
        }

        return -1;
    }
};
