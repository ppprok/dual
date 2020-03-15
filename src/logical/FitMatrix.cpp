#include "FitMatrix.h"
#include "bit_mask.h"




Weight FitMatrix::GetCoveredRowsWeight( bit_chunk cover ) const
{
    if (_fitMatrix.height() == 0)
        return (Weight)cover.count();

    Weight c = 0;
    for (auto& row : _fitMatrix.get_rows())
    {
        if (cover.intersects(row))
            c += row.weight;
    }

    return c;
}




void FitMatrix::AddFittingObject( Object const& precedent, Object const& o, Weight w )
{
    if (! _comparer)
        return;
    
    if (&precedent == &o || w == 0) 
        return;
    
    _cmpRow.zero(_n);
    _comparer->Compare(precedent, o, _cmpRow);

    if (_cmpRow.none())
        return;

    int i = _fitMatrix.find_equal_row(_cmpRow);
    if (i >= 0)
    {
        _fitMatrix.get_row(i).weight += w;
        UpdateColumnWeights(_fitMatrix.get_row(i), w);
        return;
    }

    auto& row = _fitMatrix.add_row(_cmpRow, _n);
    row.weight = w;
    UpdateColumnWeights(row, w);
}




void FitMatrix::AddFittingObject( ObjectsRange const& precedents, Object const& o, Weight w)
{
    if (! _comparer)
        return;

    for(auto const& po:precedents)
        AddFittingObject(*po, o, w);
}

void FitMatrix::AddFittingObjects(Object const& precedent, ObjectsRange const& objects, Weight w)
{
    _fitMatrix.reserve(_fitMatrix.height() + objects.size(), _n);

    for (auto const& o : objects)
        AddFittingObject(precedent, *o, o->weight * w);
}




void FitMatrix::AddFittingObjects( ObjectsRange const& precedents, ObjectsRange const& objects, Weight w)
{
    if (! _comparer)
        return;

    _fitMatrix.reserve(_fitMatrix.height() + std::max(precedents.size(), objects.size()), _n);

    for (auto const& o : objects)
        AddFittingObject(precedents, *o, o->weight * w);
}




weighted_bit_matrix& FitMatrix::GetFitMatrix()
{
    return _fitMatrix;
}

weighted_bit_matrix const& FitMatrix::GetFitMatrix() const
{
    return _fitMatrix;
}



void FitMatrix::SetComparer( IComparerPtr comparer )
{
    _comparer = comparer;
    if (_comparer)
    {
        _n = _comparer->GetSize();
    }
    else
    {
        _n = 0;
    }

    _columnWeights.assign(_n, 0);
    _cmpRow.zero(_n);
    _fitMatrix.zero(0, _n);
}




FitMatrix::FitMatrix()
{
    SetComparer(0);
}




void FitMatrix::UpdateColumnWeights(bit_chunk row, Weight deltaW)
{
    for (int j = row.find_first(); j >=0 && j < _n; j = row.find_next(j))
    {
        _columnWeights[j] += deltaW;
    }
}




void FitMatrix::RemoveSupRows(bit_chunk row)
{
    _fitMatrix.remove_sup_rows(row);
}




void FitMatrix::RemoveCoveredRows(bit_chunk cover)
{
    _fitMatrix.remove_covered_rows(cover);
}




Weight FitMatrix::GetCommonColumnsWeight(bit_chunk const& c1, bit_chunk const& c2) const
{
    Weight w = 0;
    
    bit_mask bm(c1);
    for (auto j = bm.find_first(c2); j != -1; j = bm.find_next(c2, j))
        w += _columnWeights[j];
    
    return w;
}




void FitMatrix::RemoveZeroWeightRows()
{
    _fitMatrix.remove_rows([](weighted_bit_matrix::bit_row const& r){return r.weight == 0;});
}


struct candidate
{
    int i;
    int j;
    Weight w;

    candidate(int i_, int j_, Weight w_):i(i_),j(j_),w(w_){};
};


void FitMatrix::MergeSimilarRows(int restCount)
{
    if (restCount <= 0)
    {
        _fitMatrix.zero(0,_n);
        _columnWeights.assign(_n, 0);
        return;
    }

    RemoveZeroWeightRows();

    int m = _fitMatrix.height();
    if (m <= restCount)
        return;
    
    
    std::vector<candidate> candidates;
    candidates.reserve(m);

    for (int i = 0; i != m; ++i)
        candidates.emplace_back(i, -1, 0.0f);

    // Поиск для каждой строки наиболее подходящей пары
    for (int i = 0; i != m-1; ++i)
    {
        for (int j = i+1; j != m; ++j)
        {
            auto w = GetDiffColumnsWeight(
                _fitMatrix.get_row(i), 
                _fitMatrix.get_row(j));

            if (! _fitMatrix.get_row(i).intersects(_fitMatrix.get_row(j)) )
                continue;

            if (candidates[i].j == -1 || candidates[i].w > w)
            {
                candidates[i].j = j;
                candidates[i].w = w;

                candidates[j].j = i;
                candidates[j].w = w;
            }
        }
    }

    // Упорядочивание пар для слияния
    random_sort(candidates, [](candidate const&c1, candidate const&c2)
    {
        if (c1.j == -1)
            return false;
        return c2.j == -1 || c1.w < c2.w;
    });

    int m0 = m;
        
    for(auto const& c: candidates)
    {
        if (c.j == -1)
            break;
        
        auto& r1 = _fitMatrix.get_row(c.i);        
        auto& r2 = _fitMatrix.get_row(c.j);
        
        if (! r1.intersects(r2))
            continue;
                
        r1.assign_and(r1, r2);
        r1.weight += r2.weight;
        r2.clear();
        --m;

        // Соединяем веса одинаковых строк
        for (int t = 0; t != m0; ++t)
        {
            if (t == c.i || t == c.j)
                continue;

            auto& r = _fitMatrix.get_row(t);
            if (r.none())
                continue;

            if (r == r1)
            {
                r1.weight += r.weight;
                r.clear();
                --m;
            }
        }
                
        if (m <= restCount)
            break;
    }

    if (m != m0)
    {
        _fitMatrix.remove_zero_rows();
        RemoveZeroWeightRows();
    }
}

Weight FitMatrix::GetDiffColumnsWeight( bit_chunk const& c1, bit_chunk const& c2 ) const
{
    Weight w = 0;

    bit_mask bm(c1);
    for (auto j = bm.find_first_diff(c2); j != -1; j = bm.find_next_diff(c2, j))
        w += _columnWeights[j];

    return w;

}
