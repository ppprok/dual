#pragma once

// Helpers shared by the algorithm tests (#47): matrices as rows of column indices, brute-force
// minimal covers, random matrices, the data files with their expected cover counts, and
// collecting the covers of any DualizationBacktrackAlgorithmBase.

#include <doctest.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <random>
#include <set>
#include <string>
#include <vector>

#include <dual/bits.h>
#include <dual/cover_collector.h>
#include <dual/dualization_backtrack_algorithm_base.h>
#include <dual/integers_file_reader.h>

namespace dual_test {

using namespace dual;

typedef std::vector<std::vector<int>> Rows;
typedef std::vector<int> Cover;
typedef std::set<Cover> Covers;

// Number of columns: the largest column index + 1
inline int widthOf(Rows const& rows) {
    int n = 0;
    for (auto const& r : rows)
        for (int j : r)
            n = std::max(n, j + 1);
    return n;
}

// Builds a bit matrix with at least n columns and a fixed row stride, so no row has to be moved
// while filling it; empty rows stay zero rows
inline bit_matrix makeMatrix(Rows const& rows, int n = 0) {
    bit_matrix bm;
    bm.zero(static_cast<int>(rows.size()), std::max(n, widthOf(rows)));
    for (size_t i = 0; i < rows.size(); ++i)
        for (int j : rows[i])
            bm.get_row(static_cast<int>(i)).set(j);
    return bm;
}

// bit_matrix is not copyable, so matrices are passed around as rows of column indices
inline Rows toRows(bit_matrix const& matrix) {
    Rows rows(matrix.height());
    for (int i = 0; i < matrix.height(); ++i) {
        auto const& row = matrix.get_row(i);
        for (int j = row.find_first(); j >= 0; j = row.find_next(j))
            rows[i].push_back(j);
    }
    return rows;
}

inline Rows loadRows(std::string const& filename) {
    return toRows(IntegersFileReader(filename).ReadBitMatrix());
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

// Minimal covers by exhaustive search over the column subsets of the first n columns (n <= 20,
// it takes 2^n steps);
// n defaults to the width of the rows
inline Covers bruteForce(Rows const& rows, int n = -1) {
    if (n < 0)
        n = widthOf(rows);
    Covers result;
    for (unsigned s = 0; s < (1u << n); ++s) {
        Cover c;
        for (int j = 0; j < n; ++j)
            if (s >> j & 1)
                c.push_back(j);
        if (isMinimalCover(rows, c))
            result.insert(c);
    }
    return result;
}

// Random m x n matrix of a random density; about one matrix in zeroRowOdds gets a zero row
// (0: never), the other rows are not empty
inline Rows randomRows(std::mt19937& random, int m, int n, int zeroRowOdds = 8) {
    std::uniform_real_distribution<double> density(0.05, 0.65);
    std::uniform_real_distribution<double> uniform(0, 1);
    double p = density(random);
    Rows rows(m);
    for (auto& row : rows) {
        for (int j = 0; j < n; ++j)
            if (uniform(random) < p)
                row.push_back(j);
        if (row.empty())
            row.push_back(random() % n);
    }
    if (zeroRowOdds > 0 && random() % zeroRowOdds == 0)
        rows[random() % m].clear();
    return rows;
}

// Sorted copy of a cover
inline Cover sorted(Cover c) {
    std::sort(c.begin(), c.end());
    return c;
}

// Runs the algorithm with a CoverCollector and returns its covers with sorted columns; fails on a
// repeated cover. With maxCount > 0 the enumeration stops after that many covers.
inline Covers collect(DualizationBacktrackAlgorithmBase& alg, int maxCount = -1) {
    CoverCollector collector;
    collector.MaxCount = maxCount;
    auto* saved = alg.CoverCallback;
    alg.CoverCallback = &collector;
    alg.dualize();
    alg.CoverCallback = saved;

    Covers covers;
    for (auto const& c : collector.GetCovers())
        CHECK_MESSAGE(covers.insert(sorted(c.columns)).second, "repeated cover");
    return covers;
}

// Expected cover count from a file name like "15_20_275.hg", or -1
inline int expectedCount(std::filesystem::path const& file) {
    auto stem = file.stem().string();
    auto pos = stem.rfind('_');
    bool numeric = std::all_of(stem.begin(), stem.end(), [](char c) { return c == '_' || std::isdigit(c); });
    if (!numeric || std::count(stem.begin(), stem.end(), '_') != 2)
        return -1;
    return std::stoi(stem.substr(pos + 1));
}

// All data/*.hg inputs, sorted by name
inline std::vector<std::filesystem::path> dataFiles() {
    std::vector<std::filesystem::path> files;
    for (auto const& entry : std::filesystem::directory_iterator("data"))
        if (entry.path().extension() == ".hg")
            files.push_back(entry.path());
    std::sort(files.begin(), files.end());
    return files;
}

// Rows as bit masks for RuncFixed (all columns must be below the bit width of T)
template<typename T>
std::vector<T> toBits(Rows const& rows) {
    std::vector<T> result;
    for (auto const& r : rows) {
        T mask = 0;
        for (int j : r)
            mask |= T(1) << j;
        result.push_back(mask);
    }
    return result;
}

template<typename T>
Cover fromBits(T mask) {
    Cover c;
    for (int j = 0; j < int(sizeof(T) * 8); ++j)
        if ((mask >> j) & 1)
            c.push_back(j);
    return c;
}

}  // namespace dual_test
