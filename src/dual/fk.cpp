#include <dual/fk.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <functional>
#include <stdexcept>
#include <utility>
#include <vector>

#include <dual/bit_vector.h>

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

namespace dual {

namespace FK {

namespace {

// Rows of the work matrices need memory even when there are no columns
int workWidth(int n) {
    return std::max(n, 1);
}

// Append the rows of src to dst bit by bit, so rows with any layout fit into dst's row slots
void copyRows(bit_matrix const& src, bit_matrix& dst, int n) {
    for (auto const& r : src.get_rows()) {
        dst.add_rows(1, n);
        if (!r.begin)
            continue;
        auto& row = dst.get_rows().back();
        for (int j = r.find_first(); j >= 0; j = r.find_next(j))
            row.set(j);
    }
}

// Keep only the minimal rows (drop duplicates and supersets of other rows), in their order. Only
// the rows listed in dominators are compared with the others: the caller knows that no other row
// is a subset of another row.
void removeDominated(bit_matrix& m,
                     std::vector<int> const& dominators,
                     std::vector<int>& counts,
                     std::vector<char>& keep) {
    auto& rows = m.get_rows();
    int s = (int) rows.size();
    counts.resize(s);
    keep.assign(s, 1);
    for (int i = 0; i < s; ++i)
        counts[i] = rows[i].count();

    for (int i = 0; i < s; ++i) {
        for (int j : dominators) {
            if (j == i || counts[j] > counts[i])
                continue;
            bool dominated = counts[j] < counts[i] ? rows[j].is_subset_of(rows[i]) : j < i && rows[j] == rows[i];
            if (dominated) {
                keep[i] = 0;
                break;
            }
        }
    }

    int k = 0;
    for (int i = 0; i < s; ++i) {
        if (!keep[i])
            continue;
        if (k != i)
            rows[k].assign(rows[i]);
        ++k;
    }
    m.trim_rows(k);
}

// Keep only the minimal rows of any family
void minimize(bit_matrix& m, std::vector<int>& dominators, std::vector<int>& counts, std::vector<char>& keep) {
    dominators.resize(m.height());
    for (int i = 0; i < m.height(); ++i)
        dominators[i] = i;
    removeDominated(m, dominators, counts, keep);
}

// dst = min(sets of a with the columns of t removed) for a Sperner family a. A set that misses t
// stays as it is and is a subset of no other set of the result (it would be a subset of another
// set of a), so only the sets that meet t can make other sets non-minimal.
void subtractMin(bit_matrix& dst,
                 bit_matrix const& a,
                 bit_chunk const& t,
                 int n,
                 std::vector<int>& dominators,
                 std::vector<int>& counts,
                 std::vector<char>& keep) {
    dst.trim_rows(0);
    dominators.clear();
    for (auto const& r : a.get_rows()) {
        if (r.intersects(t))
            dominators.push_back(dst.height());
        dst.add_rows(1, n);
        dst.get_rows().back().assign_sub(r, t);
    }
    removeDominated(dst, dominators, counts, keep);
}

// dst = min(a + b) for Sperner families a and b where no set of b is a proper subset of a set of a
void unionMin(bit_matrix& dst, bit_matrix const& a, bit_matrix const& b, int n) {
    dst.trim_rows(0);
    for (auto const& r : a.get_rows())
        dst.add_row(r, n);
    for (auto const& r : b.get_rows()) {
        if (a.find_sub_row(r) < 0)
            dst.add_row(r, n);
    }
}

// Add s to a Sperner family unless a set of it is a subset of s, removing the supersets of s
bool insertMin(bit_matrix& m, bit_chunk const& s, int n) {
    if (m.find_sub_row(s) >= 0)
        return false;
    m.remove_sup_rows(s);
    m.add_row(s, n);
    return true;
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

// The duality test and the generation of covers. One recursion frame per depth is kept between
// runs, so the matrices of the subproblems are allocated once.
class Solver {
public:
    // Called for every new cover; returns false to stop the generation
    typedef std::function<bool(bit_chunk)> Report;

    // Prepare for families over n columns (n >= 1)
    void init(int n);

    // Duality test of Sperner families f and g whose sets pairwise intersect.
    // Returns true if they are dual; otherwise witness() is a witness.
    bool test(bit_matrix const& f, bit_matrix const& g);

    // Generate the minimal transversals of the Sperner family f that are not in g (g is a subset of
    // Tr(f)), adding each to g and passing it to report. With once, stops after the first one.
    // Returns true if g = Tr(f) at the end, false if stopped.
    bool generate(bit_matrix const& f, bit_matrix& g, bool once, Report const& report);

    // Witness of the last test that returned false
    bit_chunk witness() const {
        return witness_;
    }

private:
    enum class Result {
        Dual,     // the pair is dual
        NotDual,  // the pair is not dual, witness_ holds a witness (test)
        Stopped,  // the generation is stopped
        Retry     // a new cover was added to the families on the path: test this pair again (generate)
    };

    enum class Base {
        Dual,     // the pair is dual
        Witness,  // witness_ is a witness for the pair
        Split     // no answer without splitting
    };

    enum class Phase {
        None,  // the frame is a leaf
        Pair1,
        Pair2,
        Alpha,
        Beta
    };

    struct Frame {
        bit_matrix const* f = nullptr;  // input families
        bit_matrix const* g = nullptr;
        int x = -1;  // split column
        Phase phase = Phase::None;
        bit_matrix f0, f1, g0, g1;  // input families split on x
        bit_matrix childF, childG;  // input families of the child, unless they are f0 or g0
        bit_matrix ts;              // sets t of the alpha subproblems
        bit_vector t;               // set t of the alpha or beta subproblem

        explicit Frame(int n) : t(n) {}
    };

    Frame& frame(int d);
    Result solve(int d);
    Base baseCase(Frame const& fr);
    bool missingTransversal(bit_chunk const& a, bit_chunk const& b, bit_matrix::bit_rows const& other);
    bool volumeWitness(bit_matrix::bit_rows const& f, bit_matrix::bit_rows const& g);
    void chooseColumn(Frame& fr);
    Result split(int d);
    Result pair1(int d);
    Result pair2(int d);
    Result alpha(int d, bit_chunk const& t);
    Result beta(int d, int i);
    Result child(int d, bit_matrix const* f, bit_matrix const* g);
    Result found(int d);
    void lift(Frame const& fr);
    void minimalTransversal();
    bool propagate(Frame& fr);

    int n_ = 0;
    std::vector<std::unique_ptr<Frame>> frames_;
    bit_vector witness_, supportF_, supportG_, temp_, image_, childImage_, pairA_, pairB_, pairC_, pairT_;
    std::vector<int> countF_, countG_, counts_, dominators_;
    std::vector<char> keep_;
    std::vector<int> restF_, restG_;
    bool splitAlpha_ = false, splitBeta_ = false;

    // Generation mode
    bit_matrix* covers_ = nullptr;
    Report const* report_ = nullptr;
    bool once_ = false;
};

void Solver::init(int n) {
    assert(n >= 1);
    if (n != n_) {
        n_ = n;
        frames_.clear();
    }
    witness_.zero(n);
    supportF_.zero(n);
    supportG_.zero(n);
    temp_.zero(n);
    image_.zero(n);
    childImage_.zero(n);
    pairA_.zero(n);
    pairB_.zero(n);
    pairC_.zero(n);
    pairT_.zero(n);
}

Solver::Frame& Solver::frame(int d) {
    while ((int) frames_.size() <= d)
        frames_.push_back(std::make_unique<Frame>(n_));
    return *frames_[d];
}

bool Solver::test(bit_matrix const& f, bit_matrix const& g) {
    covers_ = nullptr;
    report_ = nullptr;
    auto& root = frame(0);
    root.f = &f;
    root.g = &g;
    return solve(0) == Result::Dual;
}

bool Solver::generate(bit_matrix const& f, bit_matrix& g, bool once, Report const& report) {
    covers_ = &g;
    report_ = &report;
    once_ = once;
    auto& root = frame(0);
    root.f = &f;
    root.g = &g;
    auto r = solve(0);
    covers_ = nullptr;
    report_ = nullptr;
    return r == Result::Dual;
}

Solver::Result Solver::solve(int d) {
    for (;;) {
        auto& fr = frame(d);
        fr.phase = Phase::None;
        switch (baseCase(fr)) {
            case Base::Dual:
                return Result::Dual;
            case Base::Witness: {
                auto r = found(d);
                if (r != Result::Retry)
                    return r;
                continue;
            }
            case Base::Split:
                break;
        }
        return split(d);
    }
}

// Answer without splitting where the families are small or obviously not dual. Relies on F and G
// being Sperner with pairwise intersecting sets.
Solver::Base Solver::baseCase(Frame const& fr) {
    auto const& f = fr.f->get_rows();
    auto const& g = fr.g->get_rows();
    witness_.clear();

    if (f.empty()) {
        // f = 0 is dual only to g = 1, i.e. G = {{}}; otherwise X = {} has f(C) = g({}) = 0
        if (g.size() == 1 && g[0].none())
            return Base::Dual;
        return Base::Witness;
    }

    if (g.empty()) {
        // g = 0 is dual only to F = {{}}; otherwise X = the columns of F has f({}) = g(X) = 0
        if (f.size() == 1 && f[0].none())
            return Base::Dual;
        for (auto const& r : f)
            witness_.assign_or(witness_, r);
        return Base::Witness;
    }

    if (f.size() == 1 && f[0].none()) {
        // f = 1 and G is not empty: X = a set of G has f(C \ X) = g(X) = 1
        witness_.assign(g[0]);
        return Base::Witness;
    }

    if (g.size() == 1 && g[0].none()) {
        // g = 1 and F is not empty: X = {} has f(C) = g({}) = 1
        return Base::Witness;
    }

    supportF_.clear();
    for (auto const& r : f)
        supportF_.assign_or(supportF_, r);
    supportG_.clear();
    for (auto const& r : g)
        supportG_.assign_or(supportG_, r);

    if (!(supportF_ == supportG_)) {
        temp_.assign_sub(supportF_, supportG_);
        int v = temp_.find_first();
        if (v >= 0) {
            // v is in a set h of F but in no set of G: X = (columns \ h) + {v}
            auto const& h = f[fr.f->find_row([v](bit_chunk const& r) { return r.get(v); })];
            witness_.assign_or(supportF_, supportG_);
            witness_.assign_sub(witness_, h);
            witness_.set(v);
        } else {
            // v is in a set s of G but in no set of F: X = s \ {v}
            temp_.assign_sub(supportG_, supportF_);
            v = temp_.find_first();
            auto const& s = g[fr.g->find_row([v](bit_chunk const& r) { return r.get(v); })];
            witness_.assign(s);
            witness_.reset(v);
        }
        return Base::Witness;
    }

    if (g.size() == 1 || f.size() == 1) {
        // One set s on one side (it is the common support): the other side must be its singletons.
        // Otherwise X = s \ {v} (s in G) or X = {v} (s in F) for a column v of s that is not a
        // singleton of the other side.
        bool sInG = g.size() == 1;
        auto const& s = sInG ? g[0] : f[0];
        auto const& other = sInG ? f : g;
        temp_.clear();
        for (auto const& r : other) {
            if (r.count(1) == 1)
                temp_.set(r.find_first());
        }
        temp_.assign_sub(s, temp_);
        int v = temp_.find_first();
        if (v < 0)
            return Base::Dual;
        if (sInG) {
            witness_.assign(s);
            witness_.reset(v);
        } else {
            witness_.set(v);
        }
        return Base::Witness;
    }

    if (g.size() == 2 || f.size() == 2) {
        // Two sets on one side: compare their few minimal transversals with the other side. A
        // missing transversal t of F is a witness itself; for G, X = (columns) \ t.
        bool pairInF = f.size() == 2;
        auto const& pair = pairInF ? f : g;
        if (!missingTransversal(pair[0], pair[1], pairInF ? g : f))
            return Base::Dual;
        if (pairInF)
            witness_.assign(pairT_);
        else
            witness_.assign_sub(supportF_, pairT_);
        return Base::Witness;
    }

    // Duality needs sum 2^-|h| + sum 2^-|s| >= 1 (the expected number of sets h of F with h in
    // C \ X plus sets s of G with s in X, for a random X, is at least 1)
    double volume = 0;
    for (auto const& r : f)
        volume += std::ldexp(1.0, -r.count());
    for (auto const& r : g)
        volume += std::ldexp(1.0, -r.count());
    if (volume < 1 && volumeWitness(f, g))
        return Base::Witness;

    return Base::Split;
}

// Tr({a, b}) = {{v} : v in a & b} + {{u, w} : u in a \ b, w in b \ a}. Finds a set of Tr({a, b})
// that is not in the Sperner family `other` of transversals of {a, b} and puts it into pairT_;
// returns false if there is none (other = Tr({a, b})).
bool Solver::missingTransversal(bit_chunk const& a, bit_chunk const& b, bit_matrix::bit_rows const& other) {
    pairC_.assign_and(a, b);
    pairA_.assign_sub(a, b);
    pairB_.assign_sub(b, a);
    pairT_.clear();
    temp_.clear();  // singletons of other
    long long pairs = 0;
    for (auto const& r : other) {
        if (r.intersects(pairC_)) {
            if (r.count(1) > 1) {
                // r is a proper superset of {v} for a common column v, so {v} is not in other
                temp_.assign_and(r, pairC_);
                pairT_.set(temp_.find_first());
                return true;
            }
            temp_.set(r.find_first());
        } else if (r.count(2) > 2) {
            // r is a proper superset of a pair {u, w}, which is not in other then
            temp_.assign_and(r, pairA_);
            pairT_.set(temp_.find_first());
            temp_.assign_and(r, pairB_);
            pairT_.set(temp_.find_first());
            return true;
        } else {
            ++pairs;
        }
    }

    temp_.assign_sub(pairC_, temp_);
    int v = temp_.find_first();
    if (v >= 0) {
        pairT_.set(v);
        return true;
    }

    if (pairs == (long long) pairA_.count() * pairB_.count())
        return false;
    for (int u = pairA_.find_first(); u >= 0; u = pairA_.find_next(u)) {
        temp_.clear();  // columns paired with u in other
        for (auto const& r : other) {
            if (r.get(u))
                temp_.assign_or(temp_, r);
        }
        temp_.assign_sub(pairB_, temp_);
        int w = temp_.find_first();
        if (w >= 0) {
            pairT_.set(u);
            pairT_.set(w);
            return true;
        }
    }
    throw std::logic_error("FK: inconsistent transversals of two sets");
}

// Find X with f(C \ X) = g(X) = 0 by the method of conditional expectations: put the columns of
// the support into X or C \ X one by one, keeping the expected number of sets h of F in C \ X plus
// sets s of G in X below 1. Returns false if rounding errors spoiled the result.
bool Solver::volumeWitness(bit_matrix::bit_rows const& f, bit_matrix::bit_rows const& g) {
    int p = (int) f.size();
    int q = (int) g.size();
    restF_.resize(p);  // columns of h not placed yet, or -1 if h meets X
    restG_.resize(q);  // columns of s not placed yet, or -1 if s meets C \ X
    for (int i = 0; i < p; ++i)
        restF_[i] = f[i].count();
    for (int i = 0; i < q; ++i)
        restG_[i] = g[i].count();

    for (int v = supportF_.find_first(); v >= 0; v = supportF_.find_next(v)) {
        bit_chunk::get_bit_iterator it(v);
        // Expectations of the sets with v if v goes into C \ X (sets of F) or into X (sets of G)
        double outside = 0;
        double inside = 0;
        for (int i = 0; i < p; ++i) {
            if (restF_[i] > 0 && f[i].get(it))
                outside += std::ldexp(1.0, 1 - restF_[i]);
        }
        for (int i = 0; i < q; ++i) {
            if (restG_[i] > 0 && g[i].get(it))
                inside += std::ldexp(1.0, 1 - restG_[i]);
        }
        bool inX = inside <= outside;
        if (inX)
            witness_.set(v);
        for (int i = 0; i < p; ++i) {
            if (restF_[i] > 0 && f[i].get(it))
                restF_[i] = inX ? -1 : restF_[i] - 1;
        }
        for (int i = 0; i < q; ++i) {
            if (restG_[i] > 0 && g[i].get(it))
                restG_[i] = inX ? restG_[i] - 1 : -1;
        }
    }

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
    auto const& f = fr.f->get_rows();
    auto const& g = fr.g->get_rows();
    countF_.assign(n_, 0);
    countG_.assign(n_, 0);
    for (auto const& r : f) {
        for (int j = r.find_first(); j >= 0; j = r.find_next(j))
            ++countF_[j];
    }
    for (auto const& r : g) {
        for (int j = r.find_first(); j >= 0; j = r.find_next(j))
            ++countG_[j];
    }

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
    splitAlpha_ = countG_[fr.x] / q < eps;
    splitBeta_ = !splitAlpha_ && countF_[fr.x] / p < eps;
}

Solver::Result Solver::split(int d) {
    auto& fr = *frames_[d];
    chooseColumn(fr);
    bool alphaSplit = splitAlpha_;
    bool betaSplit = splitBeta_;

    int x = fr.x;
    fr.f0.trim_rows(0);
    fr.f1.trim_rows(0);
    fr.g0.trim_rows(0);
    fr.g1.trim_rows(0);
    for (auto const& r : fr.f->get_rows()) {
        if (r.get(x))
            fr.f1.add_row(r, n_).reset(x);
        else
            fr.f0.add_row(r, n_);
    }
    for (auto const& r : fr.g->get_rows()) {
        if (r.get(x))
            fr.g1.add_row(r, n_).reset(x);
        else
            fr.g0.add_row(r, n_);
    }

    Result r;
    if (alphaSplit) {
        if ((r = pair2(d)) != Result::Dual)
            return r;
        // In the generation, the covers found meanwhile may have made x frequent in G: then the
        // single first pair is cheaper than a subproblem for every set of G1
        double q = fr.g0.height() + fr.g1.height();
        if (fr.g1.height() / q >= 1 / chi(fr.f->height() * q))
            return pair1(d);
        // New covers found in these subproblems do not contain x, so they add no set to G1, but
        // they may remove supersets from it: go over a copy and skip the removed sets
        fr.ts.trim_rows(0);
        for (auto const& t : fr.g1.get_rows())
            fr.ts.add_row(t, n_);
        for (int i = 0; i < fr.ts.height(); ++i) {
            if (fr.g1.find_equal_row(fr.ts.get_row(i)) < 0)
                continue;
            if ((r = alpha(d, fr.ts.get_row(i))) != Result::Dual)
                return r;
        }
        return Result::Dual;
    }

    if (betaSplit) {
        if ((r = pair1(d)) != Result::Dual)
            return r;
        for (int i = 0; i < fr.f1.height(); ++i) {
            if ((r = beta(d, i)) != Result::Dual)
                return r;
        }
        return Result::Dual;
    }

    if ((r = pair1(d)) != Result::Dual)
        return r;
    return pair2(d);
}

// (min(F0 + F1), G0)
Solver::Result Solver::pair1(int d) {
    auto& fr = *frames_[d];
    fr.phase = Phase::Pair1;
    // A set of F0 cannot be a proper subset of a set of F1 since F is Sperner
    unionMin(fr.childF, fr.f1, fr.f0, n_);
    return child(d, &fr.childF, &fr.g0);
}

// (F0, min(G0 + G1))
Solver::Result Solver::pair2(int d) {
    auto& fr = *frames_[d];
    fr.phase = Phase::Pair2;
    unionMin(fr.childG, fr.g1, fr.g0, n_);
    return child(d, &fr.f0, &fr.childG);
}

// (sets of F1 disjoint with t, min(G0 with t removed)) for a set t of G1
Solver::Result Solver::alpha(int d, bit_chunk const& t) {
    auto& fr = *frames_[d];
    fr.phase = Phase::Alpha;
    fr.t.assign(t);
    fr.childF.trim_rows(0);
    for (auto const& r : fr.f1.get_rows()) {
        if (!r.intersects(fr.t))
            fr.childF.add_row(r, n_);
    }
    subtractMin(fr.childG, fr.g0, fr.t, n_, dominators_, counts_, keep_);
    return child(d, &fr.childF, &fr.childG);
}

// (min(F0 with t removed), sets of G1 disjoint with t) for the i-th set t of F1
Solver::Result Solver::beta(int d, int i) {
    auto& fr = *frames_[d];
    fr.phase = Phase::Beta;
    fr.t.assign(fr.f1.get_row(i));
    subtractMin(fr.childF, fr.f0, fr.t, n_, dominators_, counts_, keep_);
    fr.childG.trim_rows(0);
    for (auto const& r : fr.g1.get_rows()) {
        if (!r.intersects(fr.t))
            fr.childG.add_row(r, n_);
    }
    return child(d, &fr.childF, &fr.childG);
}

Solver::Result Solver::child(int d, bit_matrix const* f, bit_matrix const* g) {
    auto& c = frame(d + 1);
    c.f = f;
    c.g = g;
    return solve(d + 1);
}

// Turn a witness X' of the child of fr into a witness X of fr's pair
void Solver::lift(Frame const& fr) {
    switch (fr.phase) {
        case Phase::Pair1:  // X = X' without x
            witness_.reset(fr.x);
            break;
        case Phase::Pair2:  // X = X' + {x}
            witness_.set(fr.x);
            break;
        case Phase::Alpha:  // X = X' + t without x
            witness_.assign_or(witness_, fr.t);
            witness_.reset(fr.x);
            break;
        case Phase::Beta:  // X = X' \ t + {x}
            witness_.assign_sub(witness_, fr.t);
            witness_.set(fr.x);
            break;
        case Phase::None:
            assert(false);
            break;
    }
}

// witness_ is a witness for the pair at depth d
Solver::Result Solver::found(int d) {
    for (int k = d - 1; k >= 0; --k)
        lift(*frames_[k]);

    if (!covers_)
        return Result::NotDual;

    minimalTransversal();
    covers_->add_row(image_, n_);
    if (!(*report_)(image_) || once_)
        return Result::Stopped;

    // Add the cover to the families of every frame on the path, so that the leaf pair is tested
    // again with it and the frames above go on with it
    for (int k = 0; k < d; ++k) {
        if (!propagate(*frames_[k]))
            throw std::logic_error("FK: a new cover does not reach the subproblem it was found in");
    }
    return Result::Retry;
}

// Reduce the witness (a transversal of the root F that contains no set of G) to a minimal
// transversal in image_, removing columns greedily in increasing order
void Solver::minimalTransversal() {
    auto const& f = frames_[0]->f->get_rows();
    int p = (int) f.size();
    counts_.assign(p, 0);  // columns of X in each set of F
    for (int j = witness_.find_first(); j >= 0; j = witness_.find_next(j)) {
        bit_chunk::get_bit_iterator it(j);
        for (int i = 0; i < p; ++i) {
            if (f[i].get(it))
                ++counts_[i];
        }
    }
    for (int c : counts_) {
        if (c == 0)
            throw std::logic_error("FK: the witness is not a transversal");
    }

    image_.assign(witness_);
    for (int j = witness_.find_first(); j >= 0; j = witness_.find_next(j)) {
        bit_chunk::get_bit_iterator it(j);
        bool needed = false;
        for (int i = 0; i < p && !needed; ++i)
            needed = counts_[i] == 1 && f[i].get(it);
        if (needed)
            continue;
        image_.reset(j);
        for (int i = 0; i < p; ++i) {
            if (f[i].get(it))
                --counts_[i];
        }
    }
    assert(covers_->find_sub_row(image_) < 0);
}

// The image of a new cover is in image_ and already in fr's input G: add it to fr's split families
// and put its image for the child into image_ (adding it to the child's input G). Returns false if
// the child's input G does not get it.
bool Solver::propagate(Frame& fr) {
    // The parent removed the supersets of the image from fr's input G; remove them from both split
    // families (a set of G0 cannot contain an image with x)
    bool hasX = image_.get(fr.x);
    bool inserted;
    if (hasX) {
        childImage_.assign(image_);
        childImage_.reset(fr.x);
        inserted = insertMin(fr.g1, childImage_, n_);
    } else {
        inserted = insertMin(fr.g0, image_, n_);
        fr.g1.remove_sup_rows(image_);
    }

    switch (fr.phase) {
        case Phase::Pair1:  // the child's G is g0
            return !hasX && inserted;
        case Phase::Pair2:
            if (!hasX)
                childImage_.assign(image_);
            break;
        case Phase::Alpha:
            if (hasX)
                return false;
            childImage_.assign_sub(image_, fr.t);
            break;
        case Phase::Beta:
            if (!hasX || image_.intersects(fr.t))
                return false;
            break;
        case Phase::None:
            return false;
    }
    if (!insertMin(fr.childG, childImage_, n_))
        return false;
    std::swap(image_, childImage_);
    return true;
}

DualityResult isDual(bit_matrix const& h, bit_matrix const& g) {
    int n = workWidth(std::max(h.width(), g.width()));
    std::vector<int> dominators, counts;
    std::vector<char> keep;
    bit_matrix f;
    copyRows(h, f, n);
    minimize(f, dominators, counts, keep);
    bit_matrix s;
    copyRows(g, s, n);
    minimize(s, dominators, counts, keep);

    DualityResult result;
    for (auto const& a : s.get_rows()) {
        for (auto const& b : f.get_rows()) {
            if (!a.intersects(b)) {
                // X = a has g(X) = 1 and f(C \ X) = 1 (b lies in C \ X)
                result.result = Duality::NotTransversal;
                a.get_bits(result.witness);
                return result;
            }
        }
    }

    Solver solver;
    solver.init(n);
    if (solver.test(f, s))
        return result;

    auto x = solver.witness();
    x.get_bits(result.witness);
    bool transversal =
            std::all_of(f.get_rows().begin(), f.get_rows().end(), [&x](bit_chunk const& r) { return r.intersects(x); });
    bool containsG = s.find_sub_row(x) >= 0;
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
    hypergraph_.zero(0, w);
    copyRows(_matrix, hypergraph_, w);
    std::vector<int> dominators, counts;
    std::vector<char> keep;
    minimize(hypergraph_, dominators, counts, keep);
    covers_.zero(0, w);
    solver_->init(w);
    enumeration_ = Enumeration::Running;
}

bool GlobalState::run(bool once) {
    if (enumeration_ == Enumeration::NotStarted)
        start();
    if (enumeration_ != Enumeration::Running)
        return false;

    int before = covers_.height();
    try {
        bool complete =
                solver_->generate(hypergraph_, covers_, once, [this](bit_chunk cover) { return reportCover(cover); });
        if (complete || _currentNode.stoped)
            enumeration_ = Enumeration::Finished;
    }
    catch (...) {
        resetDualize();
        throw;
    }
    return covers_.height() > before;
}

bool GlobalState::reportCover(bit_chunk cover) {
    lastCover_.clear();
    cover.get_bits(lastCover_);
    _currentNode.columns = lastCover_;
    return doCoverCallback();
}

}  // namespace FK

}  // namespace dual
