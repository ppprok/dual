/// @file
/// @brief Growable bit matrix `bit_matrix`.

#pragma once

#include <algorithm>
#include <climits>
#include <stdexcept>

#include <dual/bit_chunk.h>
#include <dual/blob.h>
#include <dual/containers.h>

namespace dual {

/// @brief Bit matrix: rows are `bit_chunk` views into one contiguous block buffer.
///
/// Every row occupies a slot of whole blocks in a shared buffer; a row added with `n` columns gets
/// `bit_chunk::block_for_bits(n)` blocks. The width (`width()`) grows automatically when a bit is set
/// in a column past it, and the buffer grows (and rows are re-laid out) as needed, so bits may be set
/// in any order.
///
/// @warning Growing the matrix (`set`, `add_bit`, `add_row*`, `reserve`, `zero`) may reallocate the
/// buffer and the row container: references to rows and `bit_chunk` copies of rows obtained earlier
/// become invalid.
/// @tparam bit_rows_type Container of row views, e.g. `bit_chunks` (`std::vector<bit_chunk>`).
template<typename bit_rows_type>
class bit_matrix_ {
public:
    /// Container of rows.
    typedef bit_rows_type bit_rows;

    /// Row type (a `bit_chunk` view).
    typedef typename bit_rows::value_type bit_row;

private:
    int _n;

    blob<typename bit_chunk::Block> _data;

    bit_rows _rows;

public:
    /// @brief Matrix without rows and with @p n columns (`n >= 0`).
    bit_matrix_(int n = 0) {
        assert(n >= 0);
        _n = n;
    }

    /// @brief Zero matrix with @p m rows and @p n columns.
    /// @throws std::length_error If the matrix needs more blocks than fit into `int`.
    bit_matrix_(int m, int n) {
        zero(m, n);
    }

    /// @brief Number of blocks in use (from the buffer start to the end of the last row's slot).
    int size() const {
        return (int) size_for_width(_n);
    }

    /// @brief Number of blocks in use if the last row had a slot for @p n columns.
    long long size_for_width(int n) const {
        if (_rows.empty())
            return 0;

        auto const& last = _rows.back();
        long long begin = last.begin - _data.begin();
        return std::max(begin + bit_chunk::block_for_bits(n), (long long) (last.end - _data.begin()));
    }

    /// @brief Number of allocated blocks.
    int capacity() const {
        return _data.size();
    }

    /// @brief Number of rows.
    int height() const {
        return _rows.size();
    }

    /// @brief Number of columns: one past the highest column ever set or requested.
    int width() const {
        return _n;
    }

    /// @brief Create a zero matrix: drop all rows and add @p m zero rows with @p n columns.
    /// @throws std::length_error If the matrix needs more blocks than fit into `int`.
    void zero(int m, int n) {
        _rows.clear();
        _n = n;
        add_rows(m, n);
    }

    /// @brief Reserve the required number of blocks.
    ///
    /// Never shrinks the buffer below the blocks used by the rows; the rows are rebased if the
    /// buffer moves.
    void reserve(int blocks) {
        blocks = std::max(blocks, size());
        if (_data.size() >= blocks)
            return;

        auto old = _data.begin();
        _data.resize(blocks);
        if (old == _data.begin())
            return;

        for (auto& r : _rows) {
            r -= old;
            r += _data.begin();
        }
    }

    /// @brief Reserve enough blocks for @p m rows with @p n columns.
    /// @throws std::length_error If that needs more blocks than fit into `int`.
    void reserve(int m, int n) {
        _rows.reserve(m);
        reserve(checked_blocks((long long) m * bit_chunk::block_for_bits(n)));
    }

    /// @brief Set or clear the bit in row @p i and column @p j.
    /// @return For `v == true` the result of `set(i, j)`; for `v == false` always `true` (see `reset`).
    /// @throws std::length_error See `set(int, int)`.
    bool set(int i, int j, bool v) {
        if (v)
            return set(i, j);

        reset(i, j);
        return true;
    }

    /// @brief Set the bit in row @p i and column @p j.
    ///
    /// Rows and columns may come in any order: missing rows are added as zero rows, the width grows
    /// to `j + 1` if needed, and rows are re-laid out when a column does not fit into a row's slot
    /// (the slot size at least doubles, so setting bits in any order costs amortized linear time).
    /// @return `false` (and nothing changes) when @p i or @p j is negative or equal to `INT_MAX`;
    ///         `true` otherwise.
    /// @throws std::length_error If the matrix would need more blocks than fit into `int`.
    bool set(int i, int j) {
        if (i < 0 || j < 0 || i == INT_MAX || j == INT_MAX)
            return false;

        int m = height();
        if (i == m - 1) {
            add_bit(j);
            return true;
        }

        if (i >= m) {
            add_rows(i + 1 - m, std::max(_n, j + 1));
        } else {
            // An intermediate row has no room past its stride: re-lay out the rows with a larger one
            int b = bit_chunk::block_for_bits(j + 1);
            if (_rows[i].begin + b > _rows[i + 1].begin)
                restride(b);
            if (_n <= j)
                _n = j + 1;
        }

        _rows[i].set(j);
        return true;
    }

private:
    // Convert a block count computed in 64 bits to int, the size type of the containers
    static int checked_blocks(long long blocks) {
        if (blocks > INT_MAX)
            throw std::length_error("bit_matrix: too many blocks");
        return (int) blocks;
    }

    // Copy the rows into a new buffer where every row but the last one has a stride of at least b blocks.
    // The stride at least doubles, so setting bits in any order costs amortized linear time.
    void restride(int b) {
        int m = height();
        assert(m > 0);
        long long stride = bit_chunk::block_for_bits(_n);
        for (int t = 0; t + 1 < m; ++t)
            stride = std::max(stride, 2 * (long long) (_rows[t + 1].begin - _rows[t].begin));
        stride = std::max(stride, (long long) b);

        long long last = std::max(stride, (long long) _rows.back().size());
        blob<typename bit_chunk::Block> data(checked_blocks((m - 1) * stride + last));
        std::fill(data.begin(), data.end(), 0);

        auto p = data.begin();
        for (auto& r : _rows) {
            std::copy(r.begin, r.end, p);
            r.end = p + r.size();
            r.begin = p;
            p += stride;
        }
        _data = std::move(data);
    }

public:
    /// @brief Clear the bit in row @p i and column @p j; no-op if the row does not exist or `j < 0`.
    void reset(int i, int j) {
        if (j >= 0 && inside(_rows, i))
            _rows[i].set(j, false);
    }

    /// @brief Get the bit in row @p i (must exist) and column @p j; `false` past the row's end.
    bool get(int i, int j) const {
        assert(inside(_rows, i));
        return _rows[i].get(j);
    }

    /// @brief Get the rows.
    bit_rows const& get_rows() const {
        return _rows;
    }

    /// @brief Get the rows.
    bit_rows& get_rows() {
        return _rows;
    }

    /// @brief Get the row with the given index (`0 <= i < height()`).
    bit_row const& get_row(int i) const {
        assert(inside(_rows, i));
        return _rows[i];
    }

    /// @brief Get the row with the given index (`0 <= i < height()`).
    /// @warning Setting bits through the returned view does not grow the matrix; stay within
    ///          `width()` columns.
    bit_row& get_row(int i) {
        assert(inside(_rows, i));
        return _rows[i];
    }

    /// @brief Append @p m zero rows with `max(width(), n)` columns; the width becomes that value.
    /// @throws std::length_error If the rows would need more blocks than fit into `int` (nothing changes then).
    void add_rows(int m, int n) {
        if (m <= 0)
            return;

        n = std::max(_n, n);
        int bn = bit_chunk::block_for_bits(n);
        long long s = (long long) m * bn;
        long long cs = size_for_width(n);
        if (capacity() - cs < s) {
            checked_blocks(cs + s);
            reserve((int) std::min<long long>(cs * 5LL / 4 + s, INT_MAX));
        }

        _n = n;
        auto p = _data.begin() + cs;

        _rows.reserve(_rows.size() + m);
        for (int t = 0; t < m; ++t, p += bn) {
            _rows.emplace_back(p, p);
        }
    }

    /// @brief Set bit @p j (`j >= 0`) in the last row, adding a first row if there is none.
    ///
    /// The last row may grow past its slot; the buffer is enlarged if needed.
    /// @throws std::length_error If the matrix would need more blocks than fit into `int`.
    void add_bit(int j) {
        if (_rows.empty()) {
            add_rows(1, j + 1);
        } else {
            // make sure the buffer has enough memory allocated
            int b = bit_chunk::block_for_bits(j + 1);
            if (_rows.back().begin + b > _data.end()) {
                long long need = checked_blocks((_rows.back().begin - _data.begin()) + (long long) b);
                reserve((int) std::min<long long>(std::max(need, _data.size() * 5LL / 4 + b), INT_MAX));
            }

            if (_n <= j)
                _n = j + 1;
        }

        _rows.back().set(j);
    }

    /// @brief Keep only the first @p m rows of the matrix (the buffer is kept).
    void trim_rows(int m) {
        if (m <= 0)
            _rows.clear();
        else if ((int) _rows.size() > m)
            _rows.resize(m);
    }

    /// @brief Clear all bits of row @p i (`0 <= i < height()`).
    void zero_row(int i) {
        assert(inside(_rows, i));
        _rows[i].clear();
    }

    /// @brief Assign the given bits to the i-th row.
    /// @warning @p row must fit into the row's slot (at most `width()` columns).
    void assign_row(int i, bit_chunk row) {
        assert(inside(_rows, i));
        _rows[i].assign(row);
    }

    /// @brief Find a row that satisfies the predicate.
    /// @return Index of the first such row, or -1.
    template<typename Pred>
    int find_row(Pred pred) const {
        return find_if(_rows, pred);
    }

    /// @brief Find a row that is a subset of the given row.
    /// @return Index of the first such row, or -1.
    int find_sub_row(bit_chunk suprow) const {
        return find_row([suprow](bit_chunk const& row) { return row.is_subset_of(suprow); });
    }

    /// @brief Find a row equal to the given row.
    /// @return Index of the first such row, or -1.
    int find_equal_row(bit_chunk row) const {
        return find_row([row](bit_chunk const& r) { return row == r; });
    }

    /// @brief Remove the rows that satisfy the predicate, keeping the order of the others.
    /// @return Number of removed rows (0 if no row satisfies the predicate).
    template<typename Pred>
    int remove_rows(Pred pred) {
        int m = height();
        if (m == 0)
            return 0;
        int i = find_row(pred);
        if (i == -1)
            return 0;

        for (int t = i + 1; t < m; ++t) {
            if (!pred(_rows[t]))
                _rows[i++].assign(_rows[t]);
        }

        _rows.resize(i);
        return m - i;
    }

    /// @brief Remove zero rows.
    /// @return See `remove_rows`.
    int remove_zero_rows() {
        return remove_rows([](bit_chunk const& r) { return r.none(); });
    }

    /// @brief Remove the rows covered by the columns (rows that intersect @p columns).
    /// @return See `remove_rows`.
    int remove_covered_rows(bit_chunk columns) {
        return remove_rows([columns](bit_chunk const& r) { return r.intersects(columns); });
    }

    /// @brief Remove the rows equal to the given row.
    /// @return See `remove_rows`.
    int remove_equal_rows(bit_chunk row) {
        return remove_rows([row](bit_chunk const& r) { return r == row; });
    }

    /// @brief Remove the rows that are supersets of the given row.
    /// @return See `remove_rows`.
    int remove_sup_rows(bit_chunk subrow) {
        return remove_rows([subrow](bit_chunk const& r) { return subrow.is_subset_of(r); });
    }

    /// @brief Append a copy of @p row with an explicitly specified length of @p n columns.
    /// @param row Bits of the new row; must fit into `max(width(), n)` columns.
    /// @param n Number of columns of the new row's slot.
    /// @return The new row.
    /// @throws std::length_error If the matrix would need more blocks than fit into `int`.
    bit_row& add_row(bit_row const& row, int n) {
        add_rows(1, n);
        _rows.back().assign(row);
        return _rows.back();
    }

    /// @brief Append a copy of @p row, sized to its highest set bit.
    /// @throws std::length_error If the matrix would need more blocks than fit into `int`.
    void add_row(bit_chunk row) {
        add_row(row, row.find_last() + 1);
    }

    /// @brief Append a zero row, growing the width to at least @p n columns.
    /// @throws std::length_error If the matrix would need more blocks than fit into `int`.
    void add_row(int n) {
        if (_n < n)
            _n = n;
        add_row();
    }

    /// @brief Append a zero row with `width()` columns (at least one).
    /// @throws std::length_error If the matrix would need more blocks than fit into `int`.
    void add_row() {
        if (_n <= 1)
            add_rows(1, 1);
        else
            add_rows(1, _n);
    }
};

/// Bit matrix with rows stored as `std::vector<bit_chunk>`.
typedef bit_matrix_<bit_chunks> bit_matrix;

}  // namespace dual
