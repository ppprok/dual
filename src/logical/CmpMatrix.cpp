#include "CmpMatrix.h"
#include "bit_chunk.h"



void CmpMatrix::AddComparableObjects(Object const& po, Object const& no)
{
    if (! _comparer)
        return;
    
    if (&no == & po)    
        return;

    _cmpRow.zero(_n);    
    _comparer->Compare(po, no, _cmpRow);
    AddRow(_cmpRow);    
}

void CmpMatrix::AddComparableObjects( ObjectsRange const& positive, ObjectsRange const& negative)
{
    if (! _comparer)
        return;

    _cmpMatrix.reserve(_cmpMatrix.height() + std::max(positive.size(), negative.size()), _n);

    for (auto const& po : positive)
        for(auto const& no : negative)
            AddComparableObjects(positive, negative);
}

void CmpMatrix::AddComparableObjects(Object const& po, ObjectsRange const& negative)
{
    if (! _comparer)
        return;

    _cmpMatrix.reserve(_cmpMatrix.height() + negative.size(), _n);

    for(auto const& no : negative)
         AddComparableObjects(po, *no);
}

bit_matrix& CmpMatrix::GetCmpMatrix()
{
    return _cmpMatrix;
}

CmpMatrix::CmpMatrix()
{
    SetComparer(0);
}

void CmpMatrix::SetComparer( IComparerPtr comparer )
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

    _cmpRow.zero(_n);
    _cmpMatrix.zero(0, _n);
}

struct candidate
{
    int i;
    int j;
    Weight w;

    candidate(int i_, int j_, Weight w_) : i(i_), j(j_), w(w_){};
};


void CmpMatrix::MergeSimilarRows(int restCount, FitMatrix const& fitMatrix)
{
    if (restCount <= 0)
    {
        _cmpMatrix.zero(0,_n);
        return;
    }
    
    int m = _cmpMatrix.height();
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
            if (! _cmpMatrix.get_row(i).intersects(_cmpMatrix.get_row(j)) )
                continue;

            auto w = fitMatrix.GetCommonColumnsWeight(
                _cmpMatrix.get_row(i), 
                _cmpMatrix.get_row(j));
            
            if (candidates[i].j == -1 || candidates[i].w < w)
            {
                candidates[i].j = j;
                candidates[i].w = w;

                candidates[j].j = i;
                candidates[j].w = w;
            }
        }
    }

    // Упорядочивание пар для слияния
    random_sort(candidates, 
        [](candidate const&c1, candidate const&c2)
            {
                if (c1.j == -1)
                    return false;
                return c2.j == -1 || c1.w > c2.w;
            });

    int m1 = m;

    for(auto const& c: candidates)
    {
        
        if (c.j == -1)  // для строки не найдено пары
            break;      // все такие строки располагаются в конце
        
        auto& r1 = _cmpMatrix.get_row(c.i);
        auto& r2 = _cmpMatrix.get_row(c.j);
        
        if (! r1.intersects(r2))
            continue;

        // строки ещё не были слиты
        r1.assign_and(r1, r2);        
        r2.clear();   
        --m1;

        // Обнуляем строки, охватывающие результат слияния строк
        for (int t = 0; t < m; ++t)
        {
            if (t == c.i || t == c.j)
                continue;

            auto& r = _cmpMatrix.get_row(t);
            if (r.empty())
                continue;
            
            if (r1.is_subset_of(r))
            {
                r.clear();
                --m1;
            }                
        }

        if (m1 <= restCount)
            break;
    }

    if (m1 < m)
        _cmpMatrix.remove_zero_rows();
}

void CmpMatrix::AddRow(bit_chunk const& row)
{
    if (row.none())
        return;

    if (_cmpMatrix.find_sub_row(row) >= 0)
        return;

    _cmpMatrix.remove_sup_rows(row);
    _cmpMatrix.add_row(row, _n);
}



