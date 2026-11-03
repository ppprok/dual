/// @file
/// @brief Fixed-width local state of the RUNC family: rows of at most 64 or 128 columns kept in
///        machine words (internal to `runc.cpp`, see `Runc::RuncGlobalState`).

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include <dual/bit_chunk.h>
#include <dual/runc.h>

#if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_ARM64))
#    include <intrin.h>
#endif

namespace dual {

namespace Runc {

/// @brief Set of at most 128 columns in two 64-bit words (bit `j` of `lo` is column `j`, bit `j` of
///        `hi` is column `64 + j`).
struct Bits128 {
    /// Columns 0-63.
    uint64_t lo = 0;
    /// Columns 64-127.
    uint64_t hi = 0;
};

inline Bits128 operator&(Bits128 a, Bits128 b) {
    return {a.lo & b.lo, a.hi & b.hi};
}
inline Bits128 operator|(Bits128 a, Bits128 b) {
    return {a.lo | b.lo, a.hi | b.hi};
}
inline Bits128 operator~(Bits128 a) {
    return {~a.lo, ~a.hi};
}
inline bool operator==(Bits128 a, Bits128 b) {
    return a.lo == b.lo && a.hi == b.hi;
}

/// @name Word operations of the fixed-width state, for `uint64_t` and `Bits128`
/// @{

/// @brief Number of set bits.
inline int wordCount(uint64_t x) {
#if defined(__GNUC__) && defined(__POPCNT__)
    return __builtin_popcountll(x);
#else
    // SWAR popcount: without the POPCNT instruction the builtin is a library call
    x = x - ((x >> 1) & 0x5555555555555555ULL);
    x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL);
    x = (x + (x >> 4)) & 0x0f0f0f0f0f0f0f0fULL;
    return int((x * 0x0101010101010101ULL) >> 56);
#endif
}
/// @brief Number of set bits.
inline int wordCount(Bits128 x) {
    return wordCount(x.lo) + wordCount(x.hi);
}

/// @brief Index of the highest set bit of a non-zero word.
inline int wordLast(uint64_t x) {
#if defined(__GNUC__)
    return 63 - __builtin_clzll(x);
#elif defined(_MSC_VER) && (defined(_M_X64) || defined(_M_ARM64))
    unsigned long i = 0;
    _BitScanReverse64(&i, x);
    return int(i);
#else
    int i = 0;
    while (x >>= 1)
        ++i;
    return i;
#endif
}
/// @brief Index of the highest set bit of a non-zero word.
inline int wordLast(Bits128 x) {
    return x.hi ? 64 + wordLast(x.hi) : wordLast(x.lo);
}

/// @brief Whether some bit is set.
inline bool wordAny(uint64_t x) {
    return x != 0;
}
/// @brief Whether some bit is set.
inline bool wordAny(Bits128 x) {
    return (x.lo | x.hi) != 0;
}

/// @brief Word with only bit @p j set.
template<typename Word>
Word wordBit(int j);
template<>
inline uint64_t wordBit<uint64_t>(int j) {
    return uint64_t(1) << j;
}
template<>
inline Bits128 wordBit<Bits128>(int j) {
    Bits128 b;
    if (j < 64)
        b.lo = uint64_t(1) << j;
    else
        b.hi = uint64_t(1) << (j - 64);
    return b;
}

/// @brief Word with all bits set.
template<typename Word>
Word wordAll() {
    return ~Word{};
}

/// @brief Number of columns a word holds.
template<typename Word>
constexpr int wordWidth() {
    return int(sizeof(Word) * 8);
}

/// @brief Word from the blocks of a matrix row; `false` if the row has a column beyond the word.
template<typename Word>
bool wordFromRow(bit_chunk const& row, Word& word);
template<>
inline bool wordFromRow<uint64_t>(bit_chunk const& row, uint64_t& word) {
    word = 0;
    int k = 0;
    for (auto d = row.begin; d != row.end; ++d, ++k) {
        if (k >= 2) {
            if (*d)
                return false;
            continue;
        }
        word |= uint64_t(*d) << (32 * k);
    }
    return true;
}
template<>
inline bool wordFromRow<Bits128>(bit_chunk const& row, Bits128& word) {
    word = Bits128{};
    int k = 0;
    for (auto d = row.begin; d != row.end; ++d, ++k) {
        if (k >= 4) {
            if (*d)
                return false;
            continue;
        }
        uint64_t& half = k < 2 ? word.lo : word.hi;
        half |= uint64_t(*d) << (32 * (k & 1));
    }
    return true;
}

/// @}

/// @brief `RuncLocalState` on rows of at most `wordWidth<Word>()` columns kept in machine words.
///
/// The same node of the same search tree as `RuncLocalState`: every step (which rows stay
/// uncovered and in which order, which support groups are kept or dropped, which columns are
/// removed, which row is chosen to branch on and the order of its columns) mirrors the general
/// state, so `RuncGlobalState` reports the identical sequence of nodes on both paths. Rows are kept
/// unmasked (as in the matrix) and are intersected with the remaining columns where the general
/// state applies `restCols_` as a mask; a raw row is never zero (`RuncGlobalState` stops at a zero
/// row before the search), so 0 marks a removed row.
template<typename Word>
class FixedWidthLocalState {
public:
    /// Same meaning as `RuncLocalState::AddResult`.
    enum AddResult {
        Inner,
        Cover,
        Uncompatable,
        Uncoverable
    };

    /// @brief State with room for @p m rows.
    explicit FixedWidthLocalState(size_t m = 0) : uncovered_(m), support_(m) {
        groups_.reserve(m);
    }

    /// @brief Make this the root: all @p rows uncovered, the remaining columns their union.
    void SetRoot(std::vector<Word> const& rows, AlgorithmType alg) {
        uncoveredCount_ = 0;
        supportCount_ = 0;
        groups_.clear();
        rest_ = Word{};
        for (auto r : rows) {
            uncovered_[uncoveredCount_++] = r;
            rest_ = rest_ | r;
        }
        collect(alg);
    }

    /// @brief The highest remaining column among the columns to branch on, or -1.
    int NextColumn() const {
        Word w = rest_ & collected_;
        return wordAny(w) ? wordLast(w) : -1;
    }

    /// @brief Remove column @p j from the remaining columns.
    void RemoveColumn(int j) {
        rest_ = rest_ & ~wordBit<Word>(j);
    }

    /// @brief `RuncLocalState::AddColumn`: make this the child of @p parent obtained by adding @p col.
    AddResult AddColumn(FixedWidthLocalState const& parent, int col) {
        supportCount_ = 0;
        groups_.clear();
        rest_ = parent.rest_;
        Word colBit = wordBit<Word>(col);

        // RuncLocalState::SetUncovered: the rows hit by col go to the end in reverse order
        Word* u = uncovered_.data();
        int begin = 0;
        int end = parent.uncoveredCount_;
        for (int i = 0; i < parent.uncoveredCount_; ++i) {
            Word r = parent.uncovered_[i];
            if (wordAny(r & colBit)) {
                u[--end] = r;
                continue;
            }
            if (!wordAny(r & rest_))
                return Uncoverable;
            u[begin++] = r;
        }
        if (begin == 0) {
            // Every row is hit by col: a cover. The general state reaches the same result after
            // building the support groups, which cannot fail here: col is a remaining column, and
            // the remaining columns exclude the columns common to all rows of a group (see
            // AddSupportGroup), so every group keeps a row. The state of a cover is never used.
            return Cover;
        }
        if (!AddSupportGroup(Word{}, col, u + end, u + parent.uncoveredCount_))
            return Uncoverable;
        uncoveredCount_ = begin;

        // RuncLocalState::SetUncoveredSupportRows
        for (auto const& g : parent.groups_) {
            if (!AddSupportGroup(colBit, g.column, parent.support_.data() + g.begin, parent.support_.data() + g.end))
                return Uncompatable;
        }

        return uncoveredCount_ == 0 ? Cover : Inner;
    }

    /// @brief `RuncLocalState::collectCols` + `setCollectedCols`: choose the columns to branch on.
    void collect(AlgorithmType alg) {
        switch (alg) {
            case OPT:
                RemoveDependentRows();
                collected_ = wordAll<Word>();
                {
                    Word all{};
                    for (int i = 0; i < uncoveredCount_; ++i)
                        all = all | uncovered_[i];
                    rest_ = rest_ & all;
                }
                return;
            case RUNC:
                collected_ = uncovered_[0];
                return;
            case RUNCM:
                collected_ = GetMinUncovered(1);
                return;
        }
    }

private:
    struct Group {
        int begin;
        int end;
        int column;
    };

    std::vector<Word> uncovered_;
    int uncoveredCount_ = 0;
    std::vector<Word> support_;
    int supportCount_ = 0;
    std::vector<Group> groups_;
    Word rest_{};
    Word collected_{};

    // RuncLocalState::AddUncoveredSupportRows, rows hitting `skip` are left out
    bool AddSupportGroup(Word skip, int column, Word const* begin, Word const* end) {
        int s = supportCount_;
        Word common = rest_;
        bool any = wordAny(common);
        for (; begin != end; ++begin) {
            Word r = *begin;
            if (wordAny(r & skip))
                continue;
            if (any) {
                common = common & r;
                any = wordAny(common);
            }
            if (!any && !wordAny(r & rest_)) {
                // The row can never be hit again, so the column keeps its support: drop the group
                supportCount_ = s;
                return true;
            }
            support_[supportCount_++] = r;
        }

        if (s == supportCount_)
            return false;

        if (any)
            rest_ = rest_ & ~common;

        groups_.push_back(Group{s, supportCount_, column});
        return true;
    }

    // RuncLocalState::GetMinUncovered
    Word GetMinUncovered(int mc) const {
        Word row{};
        int count = wordWidth<Word>() + 1;
        for (int i = 0; i < uncoveredCount_; ++i) {
            Word r = uncovered_[i];
            int c = wordCount(r & rest_);
            if (c < count) {
                if (c <= mc)
                    return r;
                count = c;
                row = r;
            }
        }
        return row;
    }

    // BoolMatrixPreparation::RemoveDependentRows(rows, mask) with the mask rest_
    void RemoveDependentRows() {
        Word* u = uncovered_.data();
        int s = uncoveredCount_;
        int removed = 0;
        for (int i = 0; i < s - 1; ++i) {
            if (!wordAny(u[i]))
                continue;
            Word a = u[i] & rest_;
            for (int j = i + 1; j < s; ++j) {
                if (!wordAny(u[j]))
                    continue;
                Word b = u[j] & rest_;
                Word ab = a & b;
                if (!(a == b) && ab == a) {
                    // a is a proper subset of b
                    u[j] = Word{};
                    ++removed;
                } else if (ab == b) {
                    // b is a subset of a (or equal)
                    u[i] = Word{};
                    ++removed;
                    break;
                }
            }
        }

        if (removed > 0) {
            // pop_if: move the last row into a removed slot
            for (int i = 0; i < s;) {
                if (wordAny(u[i]))
                    ++i;
                else
                    u[i] = u[--s];
            }
            uncoveredCount_ = s;
        }
    }
};

}  // namespace Runc

}  // namespace dual
