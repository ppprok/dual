#pragma once

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
        if (_rows.empty())
            return 0;

        auto p = _rows.back().begin + bit_chunk::block_for_bits(_n);
        if (p < _rows.back().end)
            p = _rows.back().end;

        return p - _data.begin();
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

    // Reserve the required number of blocks
    void reserve(int blocks) {
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
        reserve(m * bit_chunk::block_for_bits(n));
    }

    // Set or clear the bit in row i and column j
    bool set(int i, int j, bool v) {
        if (v)
            return set(i, j);

        reset(i, j);
        return true;
    }

    // Set the bit in row i and column j
    bool set(int i, int j) {
        if (i < 0 || j < 0)
            return false;

        int m = height();
        if (m == 0 || i == m - 1) {
            add_bit(j);
            return true;
        }

        if (i >= m) {
            if (_n <= j)
                _n = j + 1;
            add_rows(i + 1 - m, _n);
        } else {
            // Check whether the bit can be inserted in an intermediate row
            int b = bit_chunk::block_for_bits(j + 1);
            if (_rows[i].begin + b > _rows[i + 1].begin)
                return false;
            if (_n <= j)
                _n = j + 1;
        }

        _rows[i].set(j);
        return true;
    }

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

    // Append m zero rows with n columns
    void add_rows(int m, int n) {
        if (m <= 0)
            return;

        if (_n < n)
            _n = n;

        int bn = bit_chunk::block_for_bits(_n);
        int s = m * bn;
        int cs = size();
        if (capacity() - cs < s)
            reserve(cs * 5 / 4 + s);

        auto p = _data.begin() + size();

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
            if (_n <= j)
                _n = j + 1;

            // make sure the buffer has enough memory allocated
            int b = bit_chunk::block_for_bits(j + 1);
            if (_rows.back().begin + b > _data.end())
                reserve(_data.size() * 5 / 4 + b);
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
