#pragma once

#include "bit_chunk.h"
#include "blob.h"
#include "containers.h"


// Битовая матрица
template <typename bit_rows_type>
class bit_matrix_
{
public:
    
    typedef bit_rows_type bit_rows;

    typedef typename bit_rows::value_type bit_row;

private:

    int _n;

    blob<typename bit_chunk::Block> _data;

    bit_rows _rows;

	    
public:

	bit_matrix_(int n = 0)
    {
        assert(n>=0);
        _n = n;        
    }

    bit_matrix_(int m, int n)
    {
        zero(m, n);
    }
        
    // Число используемых блоков
    int size() const
    {
        if (_rows.empty())
            return 0;

        auto p = _rows.back().begin + bit_chunk::block_for_bits(_n);
        if (p < _rows.back().end)
            p = _rows.back().end;

        return p - _data.begin();
    }

    // Число выделенных блоков
    int capacity() const
    {
        return _data.size();
    }

    // Число строк
    int height() const
    {
        return _rows.size();
    }

    // Число столбцов
    int width() const
    {
        return _n;
    }

    // Создать нулевую матрицу
	void zero(int m, int n)
    {
        _rows.clear();
        _n = n;
        add_rows(m, n);
    }

    // Зарезервировать требуемое количество блоков
    void reserve(int blocks)
    {
        if (_data.size() >= blocks)
            return;

        auto old = _data.begin();
        _data.resize(blocks);
        if (old == _data.begin())
            return;

        for (auto& r:_rows)
        {
            r-=old;
            r+=_data.begin();
        }
    }

    // Зарезервировать достаточное количество блоков для указанного размера
    void reserve(int m, int n)
    {
        _rows.reserve(m);
        reserve(m * bit_chunk::block_for_bits(n));
    }

    // Установить или сбросить бит в строке i и столбце j
    bool set(int i, int j, bool v)
    {
        if (v)
            return set(i, j);

        reset(i, j);
        return true;
    }

    // Установить бит в строке i и столбце j
    bool set( int i, int j )
    {
        if (i < 0 || j < 0)
            return false;

        int m = height();
        if (m == 0 || i == m-1)
        {
            add_bit(j);
            return true;
        }

        if (i >= m)
        {
            if (_n <= j)
                _n = j+1;
            add_rows(i + 1 - m, _n);
        }
        else
        {
            // Проверяем, можно ли вставить бит в промежуточной строке
            int b = bit_chunk::block_for_bits(j+1);
            if (_rows[i].begin + b > _rows[i+1].begin)
                return false;
            if (_n <= j)
                _n = j+1;
        }

        _rows[i].set(j);
        return true;
    }

    // Обнулить бит
    void reset( int i, int j )
    {
        if (j >=0 && inside(_rows, i))
            _rows[i].set(j, false);

    }

    // Получить бит в строке i столбце j
    bool get(int i, int j) const
    {
        assert(inside(_rows, i));
        return _rows[i].get(j);
    }
    
    // Получить набор строк
    bit_rows const& get_rows() const
    {
        return _rows;
    }

    // Получить набор строк
    bit_rows& get_rows()
    {
        return _rows;
    }

    // Получить строку с указанным номером
    bit_row const& get_row(int i) const
    {
        assert(inside(_rows, i));
        return _rows[i];
    }

    // Получить строку с указанным номером
    bit_row& get_row(int i)
    {
        assert(inside(_rows, i));
        return _rows[i];
    }

    // Добавить m нулевых строк c n столбцами
    void add_rows(int m, int n)
    {
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
        for(int t = 0; t < m; ++t, p += bn)
        {
            _rows.emplace_back(p, p);
        }
    }

    // Добавить бит в последнюю строку
    void add_bit(int j)
    {
        if (_rows.empty())
        {
            add_rows(1, j+1);
        }
        else
        {
            if (_n <= j)
                _n = j+1;

            // убеждаемся, что в буфере выделено достаточно памяти
            int b = bit_chunk::block_for_bits(j+1);
            if (_rows.back().begin + b > _data.end())
                reserve(_data.size() * 5 / 4 + b);
        }

        _rows.back().set(j);

    }
    
    // Оставить в матрице первые m строк
    void trim_rows( int m )
    {
        if (m <= 0)
            _rows.clear();
        else if ((int)_rows.size() > m)
            _rows.resize(m);

    }

    void zero_row(int i)
    {
        assert(inside(_rows, i));
        _rows[i].clear();
    }

    // Списвоить i-ой строке указанные биты
    void assign_row(int i, bit_chunk row)
    {
        assert(inside(_rows, i));
        _rows[i].assign(row);
    }

    // Найти строку, удовлетворяющую условию
    template <typename Pred>
    int find_row(Pred pred) const
    {
        return find_if(_rows, pred);
    }

    // Найти строку, охватываемую указанной строкой
    int find_sub_row(bit_chunk suprow) const
    {
        return find_row([suprow](bit_chunk const& row){return row.is_subset_of(suprow);});
    }

    // Найти строку, равную указанной строке
    int find_equal_row(bit_chunk row) const
    {
        return find_row([row](bit_chunk const& r){return row == r;});
    }
        
    // Удалить строки, удовлетворяющие условию
    template <typename Pred>
    int remove_rows(Pred pred)
    {
        int m = height();
        if (m == 0)
            return 0;
        int i = find_row(pred);
        if (i == -1)
            return m;
        
        for (int t = i+1; t < m; ++t)
        {
            if (! pred(_rows[t]))
                _rows[i++].assign(_rows[t]);
        }

        _rows.resize(i);
        return m - i;
    }

    // Удалить нулевые строки
    int remove_zero_rows()
    {
        return remove_rows([](bit_chunk const& r){return r.none();});
    }

    // Удалить строки, покрытые столбцами
    int remove_covered_rows(bit_chunk columns)
    {
        return remove_rows([columns](bit_chunk const& r){return r.intersects(columns);});
    }

    // Удалить строки, совпадающие с указанной строкой
    int remove_equal_rows(bit_chunk row)
    {
        return remove_rows([row](bit_chunk const& r){return r == row;});
    }

    // Удалить строки, охватывающие указанную
    int remove_sup_rows(bit_chunk subrow)
    {
        return remove_rows([subrow](bit_chunk const& r){return subrow.is_subset_of(r);});
    }
    
    // Добавить строку с явным указанием длины
    bit_row& add_row( bit_row const& row, int n )
    {
        add_rows(1, n);
        _rows.back().assign(row);
        return _rows.back();
    }

    // Добавить строку
    void add_row( bit_chunk row)
    {
        add_row(row, row.find_last() + 1);
    }
  
    // Добавить нулевую строку
    void add_row(int n)
    {
        if (_n < n)
            _n = n;
        add_row();

    }

    // Добавить нулевую строку
    void add_row()
    {
        if (_n <= 1)
            add_rows(1, 1);
        else
            add_rows(1, _n);

    }
};

typedef bit_matrix_<bit_chunks> bit_matrix;
