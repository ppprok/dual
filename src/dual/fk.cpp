#include <dual/fk.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <utility>
#include <vector>

#include <dual/bits.h>

// Notation. The families F (the hypergraph) and G (the covers found so far, or the second argument
// of isDual) define the monotone Boolean functions f and g: f(Y) = 1 if Y contains a set of F. The
// families are dual (G = Tr(F)) if and only if f(C \ X) != g(X) for every set X of the columns C.
// A witness of non-duality is a set X with f(C \ X) == g(X): either both are 0 (X is a transversal
// of F that contains no set of G) or both are 1 (a set of G misses a set of F).
//
// Splitting on a column x: F0 are the sets of F without x, F1 the sets with x, with x removed (the
// same for G). F and G are dual if and only if both pairs
//   (min(F0 + F1), G0)  and  (F0, min(G0 + G1))
// are dual (algorithm A). Algorithm B replaces one pair by several smaller ones: if the second
// pair is dual, the first one is dual if and only if (F1 restricted to the sets disjoint with t,
// G0 with the columns of t removed) is dual for every t in G1 (alpha below); symmetrically, if the
// first pair is dual, the second one is dual if and only if (F0 with t removed, G1 restricted to
// the sets disjoint with t) is dual for every t in F1 (beta below). A witness of a subproblem is
// lifted to a witness of F and G by fixing x and the columns of t (see Solver::lift).
//
// Implementation. The recursion is kept on an explicit stack of frames, so the generation can stop
// after a new cover and continue from the same place later (GlobalState::next). Sets are rows of
// 64-bit words stored contiguously (Family). When a cover is found at the leaf, only the leaf's
// input G gets its image at once; the split families of the frames above get the new covers
// lazily, when the frame goes on to its next subproblem (Solver::sync), and frames that have no
// subproblem left never get them.

namespace dual {

namespace FK {

namespace {

typedef std::uint64_t Word;
constexpr int wordBits = 64;

// Words for sets over n columns (at least one, so rows need memory even without columns)
int wordsFor(int n) {
    return std::max(1, (n + wordBits - 1) / wordBits);
}

// Columns of the work families: at least one
int workWidth(int n) {
    return std::max(n, 1);
}

// Index of the lowest set bit of a non-zero word (de Bruijn multiplication, portable)
constexpr Word deBruijn = 0x03f79d71b4cb0a89ULL;

constexpr std::array<int, 64> makeLowBitTable() {
    std::array<int, 64> table{};
    for (int i = 0; i < 64; ++i)
        table[(std::size_t) (((Word(1) << i) * deBruijn) >> 58)] = i;
    return table;
}

constexpr std::array<int, 64> lowBitTable = makeLowBitTable();

inline int lowBit(Word v) {
    return lowBitTable[(std::size_t) (((v & (~v + 1)) * deBruijn) >> 58)];
}

// Operations on sets of w words; columns are j >= 0
inline Word bitOf(int j) {
    assert(j >= 0);
    return Word(1) << ((unsigned) j % wordBits);
}

inline bool getBit(Word const* a, int j) {
    return (a[(unsigned) j / wordBits] & bitOf(j)) != 0;
}

inline void setBit(Word* a, int j) {
    a[(unsigned) j / wordBits] |= bitOf(j);
}

inline void resetBit(Word* a, int j) {
    a[(unsigned) j / wordBits] &= ~bitOf(j);
}

inline void clearSet(Word* a, int w) {
    std::fill(a, a + w, Word(0));
}

inline void copySet(Word* dst, Word const* a, int w) {
    std::copy(a, a + w, dst);
}

inline bool isEmpty(Word const* a, int w) {
    for (int k = 0; k < w; ++k) {
        if (a[k])
            return false;
    }
    return true;
}

inline bool isSubset(Word const* a, Word const* b, int w) {
    for (int k = 0; k < w; ++k) {
        if (a[k] & ~b[k])
            return false;
    }
    return true;
}

inline bool equalSets(Word const* a, Word const* b, int w) {
    return std::equal(a, a + w, b);
}

inline bool intersects(Word const* a, Word const* b, int w) {
    for (int k = 0; k < w; ++k) {
        if (a[k] & b[k])
            return true;
    }
    return false;
}

inline int countSet(Word const* a, int w) {
    int c = 0;
    for (int k = 0; k < w; ++k)
        c += bits_count((bits64) a[k]);
    return c;
}

// First column of a, or -1
inline int firstBit(Word const* a, int w) {
    for (int k = 0; k < w; ++k) {
        if (a[k])
            return k * wordBits + lowBit(a[k]);
    }
    return -1;
}

// dst = a | b, a & b, a \ b (dst may be a or b)
inline void assignOr(Word* dst, Word const* a, Word const* b, int w) {
    for (int k = 0; k < w; ++k)
        dst[k] = a[k] | b[k];
}

inline void assignAnd(Word* dst, Word const* a, Word const* b, int w) {
    for (int k = 0; k < w; ++k)
        dst[k] = a[k] & b[k];
}

inline void assignSub(Word* dst, Word const* a, Word const* b, int w) {
    for (int k = 0; k < w; ++k)
        dst[k] = a[k] & ~b[k];
}

// Call fn(j) for every column j of a, in increasing order
template<typename Fn>
inline void forEachBit(Word const* a, int w, Fn fn) {
    for (int k = 0; k < w; ++k) {
        for (Word v = a[k]; v; v &= v - 1)
            fn(k * wordBits + lowBit(v));
    }
}

typedef std::vector<Word> Set;

// Family of sets: rows of w words stored one after another
class Family {
public:
    explicit Family(int w = 1) : w_(w) {}

    // Remove all sets and set the number of words per set
    void reset(int w) {
        w_ = w;
        clear();
    }

    void clear() {
        rows_ = 0;
        data_.clear();
    }

    int words() const {
        return w_;
    }

    int size() const {
        return rows_;
    }

    bool empty() const {
        return rows_ == 0;
    }

    Word* row(int i) {
        return data_.data() + (std::size_t) i * w_;
    }

    Word const* row(int i) const {
        return data_.data() + (std::size_t) i * w_;
    }

    // Append an empty set; the pointer is valid until the family grows again
    Word* addRow() {
        ++rows_;
        data_.resize((std::size_t) rows_ * w_, Word(0));
        return row(rows_ - 1);
    }

    // Append a copy of s (s must not point into this family)
    void add(Word const* s) {
        copySet(addRow(), s, w_);
    }

    // Index of a set that is a subset of s, or -1
    int findSubset(Word const* s) const {
        if (w_ == 1) {
            Word out = ~s[0];
            for (int i = 0; i < rows_; ++i) {
                if (!(data_[i] & out))
                    return i;
            }
            return -1;
        }
        for (int i = 0; i < rows_; ++i) {
            if (isSubset(row(i), s, w_))
                return i;
        }
        return -1;
    }

    // Index of a set equal to s, or -1
    int findEqual(Word const* s) const {
        for (int i = 0; i < rows_; ++i) {
            if (equalSets(row(i), s, w_))
                return i;
        }
        return -1;
    }

    // Index of the first set with column j, or -1
    int findWith(int j) const {
        for (int i = 0; i < rows_; ++i) {
            if (getBit(row(i), j))
                return i;
        }
        return -1;
    }

    // Remove the supersets of s, keeping the order of the other sets
    void removeSupersets(Word const* s) {
        int k = 0;
        if (w_ == 1) {
            Word v = s[0];
            for (int i = 0; i < rows_; ++i) {
                if ((data_[i] & v) != v)
                    data_[k++] = data_[i];
            }
        } else {
            for (int i = 0; i < rows_; ++i) {
                if (isSubset(s, row(i), w_))
                    continue;
                if (k != i)
                    copySet(row(k), row(i), w_);
                ++k;
            }
        }
        truncate(k);
    }

    // Keep the sets i with keep[i], in their order
    void compact(std::vector<char> const& keep) {
        int k = 0;
        for (int i = 0; i < rows_; ++i) {
            if (!keep[i])
                continue;
            if (k != i)
                copySet(row(k), row(i), w_);
            ++k;
        }
        truncate(k);
    }

private:
    void truncate(int m) {
        rows_ = m;
        data_.resize((std::size_t) m * w_);
    }

    int w_;
    int rows_ = 0;
    std::vector<Word> data_;
};

// Family of the rows of a matrix over w words
Family familyOf(bit_matrix const& m, int w) {
    Family f(w);
    for (auto const& r : m.get_rows()) {
        Word* row = f.addRow();
        if (!r.begin)
            continue;
        for (int j = r.find_first(); j >= 0; j = r.find_next(j))
            setBit(row, j);
    }
    return f;
}

void toColumns(Word const* s, int w, Columns& columns) {
    columns.clear();
    forEachBit(s, w, [&columns](int j) { columns.push_back(j); });
}

// Work memory of removeDominated
struct DominationWork {
    std::vector<int> dominators;  // sets that may be subsets of other sets
    std::vector<int> counts;      // size of each set
    std::vector<Word> columns;    // for each column, the bit set of the sets that contain it
    std::vector<Word> supersets;  // bit set of the supersets of a dominator
    std::vector<char> keep;
};

// Keep only the minimal sets of m over n columns (drop duplicates and supersets of other sets), in
// their order. Only the sets listed in work.dominators are compared with the others: the caller
// knows that no other set is a subset of another set. The supersets of a dominator are found as
// the intersection of the bit sets (over the sets of m) of its columns.
void removeDominated(Family& m, int n, DominationWork& work) {
    int s = m.size();
    int w = m.words();
    if (work.dominators.empty() || s < 2)
        return;
    int sw = (s + wordBits - 1) / wordBits;
    work.counts.resize(s);
    work.keep.assign(s, 1);
    work.columns.assign((std::size_t) n * sw, Word(0));
    for (int i = 0; i < s; ++i) {
        work.counts[i] = countSet(m.row(i), w);
        Word bit = Word(1) << (i % wordBits);
        forEachBit(m.row(i), w, [&](int c) { work.columns[(std::size_t) c * sw + i / wordBits] |= bit; });
    }

    work.supersets.resize(sw);
    Word* sup = work.supersets.data();
    for (int j : work.dominators) {
        // A dominated dominator has the same supersets as the set dominating it, or fewer
        if (!work.keep[j])
            continue;
        std::fill(sup, sup + sw, ~Word(0));
        if (s % wordBits)
            sup[sw - 1] = (Word(1) << (s % wordBits)) - 1;
        bool any = true;
        forEachBit(m.row(j), w, [&](int c) {
            if (!any)
                return;
            Word const* col = work.columns.data() + (std::size_t) c * sw;
            Word acc = 0;
            for (int k = 0; k < sw; ++k)
                acc |= (sup[k] &= col[k]);
            any = acc != 0;
        });
        if (!any)
            continue;
        int cj = work.counts[j];
        forEachBit(sup, sw, [&](int i) {
            // A superset of the same size is equal; of equal sets the first one is kept
            if (work.counts[i] > cj || (work.counts[i] == cj && j < i))
                work.keep[i] = 0;
        });
    }
    m.compact(work.keep);
}

// Keep only the minimal sets of any family over n columns
void minimize(Family& m, int n) {
    DominationWork work;
    work.dominators.resize(m.size());
    for (int i = 0; i < m.size(); ++i)
        work.dominators[i] = i;
    removeDominated(m, n, work);
}

// dst = min(sets of a with the columns of t removed) for a Sperner family a over n columns. A set
// that misses t stays as it is and is a subset of no other set of the result (it would be a subset
// of another set of a), so only the sets that meet t can make other sets non-minimal.
void subtractMin(Family& dst, Family const& a, Word const* t, int n, DominationWork& work) {
    int w = a.words();
    dst.clear();
    work.dominators.clear();
    for (int i = 0; i < a.size(); ++i) {
        Word const* r = a.row(i);
        if (intersects(r, t, w))
            work.dominators.push_back(dst.size());
        assignSub(dst.addRow(), r, t, w);
    }
    removeDominated(dst, n, work);
}

// dst = min(a + b) for Sperner families a and b where no set of b is a proper subset of a set of a
void unionMin(Family& dst, Family const& a, Family const& b) {
    dst = a;
    for (int i = 0; i < b.size(); ++i) {
        if (a.findSubset(b.row(i)) < 0)
            dst.add(b.row(i));
    }
}

// dst = the sets of a disjoint with t
void disjointSets(Family& dst, Family const& a, Word const* t) {
    dst.clear();
    for (int i = 0; i < a.size(); ++i) {
        if (!intersects(a.row(i), t, a.words()))
            dst.add(a.row(i));
    }
}

// Root of c * ln(c) = ln(v), i.e. c^c = v, for v >= 1
double chi(double v) {
    if (v <= 1)
        return 1;
    double lv = std::log(v);
    double lo = 1;
    double hi = lv + 2;
    for (int k = 0; k < 64; ++k) {
        double c = (lo + hi) / 2;
        if (c * std::log(c) < lv)
            lo = c;
        else
            hi = c;
    }
    return hi;
}

}  // namespace

// The duality test and the generation of covers, as a resumable depth-first run over an explicit
// stack of frames. One frame per depth is kept between runs, so the families of the subproblems are
// allocated once.
class Solver {
public:
    // Called for every new cover; returns false to stop the generation
    typedef std::function<bool(Columns const&)> Report;

    enum class Result {
        Dual,     // the families are dual: the test is over, or all covers have been generated
        NotDual,  // test: the families are not dual, witness() holds a witness
        Found,    // generation with once: a new cover was reported, run() continues from here
        Stopped   // generation: report returned false
    };

    // Start a test of Sperner families f and g over n >= 1 columns whose sets pairwise intersect.
    // For the generation, g is a subset of Tr(f) and receives the new covers.
    void init(int n, Family f, Family g);

    // Run (or continue) the test. Without report it is a duality test that stops at the first
    // witness. With report it generates the minimal transversals of f that are not in g, adding
    // each to g and passing it to report; with once, it stops after the first one.
    Result run(Report const* report, bool once);

    // Witness of the last run that returned NotDual
    Columns witness() const {
        Columns columns;
        toColumns(witness_.data(), w_, columns);
        return columns;
    }

    Family const& rootF() const {
        return rootF_;
    }

    Family const& rootG() const {
        return rootG_;
    }

private:
    enum class Base {
        Dual,     // the pair is dual
        Witness,  // witness_ is a witness for the pair
        Split     // no answer without splitting
    };

    enum class Kind {
        AB,     // the two pairs of algorithm A
        Alpha,  // the second pair, then alpha subproblems (or the first pair)
        Beta    // the first pair, then beta subproblems
    };

    enum class Phase {
        None,  // no child yet
        Pair1,
        Pair2,
        Alpha,
        Beta
    };

    struct Frame {
        Family const* f = nullptr;  // input families; g gets the new covers while the frame is a leaf
        Family* g = nullptr;
        bool split = false;  // the frame has been split and goes over its children
        Kind kind = Kind::AB;
        Phase phase = Phase::None;  // the current child
        int x = -1;                 // split column
        int step = 0;               // position in the sequence of children
        int index = 0;              // next set of ts (alpha) or f1 (beta)
        int synced = 0;             // covers (rows of the root G) that are in g0 and g1
        Family f0, f1, g0, g1;      // input families split on x
        Family childF, childG;      // input families of the child, unless f is f0
        Family ts;                  // sets t of the alpha subproblems
        Set t;                      // set t of the alpha or beta subproblem

        explicit Frame(int w) : f0(w), f1(w), g0(w), g1(w), childF(w), childG(w), ts(w), t((std::size_t) w, Word(0)) {}
    };

    Frame& frame(int d);
    Base baseCase(Frame const& fr);
    bool missingTransversal(Word const* a, Word const* b, Family const& other);
    bool volumeWitness(Family const& f, Family const& g);
    void chooseColumn(Frame& fr);
    void split(Frame& fr);
    bool nextChild(Frame& fr, int d);
    void pair1(Frame& fr);
    void pair2(Frame& fr);
    void alpha(Frame& fr, Word const* t);
    void beta(Frame& fr, int i);
    Result found();
    void lift(Frame const& fr);
    void minimalTransversal();
    void imageAt(int d, Word const* cover, Word* image);
    void sync(Frame& fr, int d);

    int n_ = 0;
    int w_ = 0;
    int depth_ = -1;  // top of the stack; -1 when the run is over
    Family rootF_, rootG_;
    std::vector<std::unique_ptr<Frame>> frames_;
    Set witness_, supportF_, supportG_, temp_, image_, pairA_, pairB_, pairC_, pairT_;
    Columns cover_;
    std::vector<int> countF_, countG_, counts_;
    DominationWork work_;
    std::vector<int> restF_, restG_;

    Report const* report_ = nullptr;
    bool once_ = false;
};

void Solver::init(int n, Family f, Family g) {
    assert(n >= 1);
    int w = wordsFor(n);
    if (w != w_)
        frames_.clear();
    n_ = n;
    w_ = w;
    rootF_ = std::move(f);
    rootG_ = std::move(g);
    for (Set* s : {&witness_, &supportF_, &supportG_, &temp_, &image_, &pairA_, &pairB_, &pairC_, &pairT_})
        s->assign((std::size_t) w, Word(0));
    depth_ = 0;
    auto& root = frame(0);
    root.f = &rootF_;
    root.g = &rootG_;
    root.split = false;
}

Solver::Frame& Solver::frame(int d) {
    while ((int) frames_.size() <= d)
        frames_.push_back(std::make_unique<Frame>(w_));
    return *frames_[d];
}

Solver::Result Solver::run(Report const* report, bool once) {
    report_ = report;
    once_ = once;
    while (depth_ >= 0) {
        auto& fr = *frames_[depth_];
        if (!fr.split) {
            switch (baseCase(fr)) {
                case Base::Dual:
                    --depth_;
                    continue;
                case Base::Witness: {
                    auto r = found();
                    if (r == Result::Dual)  // test the leaf pair again with the new cover
                        continue;
                    return r;
                }
                case Base::Split:
                    split(fr);
                    break;
            }
        }
        if (!nextChild(fr, depth_)) {
            --depth_;
            continue;
        }
        auto& c = frame(++depth_);
        c.f = fr.phase == Phase::Pair2 ? &fr.f0 : &fr.childF;
        c.g = &fr.childG;
        c.split = false;
    }
    return Result::Dual;
}

// Answer without splitting where the families are small or obviously not dual. Relies on F and G
// being Sperner with pairwise intersecting sets.
Solver::Base Solver::baseCase(Frame const& fr) {
    auto const& f = *fr.f;
    auto const& g = *fr.g;
    int w = w_;
    Word* wit = witness_.data();
    clearSet(wit, w);

    if (f.empty()) {
        // f = 0 is dual only to g = 1, i.e. G = {{}}; otherwise X = {} has f(C) = g({}) = 0
        if (g.size() == 1 && isEmpty(g.row(0), w))
            return Base::Dual;
        return Base::Witness;
    }

    if (g.empty()) {
        // g = 0 is dual only to F = {{}}; otherwise X = the columns of F has f({}) = g(X) = 0
        if (f.size() == 1 && isEmpty(f.row(0), w))
            return Base::Dual;
        for (int i = 0; i < f.size(); ++i)
            assignOr(wit, wit, f.row(i), w);
        return Base::Witness;
    }

    if (f.size() == 1 && isEmpty(f.row(0), w)) {
        // f = 1 and G is not empty: X = a set of G has f(C \ X) = g(X) = 1
        copySet(wit, g.row(0), w);
        return Base::Witness;
    }

    if (g.size() == 1 && isEmpty(g.row(0), w)) {
        // g = 1 and F is not empty: X = {} has f(C) = g({}) = 1
        return Base::Witness;
    }

    Word* sf = supportF_.data();
    Word* sg = supportG_.data();
    Word* temp = temp_.data();
    clearSet(sf, w);
    for (int i = 0; i < f.size(); ++i)
        assignOr(sf, sf, f.row(i), w);
    clearSet(sg, w);
    for (int i = 0; i < g.size(); ++i)
        assignOr(sg, sg, g.row(i), w);

    if (!equalSets(sf, sg, w)) {
        assignSub(temp, sf, sg, w);
        int v = firstBit(temp, w);
        if (v >= 0) {
            // v is in a set h of F but in no set of G: X = (columns \ h) + {v}
            Word const* h = f.row(f.findWith(v));
            assignOr(wit, sf, sg, w);
            assignSub(wit, wit, h, w);
            setBit(wit, v);
        } else {
            // v is in a set s of G but in no set of F: X = s \ {v}
            assignSub(temp, sg, sf, w);
            v = firstBit(temp, w);
            copySet(wit, g.row(g.findWith(v)), w);
            resetBit(wit, v);
        }
        return Base::Witness;
    }

    if (g.size() == 1 || f.size() == 1) {
        // One set s on one side (it is the common support): the other side must be its singletons.
        // Otherwise X = s \ {v} (s in G) or X = {v} (s in F) for a column v of s that is not a
        // singleton of the other side.
        bool sInG = g.size() == 1;
        Word const* s = sInG ? g.row(0) : f.row(0);
        auto const& other = sInG ? f : g;
        clearSet(temp, w);
        for (int i = 0; i < other.size(); ++i) {
            if (countSet(other.row(i), w) == 1)
                setBit(temp, firstBit(other.row(i), w));
        }
        assignSub(temp, s, temp, w);
        int v = firstBit(temp, w);
        if (v < 0)
            return Base::Dual;
        if (sInG) {
            copySet(wit, s, w);
            resetBit(wit, v);
        } else {
            setBit(wit, v);
        }
        return Base::Witness;
    }

    if (g.size() == 2 || f.size() == 2) {
        // Two sets on one side: compare their few minimal transversals with the other side. A
        // missing transversal t of F is a witness itself; for G, X = (columns) \ t.
        bool pairInF = f.size() == 2;
        auto const& pair = pairInF ? f : g;
        if (!missingTransversal(pair.row(0), pair.row(1), pairInF ? g : f))
            return Base::Dual;
        if (pairInF)
            copySet(wit, pairT_.data(), w);
        else
            assignSub(wit, sf, pairT_.data(), w);
        return Base::Witness;
    }

    // Duality needs sum 2^-|h| + sum 2^-|s| >= 1 (the expected number of sets h of F with h in
    // C \ X plus sets s of G with s in X, for a random X, is at least 1)
    double volume = 0;
    for (int i = 0; i < f.size(); ++i)
        volume += std::ldexp(1.0, -countSet(f.row(i), w));
    for (int i = 0; i < g.size(); ++i)
        volume += std::ldexp(1.0, -countSet(g.row(i), w));
    if (volume < 1 && volumeWitness(f, g))
        return Base::Witness;

    return Base::Split;
}

// Tr({a, b}) = {{v} : v in a & b} + {{u, w} : u in a \ b, w in b \ a}. Finds a set of Tr({a, b})
// that is not in the Sperner family `other` of transversals of {a, b} and puts it into pairT_;
// returns false if there is none (other = Tr({a, b})).
bool Solver::missingTransversal(Word const* a, Word const* b, Family const& other) {
    int w = w_;
    Word* pc = pairC_.data();
    Word* pa = pairA_.data();
    Word* pb = pairB_.data();
    Word* pt = pairT_.data();
    Word* temp = temp_.data();
    assignAnd(pc, a, b, w);
    assignSub(pa, a, b, w);
    assignSub(pb, b, a, w);
    clearSet(pt, w);
    clearSet(temp, w);  // singletons of other
    long long pairs = 0;
    for (int i = 0; i < other.size(); ++i) {
        Word const* r = other.row(i);
        if (intersects(r, pc, w)) {
            if (countSet(r, w) > 1) {
                // r is a proper superset of {v} for a common column v, so {v} is not in other
                assignAnd(temp, r, pc, w);
                setBit(pt, firstBit(temp, w));
                return true;
            }
            setBit(temp, firstBit(r, w));
        } else if (countSet(r, w) > 2) {
            // r is a proper superset of a pair {u, w}, which is not in other then
            assignAnd(temp, r, pa, w);
            setBit(pt, firstBit(temp, w));
            assignAnd(temp, r, pb, w);
            setBit(pt, firstBit(temp, w));
            return true;
        } else {
            ++pairs;
        }
    }

    assignSub(temp, pc, temp, w);
    int v = firstBit(temp, w);
    if (v >= 0) {
        setBit(pt, v);
        return true;
    }

    if (pairs == (long long) countSet(pa, w) * countSet(pb, w))
        return false;
    for (int u = 0; u < n_; ++u) {
        if (!getBit(pa, u))
            continue;
        clearSet(temp, w);  // columns paired with u in other
        for (int i = 0; i < other.size(); ++i) {
            if (getBit(other.row(i), u))
                assignOr(temp, temp, other.row(i), w);
        }
        assignSub(temp, pb, temp, w);
        int c = firstBit(temp, w);
        if (c >= 0) {
            setBit(pt, u);
            setBit(pt, c);
            return true;
        }
    }
    throw std::logic_error("FK: inconsistent transversals of two sets");
}

// Find X with f(C \ X) = g(X) = 0 by the method of conditional expectations: put the columns of
// the support into X or C \ X one by one, keeping the expected number of sets h of F in C \ X plus
// sets s of G in X below 1. Returns false if rounding errors spoiled the result.
bool Solver::volumeWitness(Family const& f, Family const& g) {
    int p = f.size();
    int q = g.size();
    restF_.resize(p);  // columns of h not placed yet, or -1 if h meets X
    restG_.resize(q);  // columns of s not placed yet, or -1 if s meets C \ X
    for (int i = 0; i < p; ++i)
        restF_[i] = countSet(f.row(i), w_);
    for (int i = 0; i < q; ++i)
        restG_[i] = countSet(g.row(i), w_);

    forEachBit(supportF_.data(), w_, [&](int v) {
        // Expectations of the sets with v if v goes into C \ X (sets of F) or into X (sets of G)
        double outside = 0;
        double inside = 0;
        for (int i = 0; i < p; ++i) {
            if (restF_[i] > 0 && getBit(f.row(i), v))
                outside += std::ldexp(1.0, 1 - restF_[i]);
        }
        for (int i = 0; i < q; ++i) {
            if (restG_[i] > 0 && getBit(g.row(i), v))
                inside += std::ldexp(1.0, 1 - restG_[i]);
        }
        bool inX = inside <= outside;
        if (inX)
            setBit(witness_.data(), v);
        for (int i = 0; i < p; ++i) {
            if (restF_[i] > 0 && getBit(f.row(i), v))
                restF_[i] = inX ? -1 : restF_[i] - 1;
        }
        for (int i = 0; i < q; ++i) {
            if (restG_[i] > 0 && getBit(g.row(i), v))
                restG_[i] = inX ? restG_[i] - 1 : -1;
        }
    });

    // A set with 0 columns left lies in C \ X (F) or in X (G)
    for (int r : restF_) {
        if (r == 0)
            return false;
    }
    for (int r : restG_) {
        if (r == 0)
            return false;
    }
    return true;
}

// Split column: the most frequent one in F or G; the kind of the split by algorithm B
void Solver::chooseColumn(Frame& fr) {
    auto const& f = *fr.f;
    auto const& g = *fr.g;
    countF_.assign(n_, 0);
    countG_.assign(n_, 0);
    for (int i = 0; i < f.size(); ++i)
        forEachBit(f.row(i), w_, [this](int j) { ++countF_[j]; });
    for (int i = 0; i < g.size(); ++i)
        forEachBit(g.row(i), w_, [this](int j) { ++countG_[j]; });

    double p = (double) f.size();
    double q = (double) g.size();
    double best = -1;
    for (int j = 0; j < n_; ++j) {
        double e = std::max(countF_[j] / p, countG_[j] / q);
        if (e > best) {
            best = e;
            fr.x = j;
        }
    }

    // Fredman and Khachiyan: some column has frequency at least 1 / chi(|F| |G|) in F or in G. If
    // x is that frequent in both, split into the two pairs of algorithm A; otherwise the side where
    // x is rare gives few subproblems of algorithm B.
    double eps = 1 / chi(p * q);
    if (countG_[fr.x] / q < eps)
        fr.kind = Kind::Alpha;
    else if (countF_[fr.x] / p < eps)
        fr.kind = Kind::Beta;
    else
        fr.kind = Kind::AB;
}

void Solver::split(Frame& fr) {
    chooseColumn(fr);
    int x = fr.x;
    fr.f0.clear();
    fr.f1.clear();
    fr.g0.clear();
    fr.g1.clear();
    for (int i = 0; i < fr.f->size(); ++i) {
        Word const* r = fr.f->row(i);
        if (getBit(r, x)) {
            fr.f1.add(r);
            resetBit(fr.f1.row(fr.f1.size() - 1), x);
        } else {
            fr.f0.add(r);
        }
    }
    for (int i = 0; i < fr.g->size(); ++i) {
        Word const* r = fr.g->row(i);
        if (getBit(r, x)) {
            fr.g1.add(r);
            resetBit(fr.g1.row(fr.g1.size() - 1), x);
        } else {
            fr.g0.add(r);
        }
    }
    fr.split = true;
    fr.phase = Phase::None;
    fr.step = 0;
    fr.index = 0;
    fr.synced = rootG_.size();
}

// Prepare the next child of a split frame at depth d; false if there is none left (all the
// children were dual, so the frame's pair is dual)
bool Solver::nextChild(Frame& fr, int d) {
    sync(fr, d);
    switch (fr.kind) {
        case Kind::AB:
            if (fr.step == 0) {
                fr.step = 1;
                pair1(fr);
                return true;
            }
            if (fr.step == 1) {
                fr.step = 2;
                pair2(fr);
                return true;
            }
            return false;

        case Kind::Alpha:
            if (fr.step == 0) {
                fr.step = 1;
                pair2(fr);
                return true;
            }
            if (fr.step == 1) {
                // In the generation, the covers found meanwhile may have made x frequent in G: then
                // the single first pair is cheaper than a subproblem for every set of G1
                double q = fr.g0.size() + fr.g1.size();
                if (fr.g1.size() / q >= 1 / chi(fr.f->size() * q)) {
                    fr.step = 3;
                    pair1(fr);
                    return true;
                }
                // New covers found in the subproblems do not contain x, so they add no set to G1,
                // but they may remove supersets from it: go over a copy and skip the removed sets
                fr.ts = fr.g1;
                fr.index = 0;
                fr.step = 2;
            }
            if (fr.step == 2) {
                while (fr.index < fr.ts.size()) {
                    Word const* t = fr.ts.row(fr.index++);
                    if (fr.g1.findEqual(t) >= 0) {
                        alpha(fr, t);
                        return true;
                    }
                }
            }
            return false;

        case Kind::Beta:
            if (fr.step == 0) {
                fr.step = 1;
                pair1(fr);
                return true;
            }
            if (fr.index < fr.f1.size()) {
                beta(fr, fr.index++);
                return true;
            }
            return false;
    }
    return false;
}

// (min(F0 + F1), G0)
void Solver::pair1(Frame& fr) {
    fr.phase = Phase::Pair1;
    // A set of F0 cannot be a proper subset of a set of F1 since F is Sperner
    unionMin(fr.childF, fr.f1, fr.f0);
    fr.childG = fr.g0;
}

// (F0, min(G0 + G1))
void Solver::pair2(Frame& fr) {
    fr.phase = Phase::Pair2;
    unionMin(fr.childG, fr.g1, fr.g0);
}

// (sets of F1 disjoint with t, min(G0 with t removed)) for a set t of G1
void Solver::alpha(Frame& fr, Word const* t) {
    fr.phase = Phase::Alpha;
    copySet(fr.t.data(), t, w_);
    disjointSets(fr.childF, fr.f1, fr.t.data());
    subtractMin(fr.childG, fr.g0, fr.t.data(), n_, work_);
}

// (min(F0 with t removed), sets of G1 disjoint with t) for the i-th set t of F1
void Solver::beta(Frame& fr, int i) {
    fr.phase = Phase::Beta;
    copySet(fr.t.data(), fr.f1.row(i), w_);
    subtractMin(fr.childF, fr.f0, fr.t.data(), n_, work_);
    disjointSets(fr.childG, fr.g1, fr.t.data());
}

// Turn a witness X' of the child of fr into a witness X of fr's pair
void Solver::lift(Frame const& fr) {
    Word* wit = witness_.data();
    switch (fr.phase) {
        case Phase::Pair1:  // X = X' without x
            resetBit(wit, fr.x);
            break;
        case Phase::Pair2:  // X = X' + {x}
            setBit(wit, fr.x);
            break;
        case Phase::Alpha:  // X = X' + t without x
            assignOr(wit, wit, fr.t.data(), w_);
            resetBit(wit, fr.x);
            break;
        case Phase::Beta:  // X = X' \ t + {x}
            assignSub(wit, wit, fr.t.data(), w_);
            setBit(wit, fr.x);
            break;
        case Phase::None:
            assert(false);
            break;
    }
}

// witness_ is a witness for the pair at the top of the stack. Returns Dual to test the pair again
// with the new cover.
Solver::Result Solver::found() {
    for (int k = depth_ - 1; k >= 0; --k)
        lift(*frames_[k]);

    if (!report_)
        return Result::NotDual;

    minimalTransversal();
    rootG_.add(image_.data());
    toColumns(image_.data(), w_, cover_);
    if (!(*report_)(cover_))
        return Result::Stopped;

    // The leaf pair is tested again with the image of the new cover in its G; the frames above get
    // it when they go on to their next child (sync)
    if (depth_ > 0) {
        imageAt(depth_, rootG_.row(rootG_.size() - 1), temp_.data());
        auto& g = frames_[depth_ - 1]->childG;
        if (g.findSubset(temp_.data()) >= 0)
            throw std::logic_error("FK: a new cover does not reach the subproblem it was found in");
        g.removeSupersets(temp_.data());
        g.add(temp_.data());
    }
    return once_ ? Result::Found : Result::Dual;
}

// Reduce the witness (a transversal of the root F that contains no set of G) to a minimal
// transversal in image_, removing columns greedily in increasing order
void Solver::minimalTransversal() {
    auto const& f = rootF_;
    int p = f.size();
    counts_.assign(p, 0);  // columns of X in each set of F
    for (int i = 0; i < p; ++i) {
        Word const* r = f.row(i);
        for (int k = 0; k < w_; ++k)
            counts_[i] += bits_count((bits64) (r[k] & witness_[k]));
        if (counts_[i] == 0)
            throw std::logic_error("FK: the witness is not a transversal");
    }

    copySet(image_.data(), witness_.data(), w_);
    forEachBit(witness_.data(), w_, [&](int j) {
        for (int i = 0; i < p; ++i) {
            if (counts_[i] == 1 && getBit(f.row(i), j))
                return;  // j is needed
        }
        resetBit(image_.data(), j);
        for (int i = 0; i < p; ++i) {
            if (getBit(f.row(i), j))
                --counts_[i];
        }
    });
    assert(rootG_.findSubset(image_.data()) < 0);
}

// Image of a cover in the input G of the frame at depth d: the transformations of the frames
// above, checking that the cover reaches each of their children
void Solver::imageAt(int d, Word const* cover, Word* image) {
    copySet(image, cover, w_);
    bool reaches = true;
    for (int k = 0; k < d && reaches; ++k) {
        auto const& fr = *frames_[k];
        bool hasX = getBit(image, fr.x);
        switch (fr.phase) {
            case Phase::Pair1:  // the child's G is G0
                reaches = !hasX;
                break;
            case Phase::Pair2:  // min(G0 + G1)
                resetBit(image, fr.x);
                break;
            case Phase::Alpha:  // G0 with t removed
                reaches = !hasX;
                assignSub(image, image, fr.t.data(), w_);
                break;
            case Phase::Beta:  // the sets of G1 disjoint with t
                reaches = hasX && !intersects(image, fr.t.data(), w_);
                resetBit(image, fr.x);
                break;
            case Phase::None:
                reaches = false;
                break;
        }
    }
    if (!reaches)
        throw std::logic_error("FK: a new cover does not reach the subproblem it was found in");
}

// Add the covers found since the frame at depth d was split (all in its subtree) to its split
// families. A new cover's image contains no set of the frame's G (it was a witness there), but it
// may be a proper subset of some of them, which are removed. That needs a frame above with the
// first or an alpha subproblem: the other subproblems take some of the sets of G1 or G0 and remove
// the same columns from all of them, so the images of covers stay pairwise incomparable as at the
// root, where G is a family of minimal transversals.
void Solver::sync(Frame& fr, int d) {
    int total = rootG_.size();
    if (fr.synced == total)
        return;
    bool shrinks = false;
    for (int k = 0; k < d && !shrinks; ++k)
        shrinks = frames_[k]->phase == Phase::Pair2 || frames_[k]->phase == Phase::Alpha;

    Word* image = temp_.data();
    for (int c = fr.synced; c < total; ++c) {
        imageAt(d, rootG_.row(c), image);
        if (getBit(image, fr.x)) {
            // A set of G0 cannot contain an image with x
            resetBit(image, fr.x);
            if (shrinks)
                fr.g1.removeSupersets(image);
            fr.g1.add(image);
        } else {
            if (shrinks) {
                fr.g0.removeSupersets(image);
                fr.g1.removeSupersets(image);
            }
            fr.g0.add(image);
        }
    }
    fr.synced = total;
}

DualityResult isDual(bit_matrix const& h, bit_matrix const& g) {
    int n = workWidth(std::max(h.width(), g.width()));
    int w = wordsFor(n);
    Family f = familyOf(h, w);
    minimize(f, n);
    Family s = familyOf(g, w);
    minimize(s, n);

    DualityResult result;
    for (int i = 0; i < s.size(); ++i) {
        for (int j = 0; j < f.size(); ++j) {
            if (!intersects(s.row(i), f.row(j), w)) {
                // X = a has g(X) = 1 and f(C \ X) = 1 (b lies in C \ X)
                result.result = Duality::NotTransversal;
                toColumns(s.row(i), w, result.witness);
                return result;
            }
        }
    }

    Solver solver;
    solver.init(n, std::move(f), std::move(s));
    if (solver.run(nullptr, false) == Solver::Result::Dual)
        return result;

    result.witness = solver.witness();
    Set x((std::size_t) w, Word(0));
    for (int j : result.witness)
        setBit(x.data(), j);
    auto const& rf = solver.rootF();
    bool transversal = true;
    for (int i = 0; i < rf.size() && transversal; ++i)
        transversal = intersects(rf.row(i), x.data(), w);
    bool containsG = solver.rootG().findSubset(x.data()) >= 0;
    if (transversal == containsG)
        throw std::logic_error("FK: invalid witness");
    result.result = transversal ? Duality::NewTransversal : Duality::NotTransversal;
    return result;
}

GlobalState::GlobalState() : solver_(std::make_unique<Solver>()) {}

GlobalState::~GlobalState() = default;

void GlobalState::dualize() {
    resetDualize();
    run(false);
}

bool GlobalState::next() {
    return run(true);
}

void GlobalState::resetDualize() {
    enumeration_ = Enumeration::NotStarted;
    covers_.trim_rows(0);
    lastCover_.clear();
    resetCurrentNode(_matrix.width());
}

bool GlobalState::isDualizeFinished() const {
    return enumeration_ == Enumeration::Finished;
}

bit_matrix const& GlobalState::getCovers() const {
    return covers_;
}

Columns const& GlobalState::getLastCover() const {
    return lastCover_;
}

void GlobalState::onMatrixChanged() {
    resetDualize();
}

void GlobalState::start() {
    resetDualize();
    enumeration_ = Enumeration::Finished;

    int n = _matrix.width();
    for (auto const& r : _matrix.get_rows()) {
        // A zero row cannot be covered
        if (n == 0 || !r.begin || r.none())
            return;
    }

    int w = workWidth(n);
    Family hypergraph = familyOf(_matrix, wordsFor(w));
    minimize(hypergraph, w);
    covers_.zero(0, w);
    solver_->init(w, std::move(hypergraph), Family(wordsFor(w)));
    enumeration_ = Enumeration::Running;
}

bool GlobalState::run(bool once) {
    if (enumeration_ == Enumeration::NotStarted)
        start();
    if (enumeration_ != Enumeration::Running)
        return false;

    int before = covers_.height();
    try {
        Solver::Report report = [this](Columns const& cover) { return reportCover(cover); };
        auto r = solver_->run(&report, once);
        if (r != Solver::Result::Found || _currentNode.stoped)
            enumeration_ = Enumeration::Finished;
    }
    catch (...) {
        resetDualize();
        throw;
    }
    return covers_.height() > before;
}

bool GlobalState::reportCover(Columns const& cover) {
    lastCover_ = cover;
    // add_rows reserves the row container one row ahead: grow it geometrically instead
    int h = covers_.height();
    if ((int) covers_.get_rows().capacity() == h)
        covers_.reserve(std::max(16, 2 * h), covers_.width());
    covers_.add_rows(1, covers_.width());
    auto& row = covers_.get_rows().back();
    for (int j : cover)
        row.set(j);
    _currentNode.columns = lastCover_;
    return doCoverCallback();
}

}  // namespace FK

}  // namespace dual
