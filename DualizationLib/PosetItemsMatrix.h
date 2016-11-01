#pragma once

#include <vector>

#include "PosetItem.h"
#include "bit_chunk.h"


/*!
    Матрица элементов частичных порядков
    Задаётся набором столбцов
*/
class PosetItemsMatrix
{
public:

    typedef PosetItems Row;
    typedef PosetItems Column;
    typedef std::vector<Column> Columns;
    typedef std::vector<Row> Rows;


    class ColumnRef
    {
        Column& _column;
    
    public:
    
        ColumnRef(Column& column)
            :_column(column)
        {
        }

        PosetItem& operator [](int i) const
        {
            return _column[i];
        }        
    };

    template <typename ColumnsT, typename ItemT>
    class RowRefT
    {
        int _i;
        ColumnsT& _columns;

    public:

        struct iterator
        {
            RowRefT const& _rowRef;
            int _j;

            iterator(RowRefT const& rowRef, int j)
                :_rowRef(rowRef), _j(j)
            {
            }

            iterator& operator ++()
            {
                ++_j;
                return *this;
            }

            ItemT& operator *() const
            {
                return _rowRef[_j];
            }
        };


        RowRefT(ColumnsT& columns, int i)
            :_columns(columns), _i(i)
        {
        }

        ItemT& operator [](int j) const
        {
            return _columns[j][_i];
        }

        iterator begin() const
        {
            return iterator(*this, 0);
        }

        iterator end() const
        {
            return iterator(*this, _columns.size());
        }

        int size() const
        {
            return _columns.size();
        }

    };

    typedef RowRefT<Columns const, PosetItem const> CRowRef;

    typedef RowRefT<Columns, PosetItem> RowRef;


private:


    Columns _columns;

    Columns _transposed;

public:

    UTILITY_MOVE_DEFAULT_MEMBERS(PosetItemsMatrix, (_columns)(_transposed));

    PosetItemsMatrix(){}

    int GetHeight() const
    {
        return _columns.empty() ? 0 : _columns.front().size();
    }

    int GetWidth() const
    {
        return _columns.size();
    }

    Columns const& GetColumns() const
    {
        return _columns;
    }

    Column const& GetColumn(int j) const
    {
        return _columns[j];
    }

    ColumnRef GetColumn(int j)
    {
        return _columns[j];
    }

    RowRef operator [](int i)
    {
        return RowRef(_columns, i);
    }

    CRowRef operator [](int i) const
    {
        return CRowRef(_columns, i);
    }

    void Set(int i, int j, PosetItem const& a)
    {
        _columns[j][i] = a;
    }

    void Resize(int m, int n);

    void Reserve(int m, int n);

    void Clear(int n = 0);

    template< typename Row>
    void AddRow(Row const& row)
    {
        if (_columns.empty())
            _columns.resize(row.size());

        assert(row.size() == _columns.size());
        for (int i = 0, n = _columns.size(); i != n; ++i)
            _columns[i].push_back(row[i]);
    }

    void DropRow();

    void RemoveRow(int i);

    void RemoveRows(bit_chunk const& mask);    
        
    CRowRef GetBackRow() const
    {
        return (*this)[GetHeight()-1];
    }

    /*RowRef GetBackRow()
    {
        return (*this)[GetHeight()-1];
    }*/

    PosetItemsMatrix Transpose() const;

    void AssignTransposed(PosetItemsMatrix const& matrix);

    void DeleteDuplicateColumns();

    void DeleteDuplicateRows();

    void CopyRow(int i, PosetItems& row) const;
    
};

