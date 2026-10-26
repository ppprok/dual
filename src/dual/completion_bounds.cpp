#include <dual/completion_bounds.h>

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <limits>
#include <numeric>

#if defined(_MSC_VER) && !defined(__clang__)
#    include <intrin.h>
#endif

namespace dual {

namespace {

int wordsFor(int bits) {
    return (bits + 63) / 64;
}

int popCount(bits64 w) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_popcountll(w);
#else
    return bits_count(w);
#endif
}

// Index of the lowest set bit of a non-zero word
int lowestBit(bits64 w) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_ctzll(w);
#elif defined(_MSC_VER)
    unsigned long index = 0;
    _BitScanForward64(&index, w);
    return (int) index;
#else
    int i = 0;
    while (!(w & 1u)) {
        w >>= 1;
        ++i;
    }
    return i;
#endif
}

bool isSubset(std::vector<bits64> const& a, std::vector<bits64> const& b) {
    for (size_t k = 0; k < a.size(); ++k)
        if (a[k] & ~b[k])
            return false;
    return true;
}

int countBits(std::vector<bits64> const& a) {
    int c = 0;
    for (auto w : a)
        c += popCount(w);
    return c;
}

void setBit(bits64* mask, int i) {
    mask[i / 64] |= bits64(1) << (i % 64);
}

bool getBit(bits64 const* mask, int i) {
    return (mask[i / 64] >> (i % 64) & 1u) != 0;
}

}  // namespace

void CompletionBounds::SetMatrix(bit_matrix const& matrix) {
    Clear();
    columns_ = matrix.width();
    columnWords_ = wordsFor(columns_);

    // Rows as column masks
    std::vector<Words> all;
    all.reserve(matrix.height());
    for (auto const& r : matrix.get_rows()) {
        Words row(columnWords_, 0);
        if (r.begin)
            for (int j = r.find_first(); j >= 0 && j < columns_; j = r.find_next(j))
                setBit(row.data(), j);
        all.push_back(std::move(row));
    }

    // Keep the inclusion-minimal rows, one copy of each: a row that contains another one is never a
    // minimal uncovered row (the row inside it is not hit either). Sorted by size, which is also the
    // order of the greedy disjoint packing in MinGain.
    std::vector<int> bySize(all.size());
    std::iota(bySize.begin(), bySize.end(), 0);
    std::vector<int> sizes(all.size());
    for (size_t i = 0; i < all.size(); ++i)
        sizes[i] = countBits(all[i]);
    std::stable_sort(bySize.begin(), bySize.end(), [&sizes](int a, int b) { return sizes[a] < sizes[b]; });
    for (int i : bySize) {
        bool minimal = true;
        for (auto const& kept : matrixRows_)
            if (isSubset(kept, all[i])) {
                minimal = false;
                break;
            }
        if (minimal)
            matrixRows_.push_back(std::move(all[i]));
    }

    int m = (int) matrixRows_.size();
    rowWords_ = wordsFor(m);
    hasMatrix_ = true;

    columnRows_.assign((size_t) columns_ * rowWords_, 0);
    for (int i = 0; i < m; ++i)
        for (int j = 0; j < columns_; ++j)
            if (getBit(matrixRows_[i].data(), j))
                setBit(columnRows_.data() + (size_t) j * rowWords_, i);

    // The root hits no row
    uncovered_.assign(rowWords_, 0);
    for (int i = 0; i < m; ++i)
        setBit(uncovered_.data(), i);
    union_.assign(columnWords_, 0);
    packed_.assign(columnWords_, 0);
    depth_ = 0;
}

void CompletionBounds::SetGains(Weights const& positive,
                                Weights const& negative,
                                bool additive,
                                Weight magnitude,
                                int terms) {
    hasGains_ = hasMatrix_;
    additive_ = additive;
    positive_.assign(columns_, 0);
    negative_.assign(columns_, 0);
    for (int j = 0; j < columns_; ++j) {
        if (j < (int) positive.size() && positive[j] > 0)
            positive_[j] = positive[j];
        if (j < (int) negative.size() && negative[j] < 0)
            negative_[j] = negative[j];
    }

    // Rank the columns by decreasing positive gain: the rows are stored with the columns in that
    // order, so the largest gains of a union of rows are its lowest set bits
    std::vector<int> order(columns_);
    std::iota(order.begin(), order.end(), 0);
    std::stable_sort(order.begin(), order.end(), [this](int a, int b) { return positive_[a] > positive_[b]; });
    rank_.assign(columns_, 0);
    rankedPositive_.assign(columns_, 0);
    positiveColumns_ = 0;
    for (int p = 0; p < columns_; ++p) {
        rank_[order[p]] = p;
        rankedPositive_[p] = positive_[order[p]];
        positiveColumns_ += positive_[order[p]] > 0;
    }

    byNegative_.clear();
    for (int j = 0; j < columns_; ++j)
        if (negative_[j] < 0)
            byNegative_.push_back(j);
    std::stable_sort(byNegative_.begin(), byNegative_.end(), [this](int a, int b) {
        return negative_[a] < negative_[b];
    });

    // Sums of the k largest positive (smallest negative) gains, k = 0..n
    topPositive_.assign(1, 0);
    for (int p = 0; p < positiveColumns_; ++p)
        topPositive_.push_back(topPositive_.back() + rankedPositive_[p]);
    topNegative_.assign(1, 0);
    for (int j : byNegative_)
        topNegative_.push_back(topNegative_.back() + negative_[j]);

    int m = (int) matrixRows_.size();
    rows_.assign((size_t) m * columnWords_, 0);
    maxPositive_.assign(m, 0);
    minPositive_.assign(m, 0);
    maxNegative_.assign(m, 0);
    minNegative_.assign(m, 0);
    for (int i = 0; i < m; ++i) {
        bool first = true;
        for (int j = 0; j < columns_; ++j) {
            if (!getBit(matrixRows_[i].data(), j))
                continue;
            setBit(rows_.data() + (size_t) i * columnWords_, rank_[j]);
            if (first) {
                maxPositive_[i] = minPositive_[i] = positive_[j];
                maxNegative_[i] = minNegative_[i] = negative_[j];
                first = false;
                continue;
            }
            maxPositive_[i] = std::max(maxPositive_[i], positive_[j]);
            minPositive_[i] = std::min(minPositive_[i], positive_[j]);
            maxNegative_[i] = std::max(maxNegative_[i], negative_[j]);
            minNegative_[i] = std::min(minNegative_[i], negative_[j]);
        }
    }

    slackScale_ = 2.0 * (std::max(terms, 0) + 1) * FLT_EPSILON;
    magnitude_ = std::fabs((double) magnitude);
}

void CompletionBounds::Clear() {
    columns_ = 0;
    rowWords_ = 0;
    columnWords_ = 0;
    positiveColumns_ = 0;
    hasMatrix_ = false;
    hasGains_ = false;
    depth_ = 0;
    matrixRows_.clear();
    columnRows_.clear();
    rows_.clear();
    uncovered_.clear();
    positive_.clear();
    negative_.clear();
    rank_.clear();
    rankedPositive_.clear();
    byNegative_.clear();
}

bool CompletionBounds::Enabled() const {
    return hasMatrix_ && hasGains_;
}

bool CompletionBounds::HasMatrix() const {
    return hasMatrix_;
}

int CompletionBounds::Columns() const {
    return columns_;
}

void CompletionBounds::Update(DualizationNode const& node) {
    if (node.type != NodeType::NewNode || node.columns.empty() || rowWords_ == 0)
        return;

    int d = (int) node.columns.size();
    if (uncovered_.size() < (size_t) (d + 1) * rowWords_)
        uncovered_.resize((size_t) (d + 1) * rowWords_);
    depth_ = d;

    bits64 const* parent = uncovered(d - 1);
    bits64* child = uncovered_.data() + (size_t) d * rowWords_;
    int j = node.lastColumn;
    if (j < 0 || j >= columns_) {
        std::copy(parent, parent + rowWords_, child);
        return;
    }
    bits64 const* hit = columnRows_.data() + (size_t) j * rowWords_;
    for (int k = 0; k < rowWords_; ++k)
        child[k] = parent[k] & ~hit[k];
}

int CompletionBounds::CountUncovered() const {
    bits64 const* rows = uncovered(depth_);
    int u = 0;
    for (int k = 0; k < rowWords_; ++k)
        u += popCount(rows[k]);
    return u;
}

double CompletionBounds::MaxGain() {
    return maxGain(0, true);
}

double CompletionBounds::MinGain() {
    return minGain(0, true);
}

bool CompletionBounds::MaxGainBelow(double threshold) {
    return maxGain(threshold, false) < threshold;
}

bool CompletionBounds::MinGainAbove(double threshold) {
    return minGain(threshold, false) > threshold;
}

double CompletionBounds::maxGain(double threshold, bool exact) {
    if (rowWords_ == 0)
        return 0;
    int u = CountUncovered();
    if (u == 0)
        return 0;

    // Columns of C \ S lie in distinct uncovered minimal rows: at most u of them. Cheapest check
    // first: the u largest positive gains of all columns
    double top = topPositive_[std::min(u, positiveColumns_)];
    if (!exact && top < threshold)
        return top;

    // Each of them is at most the largest gain of its row; every uncovered row is hit by C \ S, so
    // the negative part is at most the largest negative gain of any of them (additive only)
    bits64 const* rows = uncovered(depth_);
    std::fill(union_.begin(), union_.end(), 0);
    double sumOfRowMax = 0;
    double negativePart = 0;
    bool first = true;
    for (int k = 0; k < rowWords_; ++k)
        for (bits64 w = rows[k]; w; w &= w - 1) {
            int i = k * 64 + lowestBit(w);
            sumOfRowMax += maxPositive_[i];
            if (first || maxNegative_[i] < negativePart)
                negativePart = maxNegative_[i];
            first = false;
            bits64 const* row = rows_.data() + (size_t) i * columnWords_;
            for (int c = 0; c < columnWords_; ++c)
                union_[c] |= row[c];
        }
    if (!additive_)
        negativePart = 0;
    double bound = std::min(top, sumOfRowMax) + negativePart;
    if (!exact && bound < threshold)
        return bound;

    // The u largest positive gains of the columns of the uncovered rows: the lowest set bits of
    // their union (columns are ranked by gain)
    // (unless exact, stop once the sum reaches the threshold: then so does the bound)
    double stopAt = exact ? std::numeric_limits<double>::infinity() : threshold;
    double sum = negativePart;
    int taken = 0;
    for (int c = 0; c < columnWords_ && taken < u && sum < stopAt; ++c)
        for (bits64 w = union_[c]; w && taken < u && sum < stopAt; w &= w - 1) {
            int p = c * 64 + lowestBit(w);
            if (p >= positiveColumns_) {
                taken = u;
                break;
            }
            sum += rankedPositive_[p];
            ++taken;
        }
    return std::min(bound, sum);
}

double CompletionBounds::minGain(double threshold, bool exact) {
    if (rowWords_ == 0)
        return 0;
    int u = CountUncovered();
    if (u == 0)
        return 0;

    bits64 const* rows = uncovered(depth_);
    double negativePart = 0;
    if (!byNegative_.empty()) {
        // At most u columns of the uncovered rows, each at least the smallest gain of its row
        std::fill(union_.begin(), union_.end(), 0);
        double sumOfRowMin = 0;
        for (int k = 0; k < rowWords_; ++k)
            for (bits64 w = rows[k]; w; w &= w - 1) {
                int i = k * 64 + lowestBit(w);
                sumOfRowMin += minNegative_[i];
                bits64 const* row = rows_.data() + (size_t) i * columnWords_;
                for (int c = 0; c < columnWords_; ++c)
                    union_[c] |= row[c];
            }
        double bottom = 0;
        int taken = 0;
        for (int j : byNegative_) {
            if (taken == u)
                break;
            if (getBit(union_.data(), rank_[j])) {
                bottom += negative_[j];
                ++taken;
            }
        }
        negativePart = std::max(bottom, sumOfRowMin);
    }

    // Pairwise disjoint uncovered rows are hit by different columns of C \ S, each at least the
    // smallest positive gain of its row (additive only: then the positive gains are part of
    // f(C) - f(S)). Rows are packed greedily from the smallest one; every prefix is a valid bound.
    double positivePart = 0;
    if (additive_ && positiveColumns_ > 0) {
        std::fill(packed_.begin(), packed_.end(), 0);
        double largest = 0;
        for (int k = 0; k < rowWords_; ++k)
            for (bits64 w = rows[k]; w; w &= w - 1) {
                int i = k * 64 + lowestBit(w);
                largest = std::max(largest, minPositive_[i]);
                bits64 const* row = rows_.data() + (size_t) i * columnWords_;
                bool disjoint = true;
                for (int c = 0; c < columnWords_ && disjoint; ++c)
                    disjoint = (row[c] & packed_[c]) == 0;
                if (!disjoint)
                    continue;
                positivePart += minPositive_[i];
                if (!exact && negativePart + positivePart > threshold)
                    return negativePart + positivePart;
                for (int c = 0; c < columnWords_; ++c)
                    packed_[c] |= row[c];
            }
        positivePart = std::max(positivePart, largest);
    }

    return negativePart + positivePart;
}

double CompletionBounds::Slack(Weight weight) const {
    return slackScale_ * (std::fabs((double) weight) + magnitude_);
}

}  // namespace dual
