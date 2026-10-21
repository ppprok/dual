#include <doctest.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <functional>
#include <memory>
#include <random>
#include <set>
#include <string>
#include <vector>

#include "AOT1.h"
#include "CoverCollector.h"
#include "PUNC.h"
#include "Runc.h"

namespace {

typedef std::vector<int> Cover;
typedef std::set<Cover> CoverSet;
typedef std::vector<std::vector<int>> Rows;

// Runs the algorithm and returns its covers with sorted columns; fails on a repeated cover.
// With maxCount > 0 the enumeration stops after that many covers.
CoverSet Collect(DualizationBacktrackAlgorithmBase& alg, int maxCount = -1) {
    CoverCollector collector;
    collector.MaxCount = maxCount;
    alg.CoverCallback = &collector;
    alg.dualize();
    alg.CoverCallback = nullptr;

    CoverSet covers;
    for (auto const& c : collector.GetCovers()) {
        Cover cover = c.columns;
        std::sort(cover.begin(), cover.end());
        CHECK_MESSAGE(covers.insert(cover).second, "repeated cover");
    }
    return covers;
}

CoverSet RuncmCovers(bit_matrix matrix, int maxCount = -1) {
    Runc::RuncGlobalState runc(Runc::RUNCM);
    runc.setMatrix(std::move(matrix));
    return Collect(runc, maxCount);
}

struct NamedAlgorithm {
    std::string name;
    std::function<std::unique_ptr<DualizationBacktrackAlgorithmBase>()> create;
};

std::vector<NamedAlgorithm> Algorithms() {
    std::vector<NamedAlgorithm> algs;
    std::pair<AOT1::AlgorithmType, const char*> types[] = {{AOT1::AO1, "AO1"},
                                                           {AOT1::AO2, "AO2"},
                                                           {AOT1::AO1K, "AO1K"},
                                                           {AOT1::AO2K, "AO2K"},
                                                           {AOT1::AO1M, "AO1M"},
                                                           {AOT1::AO2M, "AO2M"}};
    for (auto t : types) {
        auto type = t.first;
        algs.push_back({std::string("AOT1::") + t.second, [type] {
                            return std::unique_ptr<DualizationBacktrackAlgorithmBase>(new AOT1::GlobalState(type));
                        }});
    }
    algs.push_back(
            {"PUNC", [] { return std::unique_ptr<DualizationBacktrackAlgorithmBase>(new PUNC::GlobalState()); }});
    return algs;
}

// Zero rows get width n, or the largest column in rows + 1 if that is larger
bit_matrix MakeMatrix(Rows const& rows, int n = 0) {
    for (auto const& row : rows)
        for (int j : row)
            n = std::max(n, j + 1);

    bit_matrix matrix;
    for (int i = 0; i < (int) rows.size(); ++i) {
        // set() alone would drop a trailing zero row and put a leading one's successor in its place
        if (rows[i].empty())
            matrix.add_rows(1, n);
        for (int j : rows[i])
            matrix.set(i, j);
    }
    return matrix;
}

// bit_matrix is not copyable, so matrices are passed around as rows of column indices
Rows ToRows(bit_matrix const& matrix) {
    Rows rows(matrix.height());
    for (int i = 0; i < matrix.height(); ++i) {
        auto const& row = matrix.get_row(i);
        for (int j = row.find_first(); j >= 0; j = row.find_next(j))
            rows[i].push_back(j);
    }
    return rows;
}

Rows LoadRows(std::string const& filename) {
    Runc::RuncGlobalState reader;
    reader.loadMatrix(filename);
    return ToRows(reader.get_matrix());
}

// Random matrix; about one matrix in eight gets a zero row, the rest have none
Rows RandomRows(std::mt19937& random, int m, int n) {
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
    if (random() % 8 == 0)
        rows[random() % m].clear();
    return rows;
}

bool IsCover(Rows const& rows, unsigned mask) {
    for (auto const& row : rows) {
        bool covered = false;
        for (int j : row)
            covered |= ((mask >> j) & 1) != 0;
        if (!covered)
            return false;
    }
    return true;
}

// Minimal covers by exhaustive search over the column subsets (n <= 16)
CoverSet BruteForceCovers(Rows const& rows, int n) {
    CoverSet covers;
    for (unsigned mask = 0; mask < (1u << n); ++mask) {
        if (!IsCover(rows, mask))
            continue;
        bool minimal = true;
        for (int j = 0; j < n && minimal; ++j)
            if (((mask >> j) & 1) && IsCover(rows, mask & ~(1u << j)))
                minimal = false;
        if (!minimal)
            continue;
        Cover cover;
        for (int j = 0; j < n; ++j)
            if ((mask >> j) & 1)
                cover.push_back(j);
        covers.insert(cover);
    }
    return covers;
}

// Expected cover count from a file name like "15_20_275.hg", or -1
int ExpectedCount(std::filesystem::path const& file) {
    auto stem = file.stem().string();
    auto pos = stem.rfind('_');
    bool numeric = std::all_of(stem.begin(), stem.end(), [](char c) { return c == '_' || std::isdigit(c); });
    if (!numeric || std::count(stem.begin(), stem.end(), '_') != 2)
        return -1;
    return std::stoi(stem.substr(pos + 1));
}

std::vector<std::filesystem::path> DataFiles() {
    std::vector<std::filesystem::path> files;
    for (auto const& entry : std::filesystem::directory_iterator("data"))
        if (entry.path().extension() == ".hg")
            files.push_back(entry.path());
    std::sort(files.begin(), files.end());
    return files;
}

}  // namespace

TEST_CASE("AOT1 and PUNC match RUNCM on data files") {
    auto files = DataFiles();
    REQUIRE(files.size() >= 8);

    for (auto const& file : files) {
        CAPTURE(file.string());
        auto rows = LoadRows(file.string());
        auto expected = RuncmCovers(MakeMatrix(rows));

        int count = ExpectedCount(file);
        if (count >= 0)
            CHECK_EQ(count, (int) expected.size());

        for (auto const& alg : Algorithms()) {
            CAPTURE(alg.name);
            auto a = alg.create();
            a->setMatrix(MakeMatrix(rows));
            CHECK(Collect(*a) == expected);
        }
    }
}

TEST_CASE("AOT1 and PUNC match RUNCM and brute force on random small matrices") {
    std::mt19937 random(23);
    auto algs = Algorithms();

    for (int it = 0; it < 600; ++it) {
        int n = 1 + random() % 12;
        int m = 1 + random() % 12;
        auto rows = RandomRows(random, m, n);
        CAPTURE(it);

        auto expected = RuncmCovers(MakeMatrix(rows, n));
        REQUIRE(expected == BruteForceCovers(rows, n));

        for (auto const& alg : algs) {
            CAPTURE(alg.name);
            auto a = alg.create();
            a->setMatrix(MakeMatrix(rows, n));
            CHECK(Collect(*a) == expected);
        }
    }
}

TEST_CASE("AOT1 and PUNC match RUNCM on random larger matrices") {
    // Matrices with too many covers are skipped to keep the test fast
    const int maxCovers = 3000;
    std::mt19937 random(2323);
    auto algs = Algorithms();

    int tested = 0;
    for (int it = 0; it < 200 && tested < 60; ++it) {
        int n = 1 + random() % 40;
        int m = 1 + random() % 16;
        auto rows = RandomRows(random, m, n);
        CAPTURE(it);

        auto expected = RuncmCovers(MakeMatrix(rows, n), maxCovers + 1);
        if ((int) expected.size() > maxCovers)
            continue;
        ++tested;
        for (auto const& alg : algs) {
            CAPTURE(alg.name);
            auto a = alg.create();
            a->setMatrix(MakeMatrix(rows, n));
            CHECK(Collect(*a) == expected);
        }
    }
    CHECK(tested >= 30);
}

TEST_CASE("AOT1 and PUNC edge matrices") {
    // A single cover of the maximal depth: PUNC used to reallocate its states during recursion here
    Rows single = {{0}};
    Rows tall = {{0, 1}, {1, 2}, {0, 2}, {0}, {2}};
    Rows diagonal = {{0}, {1}, {2}, {3}, {4}, {5}};
    // A zero row with n < m: the only case where AOT1 reaches state q + 1, one more than it used to reserve
    Rows zeroRowTall = {{0}, {}, {0}, {0}};

    for (auto const& rows : {single, tall, diagonal, zeroRowTall}) {
        auto expected = RuncmCovers(MakeMatrix(rows));
        if (rows == zeroRowTall)
            CHECK(expected.empty());
        for (auto const& alg : Algorithms()) {
            CAPTURE(alg.name);
            auto a = alg.create();
            a->setMatrix(MakeMatrix(rows));
            CHECK(Collect(*a) == expected);
        }
    }
}

TEST_CASE("All algorithms agree on the empty matrix") {
    // The dual of the empty hypergraph is {empty set}
    CoverSet expected = {Cover()};
    for (auto type : {Runc::OPT, Runc::RUNC, Runc::RUNCM}) {
        CAPTURE(type);
        Runc::RuncGlobalState runc(type);
        runc.setMatrix(bit_matrix());
        CHECK(Collect(runc) == expected);
    }
    {
        // The resumable API reports the empty cover once, then nothing
        Runc::RuncGlobalState runc;
        runc.setMatrix(bit_matrix());
        CHECK_EQ(1, (int) runc.dualize(10).size());
        CHECK(runc.dualize(10).empty());
    }
    for (auto const& alg : Algorithms()) {
        CAPTURE(alg.name);
        auto a = alg.create();
        a->setMatrix(bit_matrix());
        CHECK(Collect(*a) == expected);
    }
}

TEST_CASE("AOT1 and PUNC repeated dualize on the same object") {
    Rows small = {{0, 1}, {1, 2}};
    Rows big = LoadRows("data/15_20_275.hg");

    auto expectedSmall = RuncmCovers(MakeMatrix(small));
    auto expectedBig = RuncmCovers(MakeMatrix(big));
    REQUIRE_EQ(2, (int) expectedSmall.size());
    REQUIRE_EQ(275, (int) expectedBig.size());

    for (auto const& alg : Algorithms()) {
        CAPTURE(alg.name);
        auto a = alg.create();

        a->setMatrix(MakeMatrix(small));
        CHECK(Collect(*a) == expectedSmall);
        CHECK(Collect(*a) == expectedSmall);

        // A larger matrix on the same object, then back to the smaller one
        a->setMatrix(MakeMatrix(big));
        CHECK(Collect(*a) == expectedBig);
        CHECK(Collect(*a) == expectedBig);

        a->setMatrix(MakeMatrix(small));
        CHECK(Collect(*a) == expectedSmall);
    }
}

TEST_CASE("PUNC with rows and columns order") {
    PUNC::GlobalState punc;
    punc.loadMatrix("data/10_25_480.hg");
    auto expected = RuncmCovers(MakeMatrix(ToRows(punc.get_matrix())));
    REQUIRE_EQ(480, (int) expected.size());

    Weights cw(25);
    for (int j = 0; j < 25; ++j)
        cw[j] = std::sin((Weight) j);

    Weights rw(10);
    for (int i = 0; i < 10; ++i)
        rw[i] = std::cos((Weight) i);

    punc.SetRowsOrder(rw);
    punc.SetColsOrder(std::move(cw));

    CHECK(Collect(punc) == expected);
}
