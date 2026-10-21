#pragma once

#include <algorithm>
#include <climits>
#include <stdexcept>

#include "bit_chunk.h"
#include "blob.h"
#include "containers.h"

// Bit matrix
template<typename bit_rows_type>
class bit_matrix_ {
public:
    typedef bit_rows_type bit_rows;

    typedef typename bit_rows::value_type bit_row;

private:
    int _n;

    blob<typename bit_chunk::Block> _data;

    bit_rows _rows;

public:
    bit_matrix_(int n = 0) {
        assert(n >= 0);
        _n = n;
    }

    bit_matrix_(int m, int n) {
        zero(m, n);
    }

    // Number of blocks in use
    int size() const {
        return (int) size_for_width(_n);
    }

    // Number of blocks in use if the matrix had n columns
    long long size_for_width(int n) const {
        if (_rows.empty())
            return 0;

        auto const& last = _rows.back();
        long long begin = last.begin - _data.begin();
        return std::max(begin + bit_chunk::block_for_bits(n), (long long) (last.end - _data.begin()));
    }

    // Number of allocated blocks
    int capacity() const {
        return _data.size();
    }

    // Number of rows
    int height() const {
        return _rows.size();
    }

    // Number of columns
    int width() const {
        return _n;
    }

    // Create a zero matrix
    void zero(int m, int n) {
        _rows.clear();
        _n = n;
        add_rows(m, n);
    }

    // Reserve the required number of blocks. Never shrinks the buffer below the blocks used by the rows
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

    // Reserve enough blocks for the given size
    void reserve(int m, int n) {
        _rows.reserve(m);
        reserve(checked_blocks((long long) m * bit_chunk::block_for_bits(n)));
    }

    // Set or clear the bit in row i and column j
    bool set(int i, int j, bool v) {
        if (v)
            return set(i, j);

        reset(i, j);
        return true;
    }

    // Set the bit in row i and column j. Rows and columns may come in any order: missing rows are added
    // as zero rows and rows are re-laid out when a column does not fit. Returns false and changes nothing
    // when i or j is negative or equal to INT_MAX. Throws std::length_error when the matrix would need more
    // blocks than fit into int
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
    // Clear a bit
    void reset(int i, int j) {
        if (j >= 0 && inside(_rows, i))
            _rows[i].set(j, false);
    }

    // Get the bit in row i and column j
    bool get(int i, int j) const {
        assert(inside(_rows, i));
        return _rows[i].get(j);
    }

    // Get the rows
    bit_rows const& get_rows() const {
        return _rows;
    }

    // Get the rows
    bit_rows& get_rows() {
        return _rows;
    }

    // Get the row with the given index
    bit_row const& get_row(int i) const {
        assert(inside(_rows, i));
        return _rows[i];
    }

    // Get the row with the given index
    bit_row& get_row(int i) {
        assert(inside(_rows, i));
        return _rows[i];
    }

    // Append m zero rows with n columns. Throws std::length_error and changes nothing when the rows would
    // need more blocks than fit into int
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

    // Add a bit to the last row
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

    // Keep only the first m rows of the matrix
    void trim_rows(int m) {
        if (m <= 0)
            _rows.clear();
        else if ((int) _rows.size() > m)
            _rows.resize(m);
    }

    void zero_row(int i) {
        assert(inside(_rows, i));
        _rows[i].clear();
    }

    // Assign the given bits to the i-th row
    void assign_row(int i, bit_chunk row) {
        assert(inside(_rows, i));
        _rows[i].assign(row);
    }

    // Find a row that satisfies the predicate
    template<typename Pred>
    int find_row(Pred pred) const {
        return find_if(_rows, pred);
    }

    // Find a row that is a subset of the given row
    int find_sub_row(bit_chunk suprow) const {
        return find_row([suprow](bit_chunk const& row) { return row.is_subset_of(suprow); });
    }

    // Find a row equal to the given row
    int find_equal_row(bit_chunk row) const {
        return find_row([row](bit_chunk const& r) { return row == r; });
    }

    // Remove the rows that satisfy the predicate
    template<typename Pred>
    int remove_rows(Pred pred) {
        int m = height();
        if (m == 0)
            return 0;
        int i = find_row(pred);
        if (i == -1)
            return m;

        for (int t = i + 1; t < m; ++t) {
            if (!pred(_rows[t]))
                _rows[i++].assign(_rows[t]);
        }

        _rows.resize(i);
        return m - i;
    }

    // Remove zero rows
    int remove_zero_rows() {
        return remove_rows([](bit_chunk const& r) { return r.none(); });
    }

    // Remove the rows covered by the columns
    int remove_covered_rows(bit_chunk columns) {
        return remove_rows([columns](bit_chunk const& r) { return r.intersects(columns); });
    }

    // Remove the rows equal to the given row
    int remove_equal_rows(bit_chunk row) {
        return remove_rows([row](bit_chunk const& r) { return r == row; });
    }

    // Remove the rows that are supersets of the given row
    int remove_sup_rows(bit_chunk subrow) {
        return remove_rows([subrow](bit_chunk const& r) { return subrow.is_subset_of(r); });
    }

    // Append a row with an explicitly specified length
    bit_row& add_row(bit_row const& row, int n) {
        add_rows(1, n);
        _rows.back().assign(row);
        return _rows.back();
    }

    // Append a row
    void add_row(bit_chunk row) {
        add_row(row, row.find_last() + 1);
    }

    // Append a zero row
    void add_row(int n) {
        if (_n < n)
            _n = n;
        add_row();
    }

    // Append a zero row
    void add_row() {
        if (_n <= 1)
            add_rows(1, 1);
        else
            add_rows(1, _n);
    }
};

typedef bit_matrix_<bit_chunks> bit_matrix;
