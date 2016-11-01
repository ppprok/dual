#include "stdafx.h"
#include "CoverMatrix.h"






void CoverMatrix::SetWidth(int n)
{
    _n = n;
    _covers.zero(0, _n);
}

void CoverMatrix::RestMost(int count)
{
    if (count <= 0)
    {
        _covers.zero(0, _n);
        return;
    }

    int m  = _covers.height();
    if (count >= m)
        return;

    _weightOrders.clear();
    _weightOrders.reserve(m);

    for (int i = 0; i < m; ++i)
        _weightOrders.emplace_back(_covers.get_row(i).weight, i);

    random_sort(_weightOrders);
    _weightOrders.resize(m - count);

    for (auto& o:_weightOrders)
        _covers.get_row(o.second).clear();            

    _covers.remove_zero_rows();
}

void CoverMatrix::AddCover(bit_chunk cover, Weight w)
{
    _covers.add_row(weighted_bit_matrix::bit_row(cover, w), _n);
}

weighted_bit_matrix& CoverMatrix::GetCovers()
{
    return _covers;
}

weighted_bit_matrix const& CoverMatrix::GetCovers() const
{
    return _covers;
}

CoverMatrix::CoverMatrix() :_n(0)
{

}
