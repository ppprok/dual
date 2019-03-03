
#include "PosetItemsMatrix.h"
#include "containers.h"


/*PosetItemsMatrix::Row& PosetItemsMatrix::CopyRow( int i, Row& dest ) const
{
    assert(i >=0 && i < GetHeight());
    dest.clear();
    dest.reserve(GetWidth());
    for (auto& c : _columns)
        dest.push_back(c[i]);
    return dest;
}*/

void PosetItemsMatrix::RemoveRows( bit_chunk const& mask )
{
    int s = GetHeight();
    for(int i = mask.find_first(); i != -1 && i < s; i = mask.find_next(i))
    {
        while (i < s && mask.get(s-1))
        {
            DropRow();
            --s;
        }
        if (i == s)
            break;
        RemoveRow(i);
        --s;
    }
}

void PosetItemsMatrix::RemoveRow( int i )
{
    assert(i >=0 && i < GetHeight());

    for (auto& c : _columns)
    {
        c[i] = c.back();
        c.pop_back();
    }
}

void PosetItemsMatrix::DropRow()
{
    assert(GetHeight() > 0);
    for (auto& c : _columns)
        c.pop_back();
}


void PosetItemsMatrix::Clear( int n /*= 0*/ )
{
    Resize(0, n);
}

void PosetItemsMatrix::Reserve( int m, int n )
{
    _columns.resize(n);
    for (auto& c : _columns)
        c.reserve(m);
}

void PosetItemsMatrix::Resize( int m, int n )
{
    _columns.resize(n);
    for (auto& c : _columns)
        c.resize(m);
}

PosetItemsMatrix PosetItemsMatrix::Transpose() const
{
    PosetItemsMatrix transposed;
    transposed.AssignTransposed(*this);
    return transposed;
}

void PosetItemsMatrix::AssignTransposed( PosetItemsMatrix const& matrix )
{
    int n = matrix.GetWidth();
    int m = matrix.GetHeight();

    Resize(n, m);
    for (int j = 0; j != n; ++j)
    {
        auto& column = matrix._columns[j];
        for (int i = 0; i != m; ++i)
        {
            _columns[i][j] = column[i];
        }
    }
}

void PosetItemsMatrix::DeleteDuplicateColumns()
{
    unique(_columns);
}

void PosetItemsMatrix::DeleteDuplicateRows()
{
    PosetItemsMatrix transposed;
    transposed._columns = std::move(_transposed);
    transposed.AssignTransposed(*this);
    transposed.DeleteDuplicateColumns();
    AssignTransposed(transposed);
    _transposed = std::move(transposed._columns);
}

void PosetItemsMatrix::CopyRow( int i, PosetItems& row ) const
{
    int n = GetWidth();
    row.resize(n);
    for (int j = 0; j != n; ++j)
        row[j] = _columns[j][i];
}