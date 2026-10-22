#pragma once

#include <algorithm>
#include <set>
#include <vector>

#include <dual/runc.h>
#include <dual/runc_executor.h>

// Helpers shared by the RUNC API and differential tests
namespace runc_test {

using namespace dual;

typedef std::vector<std::vector<int>> Rows;
typedef std::vector<int> Cover;

inline int widthOf(Rows const& rows) {
    int n = 0;
    for (auto const& r : rows)
        for (int j : r)
            n = std::max(n, j + 1);
    return n;
}

// Builds a bit matrix with a fixed row stride, so no row has to be moved while filling it
inline bit_matrix makeMatrix(Rows const& rows) {
    bit_matrix bm;
    bm.zero(static_cast<int>(rows.size()), widthOf(rows));
    for (size_t i = 0; i < rows.size(); ++i)
        for (int j : rows[i])
            bm.get_row(static_cast<int>(i)).set(j);
    return bm;
}

inline bool isCover(Rows const& rows, Cover const& c) {
    for (auto const& r : rows) {
        bool covered = false;
        for (int j : r)
            if (std::find(c.begin(), c.end(), j) != c.end()) {
                covered = true;
                break;
            }
        if (!covered)
            return false;
    }
    return true;
}

inline bool isMinimalCover(Rows const& rows, Cover const& c) {
    if (!isCover(rows, c))
        return false;
    for (size_t k = 0; k < c.size(); ++k) {
        Cover d = c;
        d.erase(d.begin() + k);
        if (isCover(rows, d))
            return false;
    }
    return true;
}

// Collects covers, remembering whether any was reported twice or is not a minimal cover
struct CoverSet {
    std::set<Cover> covers;
    size_t duplicates = 0;
    size_t notMinimal = 0;

    void add(Rows const& rows, Cover c) {
        std::sort(c.begin(), c.end());
        if (!isMinimalCover(rows, c))
            ++notMinimal;
        if (!covers.insert(c).second)
            ++duplicates;
    }
};

// Parses [size, col...] records written by the buffer API
inline void addRecords(Rows const& rows, CoverSet& out, uint32_t const* buffer, size_t count) {
    for (size_t k = 0; k < count; ++k) {
        uint32_t size = *buffer++;
        out.add(rows, Cover(buffer, buffer + size));
        buffer += size;
    }
}

// Enumerates through the C ABI with a buffer of the given size
inline CoverSet enumerateCApi(Rows const& rows, uint32_t buffer_size, bool* ok = nullptr) {
    CoverSet result;
    bool good = true;
    auto h = init_dualizer();
    for (size_t i = 0; i < rows.size(); ++i) {
        std::vector<int32_t> ones(rows[i].begin(), rows[i].end());
        good &= set_input_row(h, static_cast<int32_t>(i), static_cast<int32_t>(ones.size()), ones.data()) == RUNC_OK;
    }
    const uint32_t canary = 0xDEADBEEF;
    std::vector<uint32_t> buffer(buffer_size + 16, canary);
    for (;;) {
        int32_t c = enumerate_covers(h, buffer_size, buffer.data());
        for (size_t k = buffer_size; k < buffer.size(); ++k)
            good &= buffer[k] == canary;
        if (c <= 0) {
            good &= c == 0;
            break;
        }
        addRecords(rows, result, buffer.data(), c);
    }
    release_dualizer(h);
    if (ok)
        *ok = good;
    return result;
}

}  // namespace runc_test
