// Regression tests for the resumable RUNC API and the librunc C ABI (issue #24)

#include <doctest.h>

#include <stdexcept>

#include "Counters.h"
#include "CoverCollector.h"
#include "RuncTestUtils.h"

using namespace runc_test;

namespace {

// 6 rows {i, i+1} (a path): several minimal covers, enough for more than one portion
Rows pathRows() {
    Rows rows;
    for (int i = 0; i < 6; ++i)
        rows.push_back({i, i + 1});
    return rows;
}

std::set<Cover> fullEnumeration(Rows const& rows) {
    CoverSet s;
    Runc::RuncGlobalState g(Runc::RUNCM);
    g.setMatrix(makeMatrix(rows));
    for (auto v = g.dualize(1000); !v.empty(); v = g.dualize(1000))
        for (auto& c : v)
            s.add(rows, c);
    return s.covers;
}

struct ThrowingCallback : IDualizationCallback {
    int calls = 0;
    int throwAt = 0;
    void Call(DualizationNode&) override {
        if (++calls == throwAt)
            throw std::runtime_error("callback failure");
    }
};

}  // namespace

// Finding 9: dualize(max_count) / dualize(buffer) left a dangling CoverCallback and overwrote the user's one
TEST_CASE("RuncApi: batched dualize restores the user's CoverCallback") {
    auto rows = pathRows();
    auto expected = fullEnumeration(rows);
    REQUIRE(expected.size() > 2);

    Runc::RuncGlobalState g(Runc::RUNCM);
    g.setMatrix(makeMatrix(rows));
    Counters counters;
    g.CoverCallback = &counters;

    CHECK(g.dualize(size_t(2)).size() == 2);
    CHECK(g.CoverCallback == &counters);

    std::vector<uint32_t> buffer(64);
    CHECK(g.dualize(uint32_t(buffer.size()), buffer.data()) > 0);
    CHECK(g.CoverCallback == &counters);

    // A full run reports to the user's callback, not to a destroyed temporary one
    g.dualize();
    CHECK(counters.cover == static_cast<int>(expected.size()));
}

TEST_CASE("RuncApi: CoverCallback is restored and the enumeration reset when a callback throws") {
    auto rows = pathRows();
    auto expected = fullEnumeration(rows);

    Runc::RuncGlobalState g(Runc::RUNCM);
    g.setMatrix(makeMatrix(rows));
    Counters counters;
    ThrowingCallback inner;
    inner.throwAt = 5;
    g.CoverCallback = &counters;
    g.InnerCallback = &inner;

    CHECK_THROWS_AS(g.dualize(size_t(100)), std::runtime_error);
    CHECK(g.CoverCallback == &counters);

    g.InnerCallback = nullptr;
    CoverSet s;
    for (auto v = g.dualize(size_t(3)); !v.empty(); v = g.dualize(size_t(3)))
        for (auto& c : v)
            s.add(rows, c);
    CHECK(s.covers == expected);
    CHECK(s.duplicates == 0);
}

// Review of PR #27: a callback that stopped the node left the enumeration Running forever
TEST_CASE("RuncApi: a callback stop finishes the enumeration") {
    auto rows = pathRows();
    auto expected = fullEnumeration(rows);
    REQUIRE(expected.size() > 2);

    Runc::RuncGlobalState g(Runc::RUNCM);
    g.setMatrix(makeMatrix(rows));
    CoverCollector collector;
    collector.MaxCount = 2;
    g.CoverCallback = &collector;
    g.dualize();
    CHECK(collector.GetCovers().size() == 2);
    CHECK(g.isDualizeFinished());

    // Further portions return nothing, the enumeration stays finished
    CHECK(g.dualize(size_t(100)).empty());
    std::vector<uint32_t> buffer(64);
    CHECK(g.dualize(uint32_t(buffer.size()), buffer.data()) == 0);
    CHECK(g.isDualizeFinished());
    CHECK(g.CoverCallback == &collector);

    // resetDualize() restarts the enumeration from the first cover
    g.resetDualize();
    CHECK_FALSE(g.isDualizeFinished());
    CoverSet s;
    for (auto v = g.dualize(size_t(3)); !v.empty(); v = g.dualize(size_t(3)))
        for (auto& c : v)
            s.add(rows, c);
    CHECK(s.covers == expected);
    CHECK(s.duplicates == 0);
    CHECK(g.isDualizeFinished());

    // A stop by the InnerCallback during a portion finishes the enumeration as well
    struct StopInner : IDualizationCallback {
        void Call(DualizationNode& node) override {
            node.stoped = true;
        }
    } stop;
    g.resetDualize();
    g.InnerCallback = &stop;
    g.dualize(size_t(100));
    CHECK(g.isDualizeFinished());
    CHECK(g.dualize(size_t(100)).empty());
    g.InnerCallback = nullptr;

    // A full run starts over after a stop
    collector.Reset();
    collector.MaxCount = 0;
    g.dualize();
    CHECK(collector.GetCovers().size() == expected.size());
}

// Finding 10: the enumeration state survived setMatrix and pointed into the freed matrix
TEST_CASE("RuncApi: setMatrix restarts the enumeration") {
    Runc::RuncGlobalState g(Runc::RUNCM);
    g.setMatrix(makeMatrix(pathRows()));
    CHECK(g.dualize(size_t(2)).size() == 2);

    Rows b = {{0}};
    g.setMatrix(makeMatrix(b));
    CoverSet s;
    for (auto v = g.dualize(size_t(100)); !v.empty(); v = g.dualize(size_t(100)))
        for (auto& c : v)
            s.add(b, c);
    CHECK(s.covers == std::set<Cover>{{0}});
    CHECK(g.isDualizeFinished());
}

TEST_CASE("RuncApi: mutable get_matrix restarts the enumeration") {
    auto rows = pathRows();
    Runc::RuncGlobalState g(Runc::RUNCM);
    g.setMatrix(makeMatrix(rows));
    auto first = g.dualize(size_t(2));
    REQUIRE(first.size() == 2);

    g.get_matrix();  // may modify the matrix
    CoverSet s;
    for (auto v = g.dualize(size_t(100)); !v.empty(); v = g.dualize(size_t(100)))
        for (auto& c : v)
            s.add(rows, c);
    CHECK(s.covers == fullEnumeration(rows));
    CHECK(s.duplicates == 0);
}

TEST_CASE("RuncApi: C ABI restarts the enumeration after set_input_row") {
    auto h = init_dualizer();
    REQUIRE(h);
    Rows rows;
    for (int i = 0; i < 6; ++i) {
        int32_t r[] = {i, i + 1};
        REQUIRE(set_input_row(h, i, 2, r) == RUNC_OK);
        rows.push_back({i, i + 1});
    }
    std::vector<uint32_t> buffer(8);
    CHECK(enumerate_covers(h, buffer.size(), buffer.data()) > 0);

    // Grow the input in the middle of the enumeration
    for (int i = 6; i < 24; ++i) {
        int32_t r[] = {i, i + 1};
        REQUIRE(set_input_row(h, i, 2, r) == RUNC_OK);
        rows.push_back({i, i + 1});
    }
    buffer.assign(40, 0);
    CoverSet s;
    for (int32_t c; (c = enumerate_covers(h, buffer.size(), buffer.data())) > 0;)
        addRecords(rows, s, buffer.data(), c);
    CHECK(s.duplicates == 0);
    CHECK(s.notMinimal == 0);
    CHECK(s.covers == fullEnumeration(rows));
    release_dualizer(h);
}

// Finding 11: empty input crashed on an assert; semantics: no rows -> one empty cover, zero row -> no covers
TEST_CASE("RuncApi: matrix without rows has exactly the empty cover") {
    for (auto t : {Runc::OPT, Runc::RUNC, Runc::RUNCM}) {
        Runc::RuncGlobalState full(t);
        full.setMatrix(bit_matrix());
        Counters counters;
        full.CoverCallback = &counters;
        full.dualize();
        CHECK(counters.cover == 1);

        Runc::RuncGlobalState steps(t);
        auto v = steps.dualize(size_t(10));
        REQUIRE(v.size() == 1);
        CHECK(v[0].empty());
        CHECK(steps.dualize(size_t(10)).empty());

        Runc::RuncGlobalState buffered(t);
        uint32_t buffer[4] = {77, 77, 77, 77};
        CHECK(buffered.dualize(uint32_t(1), buffer) == 1);
        CHECK(buffer[0] == 0);
        CHECK(buffer[1] == 77);
        CHECK(buffered.dualize(uint32_t(4), buffer) == 0);
        CHECK(buffered.isDualizeFinished());
    }

    auto h = init_dualizer();
    uint32_t buffer[2] = {77, 77};
    CHECK(enumerate_covers(h, 1, buffer) == 1);
    CHECK(buffer[0] == 0);
    CHECK(enumerate_covers(h, 1, buffer) == 0);
    release_dualizer(h);
}

TEST_CASE("RuncApi: a zero row means no covers") {
    for (auto t : {Runc::OPT, Runc::RUNC, Runc::RUNCM}) {
        Rows rows = {{0, 1}, {}, {1, 2}};
        Runc::RuncGlobalState g(t);
        g.setMatrix(makeMatrix(rows));
        Counters counters;
        g.CoverCallback = &counters;
        g.dualize();
        CHECK(counters.cover == 0);
        CHECK(g.dualize(size_t(10)).empty());
        CHECK(g.isDualizeFinished());

        Runc::RuncGlobalState only_zero_rows(t);
        only_zero_rows.setMatrix(makeMatrix(Rows{{}, {}}));
        CHECK(only_zero_rows.dualize(size_t(10)).empty());
    }

    auto h = init_dualizer();
    int32_t r0[] = {0, 1}, r2[] = {1, 2};
    CHECK(set_input_row(h, 0, 2, r0) == RUNC_OK);
    CHECK(set_input_row(h, 1, 0, nullptr) == RUNC_OK);
    CHECK(set_input_row(h, 2, 2, r2) == RUNC_OK);
    uint32_t buffer[16];
    CHECK(enumerate_covers(h, 16, buffer) == 0);
    release_dualizer(h);

    // A row that is never given is a zero row too
    h = init_dualizer();
    CHECK(set_input_row(h, 0, 2, r0) == RUNC_OK);
    CHECK(set_input_row(h, 2, 2, r2) == RUNC_OK);
    CHECK(enumerate_covers(h, 16, buffer) == 0);
    release_dualizer(h);
}

// Finding 15: a too small buffer returned 0, indistinguishable from the end of the enumeration
TEST_CASE("RuncApi: C ABI reports a too small buffer") {
    auto h = init_dualizer();
    int32_t r[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    REQUIRE(set_input_row(h, 0, 10, r) == RUNC_OK);

    uint32_t buffer[11];
    CHECK(enumerate_covers(h, 5, buffer) == RUNC_ERROR_BUFFER_TOO_SMALL);
    CHECK(enumerate_covers(h, 10, buffer) == RUNC_ERROR_BUFFER_TOO_SMALL);

    // The failed calls did not advance the enumeration: all 10 single-column covers follow
    int total = 0;
    for (int32_t c; (c = enumerate_covers(h, 11, buffer)) > 0;)
        total += c;
    CHECK(total == 10);
    CHECK(enumerate_covers(h, 11, buffer) == 0);
    CHECK(enumerate_covers(h, 1, buffer) == RUNC_ERROR_BUFFER_TOO_SMALL);
    release_dualizer(h);

    Runc::RuncGlobalState g;
    g.setMatrix(makeMatrix(Rows{{0, 1, 2}}));
    CHECK(g.dualize(uint32_t(3), buffer) == 0);
    CHECK_FALSE(g.isDualizeFinished());
    CHECK(g.dualize(uint32_t(4), buffer) == 1);
}

TEST_CASE("RuncApi: C ABI rejects invalid arguments") {
    release_dualizer(nullptr);

    int32_t ok[] = {0};
    int32_t negative[] = {1, -1};
    int32_t too_big[] = {INT32_MAX};
    uint32_t buffer[8];
    CHECK(set_input_row(nullptr, 0, 1, ok) == RUNC_ERROR_INVALID_ARGUMENT);
    CHECK(enumerate_covers(nullptr, 8, buffer) == RUNC_ERROR_INVALID_ARGUMENT);

    auto h = init_dualizer();
    REQUIRE(h != nullptr);
    CHECK(set_input_row(h, -1, 1, ok) == RUNC_ERROR_INVALID_ARGUMENT);
    CHECK(set_input_row(h, INT32_MAX, 1, ok) == RUNC_ERROR_INVALID_ARGUMENT);
    CHECK(set_input_row(h, 0, -1, ok) == RUNC_ERROR_INVALID_ARGUMENT);
    CHECK(set_input_row(h, 0, 1, nullptr) == RUNC_ERROR_INVALID_ARGUMENT);
    CHECK(set_input_row(h, 0, 2, negative) == RUNC_ERROR_INVALID_ARGUMENT);
    CHECK(set_input_row(h, 0, 1, too_big) == RUNC_ERROR_INVALID_ARGUMENT);
    CHECK(enumerate_covers(h, 8, nullptr) == RUNC_ERROR_INVALID_ARGUMENT);

    // Rejected rows left the input unchanged: still no rows, so just the empty cover
    CHECK(enumerate_covers(h, 8, buffer) == 1);
    CHECK(buffer[0] == 0);
    release_dualizer(h);
}

// Findings 6 and 8 as seen through the C ABI: rows out of order and bits beyond a row's stride
TEST_CASE("RuncApi: C ABI accepts rows in any order and growing rows") {
    auto h = init_dualizer();
    int32_t a[] = {0}, b[] = {1};
    REQUIRE(set_input_row(h, 1, 1, a) == RUNC_OK);
    REQUIRE(set_input_row(h, 0, 1, b) == RUNC_OK);
    uint32_t buffer[16];
    REQUIRE(enumerate_covers(h, 16, buffer) == 1);
    CHECK(buffer[0] == 2);
    CHECK(Cover{int(buffer[1]), int(buffer[2])} == Cover{1, 0});
    release_dualizer(h);

    h = init_dualizer();
    int32_t c40[] = {40};
    REQUIRE(set_input_row(h, 0, 1, a) == RUNC_OK);
    REQUIRE(set_input_row(h, 1, 1, b) == RUNC_OK);
    REQUIRE(set_input_row(h, 0, 1, c40) == RUNC_OK);
    Rows rows = {{0, 40}, {1}};
    std::vector<uint32_t> big(64);
    CoverSet s;
    for (int32_t c; (c = enumerate_covers(h, big.size(), big.data())) > 0;)
        addRecords(rows, s, big.data(), c);
    CHECK(s.covers == std::set<Cover>{{0, 1}, {1, 40}});
    release_dualizer(h);
}

// Finding 17: setRestrictions wrote through a copy of the row
TEST_CASE("RuncApi: setRestrictions fills the restriction rows") {
    struct Exposed : Runc::RuncGlobalState {
        bit_matrix const& restrictions() const {
            return _restrictions;
        }
    } g;
    g.setRestrictions(makeMatrix(Rows{{0, 1}, {2}}));
    CHECK(g.restrictions().get(0, 0));
    CHECK(g.restrictions().get(0, 1));
    CHECK(g.restrictions().get(1, 1));
    CHECK(g.restrictions().get(2, 2));
    CHECK_FALSE(g.restrictions().get(2, 0));
}
